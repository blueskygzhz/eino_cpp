#!/bin/bash
# DAG Compose Example Build and Run Script

set -e  # Exit on error

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║   Eino C++ DAG Compose Example - Build & Run Script         ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
EXAMPLE_NAME="dag_compose_example"
EXECUTABLE="$BUILD_DIR/examples/$EXAMPLE_NAME"

echo -e "${BLUE}[1/4] Checking environment...${NC}"
echo "  Project root: $PROJECT_ROOT"
echo "  Build directory: $BUILD_DIR"

# Check if nlohmann/json is available
if ! pkg-config --exists nlohmann_json 2>/dev/null; then
    echo -e "${YELLOW}  Warning: nlohmann_json not found via pkg-config${NC}"
    echo -e "${YELLOW}  Make sure nlohmann/json is installed or available in the include path${NC}"
fi

echo ""
echo -e "${BLUE}[2/4] Creating build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo -e "${BLUE}[3/4] Building project with CMake...${NC}"
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "  Running CMake configuration..."
    cmake "$PROJECT_ROOT" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        || {
            echo -e "${RED}✗ CMake configuration failed${NC}"
            exit 1
        }
else
    echo "  Using existing CMake configuration"
fi

echo ""
echo "  Compiling $EXAMPLE_NAME..."
cmake --build . --target "$EXAMPLE_NAME" -j$(nproc 2>/dev/null || echo 4) || {
    echo -e "${RED}✗ Compilation failed${NC}"
    exit 1
}

echo ""
echo -e "${GREEN}✓ Build completed successfully!${NC}"

if [ -f "$EXECUTABLE" ]; then
    echo ""
    echo -e "${BLUE}[4/4] Running $EXAMPLE_NAME...${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    
    "$EXECUTABLE" || {
        echo ""
        echo -e "${RED}✗ Execution failed with exit code $?${NC}"
        exit 1
    }
    
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo -e "${GREEN}✓ Example executed successfully!${NC}"
else
    echo -e "${RED}✗ Executable not found: $EXECUTABLE${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                   All steps completed! ✓                     ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "Executable location: $EXECUTABLE"
echo ""
echo "To run again: $EXECUTABLE"
echo "To rebuild:   cd $BUILD_DIR && cmake --build . --target $EXAMPLE_NAME"
echo ""
