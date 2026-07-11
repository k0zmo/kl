FROM ubuntu:26.04

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
    sudo \
    fish \
 && rm -rf /var/lib/apt/lists/*

ARG GCC_VERSION=15
ARG CLANG_VERSION=22
RUN add-apt-repository ppa:ubuntu-toolchain-r/test \
 && apt-get install -y --no-install-recommends \
    g++-${GCC_VERSION} \
    gcc-${GCC_VERSION} \
    clang++-${CLANG_VERSION} \
    llvm-${CLANG_VERSION} \
    libclang-rt-${CLANG_VERSION}-dev \
    python3-pip \
 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION} 90 \
 && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 90 \
 && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${CLANG_VERSION} 90 \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

# Install CMake
ARG CMAKE_VERSION=4.3.0
RUN wget -O - https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz \
  | tar --strip-components=1 --directory /usr/local -xzf -

# Install Ninja
ARG NINJA_VERSION=1.13.2
RUN wget https://github.com/ninja-build/ninja/releases/download/v${NINJA_VERSION}/ninja-linux.zip \
 && unzip ninja-linux.zip -d /usr/local/bin \
 && rm ninja-linux.zip

# Boost
ARG BOOST_VERSION=1.91.0
RUN BOOST_VERSION_U=$(echo "$BOOST_VERSION" | tr . _) \
 && wget -O - "https://archives.boost.io/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_U}.tar.gz" | tar -xzf - \
 && cd "boost_${BOOST_VERSION_U}" \
 && ./bootstrap.sh \
 && ./b2 --with-filesystem link=static cxxflags=-fPIC cflags=-fPIC -a -j$(nproc) install \
 && rm -rf "/tmp/boost_${BOOST_VERSION_U}"
 
# Install CCache
ARG CCACHE_VERSION=4.13.6
RUN mkdir -p /opt/ccache \
 && wget -O - https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/ccache-${CCACHE_VERSION}-linux-x86_64-glibc.tar.xz \
  | tar --strip-components=1 --directory /opt/ccache -xJf - \
 && ln -s /opt/ccache/ccache /usr/local/bin/ccache 

# Install mold
ARG MOLD_VERSION=2.41.0
RUN wget -O - https://github.com/rui314/mold/releases/download/v${MOLD_VERSION}/mold-${MOLD_VERSION}-x86_64-linux.tar.gz \
  | tar --strip-components=1 --directory /usr/local -xzf -

# Configure the default ubuntu user (uid=1000)
ARG USERNAME=ubuntu

RUN chsh -s /usr/bin/fish $USERNAME \
 && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
 && chmod 0440 /etc/sudoers.d/$USERNAME \
 && touch /home/$USERNAME/.sudo_as_admin_successful

USER $USERNAME

WORKDIR /home/$USERNAME

# vcpkg
RUN git clone https://github.com/microsoft/vcpkg \
 && ./vcpkg/bootstrap-vcpkg.sh
ENV VCPKG_ROOT=/home/$USERNAME/vcpkg
ENV PATH="${PATH}:/home/$USERNAME/vcpkg"

# Conan
RUN pip3 install conan --break-system-package
ENV PATH="${PATH}:/home/$USERNAME/.local/bin"
