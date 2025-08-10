#!/bin/bash

echo "Building Twine Compiler..."

# Check if we have the necessary tools
if ! command -v llvm-config &> /dev/null; then
    echo "Error: llvm-config not found in PATH"
    echo "Please install LLVM and ensure llvm-config is accessible"
    echo "On Ubuntu/Debian: sudo apt install llvm-dev"
    echo "On macOS: brew install llvm"
    echo "On Arch: sudo pacman -S llvm"
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "Error: g++ not found in PATH"
    echo "Please install a C++ compiler"
    echo "On Ubuntu/Debian: sudo apt install build-essential"
    echo "On macOS: xcode-select --install"
    echo "On Arch: sudo pacman -S gcc"
    exit 1
fi

echo "Compiling Twine Compiler with g++..."

# Get LLVM flags
LLVM_FLAGS=$(llvm-config --cxxflags --ldflags --system-libs --libs core support irreader codegen mc mcparser option target)

# Compile with proper include path
g++ -std=c++17 -Iinclude -o twine src/main.cpp src/lexer.cpp src/parser.cpp src/ast.cpp src/codegen.cpp $LLVM_FLAGS

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo "Compiler is at: twine"
echo ""
echo "Example usage:"
echo "  ./twine examples/fibonacci.tw"
echo "  ./twine examples/math.tw --verbose"
echo ""