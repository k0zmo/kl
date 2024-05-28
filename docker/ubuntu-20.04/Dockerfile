FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    git \
    wget \
    software-properties-common \
    apt-transport-https \
 && rm -rf /var/lib/apt/lists/*

# Register the Microsoft repository GPG keys
RUN wget -q "https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/packages-microsoft-prod.deb" \
 && dpkg -i packages-microsoft-prod.deb \
 && rm packages-microsoft-prod.deb

# Install gcc, clang and dotnet 6.0 (required for CMake VScode extension)
RUN add-apt-repository ppa:ubuntu-toolchain-r/test \
 && apt-get update -qq \
 && apt-get install -y --no-install-recommends \
    ninja-build \
    g++-11 \
    gcc-11 \
    clang++-12 \
    llvm-12 \
    dotnet-runtime-6.0 \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 90 \
 && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 90 \
 && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-12 90 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

# CMake
RUN wget -O - https://github.com/Kitware/CMake/releases/download/v3.29.3/cmake-3.29.3-Linux-x86_64.tar.gz | tar --strip=1 -C /usr/local -xzf -

# Boost
RUN wget -O - https://boostorg.jfrog.io/artifactory/main/release/1.85.0/source/boost_1_85_0.tar.gz | tar -xzf - \
 && cd boost_1_85_0 \
 && ./bootstrap.sh \
 && ./b2 --with-system link=static cxxflags=-fPIC cflags=-fPIC -a -j$(nproc) install \
 && rm -rf /tmp/boost_1_85_0

WORKDIR /root
