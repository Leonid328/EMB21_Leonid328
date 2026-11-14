#!/bin/bash
# setup_linux.sh - Install required dependencies for Ubuntu/Debian

echo "Installing Linux dependencies..."

sudo apt update
sudo apt install -y \
    python3 \
    python3-pip \
    python3-venv \
    clang-format \
    clang-tidy \
    gcc-arm-none-eabi \
    make \
    cmake \
    git

# Install PlatformIO globally (or use venv if preferred)
if ! command -v pio &> /dev/null; then
    echo "Installing PlatformIO..."
    pip3 install -U platformio
else
    echo "PlatformIO is already installed."
fi

echo "Setup complete! Please restart your terminal."
