FROM ubuntu:16.04

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    git \
    wget \
    build-essential \
    ninja-build \
    software-properties-common \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

# CMake
RUN wget -O - https://github.com/Kitware/CMake/releases/download/v3.15.4/cmake-3.15.4-Linux-x86_64.tar.gz | tar --strip=1 -C /usr/local -xzf -

# Boost
RUN wget -O - https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz | tar -xzf - \
 && cd boost_1_68_0 \
 && ./bootstrap.sh \
 && ./b2 --with-system link=static cxxflags=-fPIC cflags=-fPIC -a -j4 install \
 && rm -rf /tmp/boost_1_68_0

# Install gcc-7
RUN add-apt-repository ppa:ubuntu-toolchain-r/test \
 && apt-get update -qq \
 && apt-get install -y --no-install-recommends g++-7 gcc-7 \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 90 \
 && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 90 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /root
