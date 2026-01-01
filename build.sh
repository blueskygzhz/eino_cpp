#!/bin/bash
# Build script for eino_cpp

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"
INSTALL_DIR="${BUILD_DIR}/install"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Eino C++ Build Script ===${NC}"
echo "Project directory: $PROJECT_DIR"

# Create build directory
echo -e "${BLUE}Creating build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Run CMake
echo -e "${BLUE}Running CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"

# Build
echo -e "${BLUE}Building...${NC}"
make -j$(nproc)

# Install
echo -e "${BLUE}Installing...${NC}"
make install

echo -e "${GREEN}Build completed successfully!${NC}"
echo "Install directory: $INSTALL_DIR"
