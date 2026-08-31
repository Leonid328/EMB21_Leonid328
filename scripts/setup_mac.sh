#!/bin/bash
# setup_mac.sh - Install required dependencies for macOS (requires Homebrew)

echo "Installing macOS dependencies..."

if ! command -v brew &> /dev/null; then
    echo "Homebrew is not installed! Please install Homebrew first: https://brew.sh/"
    exit 1
fi

brew install \
    python \
    clang-format \
    arm-none-eabi-gcc \
    make \
    cmake \
    git

# Install PlatformIO
if ! command -v pio &> /dev/null; then
    echo "Installing PlatformIO..."
    pip3 install -U platformio
else
    echo "PlatformIO is already installed."
fi

echo "Setup complete! Please restart your terminal."
