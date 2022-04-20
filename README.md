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

- Debian 11.2
- vsomeip 3.1.14
- boost 1.65.1

## Setup

The instructions below describe how to build a SOME/IP service and client application.
They follow the [Request/Response](https://github.com/COVESA/vsomeip/tree/master/examples) communication pattern.

### 1. Prepare host

```text
mkdir fuzzing
cd fuzzing/
git clone https://github.com/COVESA/vsomeip.git
git clone -b feature-demo-someip https://github.com/cfanatic/vsomeip-fuzzing.git
mkdir vsomeip/build vsomeip-fuzzing/build
```

### 2. Configure host

```text
apt update
apt install -y libboost-system1.65-dev libboost-thread1.65-dev libboost-log1.65-dev
apt install -y cmake protobuf-compiler
apt install -y gcc-5
apt install -y g++-5
apt install -y g++
apt install -y asciidoc source-highlight doxygen graphviz
```

### 3. Build library

```text
cd vsomeip/build/
cmake -DENABLE_SIGNAL_HANDLING=1 -DENABLE_MULTIPLE_ROUTING_MANAGERS=1 ..
make
make install
```

### 4. Build targets

```text
cd vsomeip-fuzzing/build/
cmake ..
make request response
```

## Usage

In separate terminals, call the service and client application:

```text
VSOMEIP_CONFIGURATION=../conf/vsomeip_request.json ./request
VSOMEIP_CONFIGURATION=../conf/vsomeip_response.json ./response
```

In case of a dynamic library loading error, try to point `LD_LIBRARY_PATH` to the library build folder:

```text
export LD_LIBRARY_PATH=~/fuzzing/vsomeip/build
```

You can trigger the Request/Response exchange by entering `s` in the terminal window of the client.
