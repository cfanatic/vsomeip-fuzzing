# vsomeip-fuzzing

This repository hosts a fuzzing environment for a [SOME/IP implementation](https://github.com/COVESA/vsomeip) developed by BMW AG.

In the automotive industry, the SOME/IP protocol is used for Ethernet-based communication. It will gain in popularity in the future, since self-driving cars record large amounts of data which needs to be transmitted among sensors, actuators and control units in real-time. A robust protocol implementation is key for secure and safe vehicle operation.

Following targets are implemented on respective branches:

- *master / feature-demo-afl*: Software fuzzing using [AFL++](https://github.com/AFLplusplus/AFLplusplus)
- *feature-demo-someip*: Network fuzzing using [someip-protocol-fuzzer](https://github.com/cfanatic/someip-protocol-fuzzer)

According to Wikipedia:
> Fuzzing is an automated software testing technique that involves providing invalid, unexpected, or random data as inputs to a computer program. The program is then monitored for exceptions such as crashes, failing built-in code assertions, or potential memory leaks.

## Requirements

Developed and tested on the following setup:

- macOS 10.15.5
- vsomeip 3.1.14
- boost 1.65.1
- docker 2.3.0.3

## Setup

Build the [vsomeip library](https://github.com/COVESA/vsomeip) and the [fuzzing target](fuzzing.cpp):

```text
docker build -t vsomeip-fuzzing .
```

Run a detached container:

```text
docker run -t -d --name vsomeip-fuzz vsomeip-fuzzing bash
```

## Fuzzing

Perform a fuzz session for 10 seconds:

```text
docker exec -it vsomeip-fuzz ../misc/runtime.sh -fuzz 10
```

Create a coverage report of the fuzz session:

```text
docker exec -it vsomeip-fuzz ../misc/runtime.sh -report
docker cp vsomeip-fuzz:/src/vsomeip-fuzzing/build/afl_output .
```

Open `afl_output/cov/web/src/vsomeip-fuzzing/index.html`, and review the coverage results.

## Instrumentation

You might want to make sure that AFL++ catches crashes in the vsomeip library prior to long fuzzing sessions.
You can add following code to [vsomeip/implementation/logger/src/message.cpp](https://github.com/COVESA/vsomeip/blob/master/implementation/logger/src/message.cpp) which causes a null pointer exception whenever the fuzzed payload in `buffer_` is equal to one of the items in vector `v`:

```cpp
#ifdef CRASH_LIBRARY
if (level_ == level_e::LL_FATAL) {
    std::vector<std::string> v = {"Hello", "hullo", "hell"};
    if (std::find(v.begin(), v.end(), buffer_.data_.str()) != v.end()) {
        *(int *)0 = 0; // crash: null pointers cannot be dereferenced to a value
    }
}
#endif
````

The crash can be triggered by inserting the fuzzed payload to the `<<` operator of `VSOMEIP_FATAL` somewhere in [fuzzing.cpp](fuzzing.cpp):

```cpp
#ifdef CRASH_LIBRARY
VSOMEIP_FATAL << str_payload;
#endif
```
