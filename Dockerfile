FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
        gdb \
        git \
        libgit2-dev \
    && : # rm -rf /var/lib/apt/lists/*

RUN mkdir -p /github.com/libgit2 && \
    cd /github.com/libgit2 && \
    git clone https://github.com/libgit2/libgit2.git

RUN apt-get remove -y libgit2-dev
RUN apt-get install -y openssl
RUN apt-get install -y cmake
RUN apt-get install -y libssl-dev
RUN apt-get install -y libssh2-1-dev
RUN apt-get install -y python

RUN cd /github.com/libgit2/libgit2 && \
    mkdir build && \
    cd build && \
    cmake .. && \
    cmake --build .

RUN apt-get install -y g++

RUN mkdir -p /src/demo
# COPY git2pp.h .
# COPY demo demo/
WORKDIR /src/demo
# RUN make test

ENV LD_LIBRARY_PATH /github.com/libgit2/libgit2/build
# RUN ARCH=linux CPPFLAGS=-I/github.com/libgit2/libgit2/include LDFLAGS=-L/github.com/libgit2/libgit2/build make test
