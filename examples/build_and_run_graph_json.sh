#!/bin/bash

# Build and run graph JSON serialization example
# Usage: ./build_and_run_graph_json.sh

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}  Building Graph JSON Serialization Example${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""

# Configuration
BUILD_DIR="build_graph_json"
EXECUTABLE="graph_json_example"
SOURCE_FILE="graph_json_example.cpp"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Compile
echo -e "${YELLOW}Compiling...${NC}"

g++ -std=c++14 \
    "../${SOURCE_FILE}" \
    -I../../include \
    -I/usr/include \
    -o "$EXECUTABLE" \
    -Wall -Wextra -Wno-unused-parameter \
    2>&1

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Build successful!${NC}"
    echo ""
else
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi

# Run
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}  Running Graph JSON Example${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""

"./$EXECUTABLE"

echo ""
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}  Execution Complete${NC}"
echo -e "${GREEN}================================================${NC}"
