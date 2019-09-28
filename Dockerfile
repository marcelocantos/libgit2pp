FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
        cmake \
        g++ \
        gdb \
        git \
        libgit2-dev \
        libssh2-1-dev \
        libssl-dev \
        openssl \
        python \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /github.com/libgit2 && \
    cd /github.com/libgit2 && \
    git clone https://github.com/libgit2/libgit2.git

WORKDIR /github.com/libgit2/libgit2
RUN mkdir build && cd build && cmake .. && cmake --build .

RUN mkdir -p /src/demo
COPY git2pp.h /src
COPY demo /src/demo
WORKDIR /src/demo
RUN ARCH=linux make test
