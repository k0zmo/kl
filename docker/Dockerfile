FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    git \
    wget \
    software-properties-common \
 && rm -rf /var/lib/apt/lists/*

# Install gcc-11
RUN add-apt-repository ppa:ubuntu-toolchain-r/test \
 && apt-get update -qq \
 && apt-get install -y --no-install-recommends g++-11 gcc-11 \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 90 \
 && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 90 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

# CMake
RUN wget -O - https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0-Linux-x86_64.tar.gz | tar --strip=1 -C /usr/local -xzf -

# Boost
RUN wget -O - https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.gz | tar -xzf - \
 && cd boost_1_82_0 \
 && ./bootstrap.sh \
 && ./b2 --with-system link=static cxxflags=-fPIC cflags=-fPIC -a -j4 install \
 && rm -rf /tmp/boost_1_82_0

WORKDIR /root
