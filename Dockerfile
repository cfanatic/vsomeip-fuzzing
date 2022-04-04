FROM aflplusplus/aflplusplus:latest AS aflplusplus-base

ENV LD_LIBRARY_PATH=/usr/local/lib/ 
RUN apt-get update
RUN apt-get -y install \
    git \
    nano \
    libboost-system1.71-dev  \
    libboost-thread1.71-dev \
    libboost-log1.71-dev \
    cmake \
    protobuf-compiler \
    gcc-11 \
    g++-11 \
    g++ \
    net-tools \
    iputils-ping

FROM aflplusplus-base

RUN git clone https://github.com/COVESA/vsomeip.git /src/vsomeip
RUN sed -i "s/add_subdirectory( examples\/routingmanagerd )/# add_subdirectory( examples\/routingmanagerd )/g" /src/vsomeip/CMakeLists.txt
RUN mkdir -p /src/vsomeip/build
WORKDIR /src/vsomeip/build
RUN CC=/usr/local/bin/afl-gcc-fast CXX=/usr/local/bin/afl-g++-fast cmake -DENABLE_SIGNAL_HANDLING=1 -DENABLE_MULTIPLE_ROUTING_MANAGERS=0 ..
RUN make
RUN make install

COPY . /src/vsomeip-fuzzing
RUN mkdir -p /src/vsomeip-fuzzing/build
WORKDIR /src/vsomeip-fuzzing/build
RUN CC=/usr/local/bin/afl-gcc-fast CXX=/usr/local/bin/afl-g++-fast cmake -DCOVERAGE=1 ..
RUN cp ../conf/vsomeip_response.json vsomeip.json
RUN make fuzzing
RUN mkdir afl_input afl_output
RUN cp vsomeip.json afl_input/

CMD [ "afl-fuzz", "-V", "3", "-m", "500", "-i", "afl_input/", "-o", "afl_output/", "./fuzzing", "@@" ]
