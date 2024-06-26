FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    git \
    wget \
    zip \
    unzip \
    curl \
    software-properties-common \
    apt-transport-https \
    gpg-agent \
    xz-utils \
 && rm -rf /var/lib/apt/lists/*

# Register the Microsoft repository GPG keys
RUN wget -q "https://packages.microsoft.com/config/ubuntu/$(lsb_release -rs)/packages-microsoft-prod.deb" \
 && dpkg -i packages-microsoft-prod.deb \
 && rm packages-microsoft-prod.deb

# https://stackoverflow.com/questions/73753672/a-fatal-error-occurred-the-folder-usr-share-dotnet-host-fxr-does-not-exist
RUN echo "Package: *\n\
Pin: origin \"packages.microsoft.com\"\n\
Pin-Priority: 1001" > /etc/apt/preferences.d/99-microsoft-dotnet.pref

# Install gcc, clang and dotnet 6.0 (required for CMake VScode extension)
RUN add-apt-repository ppa:ubuntu-toolchain-r/test \
 && apt-get update -qq \
 && apt-get install -y --no-install-recommends \
    ninja-build \
    g++-13 \
    gcc-13 \
    clang++-15 \
    llvm-15 \
    dotnet-runtime-6.0 \
    python3-pip \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 90 \
 && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 90 \
 && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-15 90 \
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

# CCache
RUN mkdir -p /opt/ccache \
 && wget -O - https://github.com/ccache/ccache/releases/download/v4.9/ccache-4.9-linux-x86_64.tar.xz | tar --strip=1 -C /opt/ccache --strip=1 -xJf - \
 && ln -s /opt/ccache/ccache /usr/local/bin/ccache 

ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
 && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME -s /bin/bash \
   && apt-get update -qq \
   && apt-get install -y sudo \
   && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
   && chmod 0440 /etc/sudoers.d/$USERNAME \
   && rm -rf /var/lib/apt/lists/*

USER $USERNAME

WORKDIR /home/$USERNAME

# vcpkg
RUN git clone https://github.com/microsoft/vcpkg \
 && ./vcpkg/bootstrap-vcpkg.sh
ENV VCPKG_ROOT=/home/$USERNAME/vcpkg
ENV PATH="${PATH}:/home/$USERNAME/vcpkg"

# Conan
RUN pip3 install conan
ENV PATH="${PATH}:/home/$USERNAME/.local/bin"
