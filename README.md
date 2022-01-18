# vsomeip-fuzzing

In the automotive industry, the SOME/IP protocol is used for Ethernet-based communication. It will gain in popularity in the future, since self-driving cars record large amounts of data which needs to be transmitted among sensors, actuators and control units in real-time. A robust protocol implementation is key for secure and safe vehicle operation.

This repository hosts a fuzz testing suite based on [AFL++](https://github.com/AFLplusplus/AFLplusplus) for a [SOME/IP implementation](https://github.com/GENIVI/vsomeip) developed by BMW AG. The setup instructions below explain how to build the code examples of the [GENIVI/vsomeip in 10 minutes](https://github.com/GENIVI/vsomeip/wiki/vsomeip-in-10-minutes#first) tutorial, which are used as the fuzzing targets.

According to Wikipedia:
> Fuzzing is an automated software testing technique that involves providing invalid, unexpected, or random data as inputs to a computer program. The program is then monitored for exceptions such as crashes, failing built-in code assertions, or potential memory leaks.

## Requirements

Developed and tested on the following setup:

- macOS 10.15.5
- vsomeip 3.1.14
- boost 1.65.1
- docker 2.3.0.3

## Setup

Don't forget to replace <*your-working-directory*> below.

_Update on 18.01.2022: It is recommended to ignore the steps below, and build a Docker image based on the `Dockerfile` instead. Perform the instructions in `setup.sh` to get your setup running._

### 1. Prepare host

```bash
cd <your-working-directory>
git clone https://github.com/GENIVI/vsomeip.git
```

### 2. Build container

```bash
docker pull ubuntu:18.04
docker run -it -v <your-working-directory>/vsomeip:/root/vsomeip --name vsomeip ubuntu:18.04
docker exec -it vsomeip bash
docker start vsomeip
docker stop vsomeip
```

### 3. Configure container

```bash
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
aptitude search boost
```

### 4. Build library

```bash
cd /root/vsomeip
mkdir build
cd build/
cmake -DENABLE_SIGNAL_HANDLING=1 -DENABLE_MULTIPLE_ROUTING_MANAGERS=1 ..
make
```

### 5. Build tutorial

```bash
mkdir -p /root/vsomeip/examples/tutorial/build
cd /root/vsomeip/examples/tutorial
git clone https://github.com/cfanatic/vsomeip-fuzzing.git
cd build
cmake ..
make request response
LD_LIBRARY_PATH=/root/vsomeip/build ./service
```

In case you would like to build the fuzzing target with a compiler other than one that is shipped with AFL++, run the following call to `cmake`:

```bash
CC=gcc CXX=g++ cmake -D USE_GCC=ON ..
make fuzzing
```

### 6. Fuzz tutorial

Replace the call to `cmake` in section 4 with the following instruction below:

```bash
CC=/usr/local/bin/afl-clang-fast CXX=/usr/local/bin/afl-clang-fast++ cmake -DENABLE_SIGNAL_HANDLING=1 -DENABLE_MULTIPLE_ROUTING_MANAGERS=1 ..
```

Replace the calls to `cmake` and `make` in section 5 with the following instruction below:

```bash
CC=/usr/local/bin/afl-clang-fast CXX=/usr/local/bin/afl-clang-fast++ cmake ..
make fuzzing
```

Run a fuzzing session by calling:

```bash
afl-fuzz -m 500 -i afl/input/ -o afl/finding/ ./fuzzing @@
```

### 7. Instrument library

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
