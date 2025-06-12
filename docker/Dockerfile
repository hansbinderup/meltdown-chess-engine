# Use Arch Linux as the base image
FROM archlinux:latest

# Install necessary packages
RUN pacman -Syu --noconfirm \
    && pacman -Sy --noconfirm \
    base-devel \
    clang \
    gcc \
    mingw-w64-gcc \
    lld \
    llvm \
    meson \
    ninja \
    gdb \
    git \
    cmake \
    wget \
    libxml2 \
    openmp \
    python \
    python-pip

# Download and install LLVM-Mingw toolchain
RUN cd /tmp \
    && wget -O llvm-mingw.tar.xz https://github.com/mstorsjo/llvm-mingw/releases/download/20250613/llvm-mingw-20250613-ucrt-ubuntu-22.04-x86_64.tar.xz \
    && mkdir -p llvm-mingw \
    && tar -xf llvm-mingw.tar.xz -C llvm-mingw --strip-components=1 \
    && mv llvm-mingw /opt/llvm-mingw \
    && rm -rf /tmp/*

# add clang-llvm to path
ENV PATH="/opt/llvm-mingw/bin:${PATH}"

# Set the working directory
WORKDIR /workspace

# Create a new user named "user" with home directory and no password
RUN useradd -m -s /bin/bash user && \
    echo "user ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# Set working directory inside the container
WORKDIR /workspace

# Change ownership of the workspace to "user"
RUN chown -R user:user /workspace

# Switch to the non-root user
USER user
