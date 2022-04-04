# vsomeip-fuzzing

In the automotive industry, the SOME/IP protocol is used for Ethernet-based communication. It will gain in popularity in the future, since self-driving cars record large amounts of data which needs to be transmitted among sensors, actuators and control units in real-time. A robust protocol implementation is key for secure and safe vehicle operation.

This repository hosts a fuzzing environment based on [AFL++](https://github.com/AFLplusplus/AFLplusplus) for a [SOME/IP implementation](https://github.com/COVESA/vsomeip) developed by BMW AG. The setup instructions below explain how to build the code examples of the [COVESA/vsomeip in 10 minutes](https://github.com/COVESA/vsomeip/wiki/vsomeip-in-10-minutes#first) tutorial, which are used as the fuzzing targets.

According to Wikipedia:
> Fuzzing is an automated software testing technique that involves providing invalid, unexpected, or random data as inputs to a computer program. The program is then monitored for exceptions such as crashes, failing built-in code assertions, or potential memory leaks.

## Requirements

Developed and tested on the following setup:

- macOS 10.15.5
- vsomeip 3.1.14
- boost 1.65.1
- docker 2.3.0.3

## Setup

***Update on 18.01.2022**: It is recommended to ignore the steps below, and build a Docker image based on the `Dockerfile` instead. Perform the instructions in `setup.sh` to get your setup running.*

Using the instructions below, you create a Docker container on your host computer in which you perform the build process.


### 1. Prepare host

```text
cd <your-working-directory>
git clone https://github.com/COVESA/vsomeip.git
```

### 2. Build container

```text
docker pull ubuntu:18.04
docker run -it -v <your-working-directory>/vsomeip:/root/vsomeip --name vsomeip ubuntu:18.04
docker exec -it vsomeip bash
```

### 3. Configure container

```text
apt update
apt install -y aptitude
apt install -y nano
apt install -y git
apt install -y libboost-system1.65-dev libboost-thread1.65-dev libboost-log1.65-dev
apt install -y cmake protobuf-compiler
apt install -y gcc-5
apt install -y g++-5
apt install -y g++
apt install -y asciidoc source-highlight doxygen graphviz
apt install -y net-tools
apt install -y iputils-ping
apt install -y afl++
aptitude search boost
```

### 4. Build library

```text
cd /root/vsomeip
mkdir build
cd build/
CC=/usr/local/bin/afl-clang-fast CXX=/usr/local/bin/afl-clang-fast++ \
cmake -DENABLE_SIGNAL_HANDLING=1 -DENABLE_MULTIPLE_ROUTING_MANAGERS=1 ..
make
make install
```

### 5. Build target

```text
mkdir -p /root/vsomeip/examples/tutorial/build
cd /root/vsomeip/examples/tutorial
git clone https://github.com/cfanatic/vsomeip-fuzzing.git
cd build
CC=/usr/local/bin/afl-clang-fast CXX=/usr/local/bin/afl-clang-fast++ cmake ..
cp ../conf/vsomeip_response.json vsomeip.json
make fuzzing
```

In case you would like to build the fuzzing target with a compiler other than one that is shipped with AFL++, run the following call to `cmake`:

```text
CC=gcc CXX=g++ cmake -D USE_GCC=ON ..
make fuzzing
```

### 6. Run target

Test if the setup is working by calling:

```text
./fuzzing vsomeip.json
```

In case of a dynamic library loading error, try instead:

```text
LD_LIBRARY_PATH=/root/vsomeip/build ./fuzzing vsomeip.json
```

## Fuzzing

Perform a fuzz session on the target by calling:

```text
mkdir -p afl_input afl_output
echo "hello" > afl_input/seed
afl-fuzz -m 500 -i afl_input/ -o afl_output/ ./fuzzing @@
```

## Instrumentation

You might want to make sure that AFL++ catches crashes in the vsomeip library prior to long fuzzing sessions. You can add following code to `vsomeip/implementation/logger/src/message.cpp` which causes a null pointer exception whenever the fuzzed payload in `buffer_` is equal to one of the items in vector `v`:

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

The crash can be triggered by inserting the fuzzed payload to the `<<` operator of `VSOMEIP_FATAL` somewhere in `fuzzing.cpp`:

```cpp
#ifdef CRASH_LIBRARY
VSOMEIP_FATAL << str_payload;
#endif
```
