# Use Arch Linux as the base image
FROM archlinux:latest

# Install necessary packages
RUN pacman -Sy --noconfirm \
    base-devel \
    gcc \
    mingw-w64-gcc \
    meson \
    ninja \
    gdb \
    git \
    cmake \
    python \
    python-pip \
    && pacman -Scc --noconfirm  # Clean package cache

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
