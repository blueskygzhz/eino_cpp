#!/bin/bash

# ADK Unit Tests Build Script
# This script builds and runs the ADK unit tests

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

echo "=========================================="
echo "ADK Unit Tests Build Script"
echo "=========================================="
echo ""

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

echo "Running CMake configuration..."
cmake .. || {
    echo "❌ CMake configuration failed"
    exit 1
}

echo "Building ADK tests..."
cmake --build . --target adk_tests --config Release || {
    echo "❌ Build failed"
    exit 1
}

echo ""
echo "=========================================="
echo "Build completed successfully!"
echo "=========================================="
echo ""

# Check if test executable exists
if [ -f "./tests/adk_tests" ]; then
    echo "Test executable created: ./tests/adk_tests"
    echo ""
    echo "To run the tests, execute:"
    echo "  ./tests/adk_tests"
    echo ""
    echo "To run specific tests:"
    echo "  ./tests/adk_tests --gtest_filter=ADKTypesTest.*"
    echo "  ./tests/adk_tests --gtest_filter=ADKPrebuiltDeepTest.*"
    echo ""
else
    echo "⚠️  Test executable not found"
    exit 1
fi
