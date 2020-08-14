# vsomeip-testing

This manual describes the necessary steps to follow in order to compile the code examples in the [GENIVI/vsomeip in 10 minutes](https://github.com/GENIVI/vsomeip/wiki/vsomeip-in-10-minutes#first) tutorial on a macOS host. Also, it describes how to configure the tutorial for fuzzing with AFL++.

A Dockerfile will follow at a later point in time.

## Requirements

Developed and tested on the following setup:

- macOS 10.15.5
- vsomeip 3.1.14
- boost 1.65.1
- docker 2.3.0.3

## Setup

Don't forget to replace <*your-working-directory*> below.

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
git clone https://github.com/cfanatic/vsomeip-testing.git
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
