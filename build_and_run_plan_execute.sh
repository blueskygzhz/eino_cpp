#!/bin/bash
# Build and Run Plan-Execute Example

set -e  # Exit on error

echo "╔═══════════════════════════════════════════════════════════════════╗"
echo "║                                                                   ║"
echo "║    EINO C++ - Plan-Execute Example Build & Run Script            ║"
echo "║                                                                   ║"
echo "╚═══════════════════════════════════════════════════════════════════╝"
echo ""

# Step 1: Create build directory
echo "Step 1: Creating build directory..."
mkdir -p build
cd build

# Step 2: Configure with CMake
echo ""
echo "Step 2: Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Step 3: Build
echo ""
echo "Step 3: Building plan_execute_example..."
make plan_execute_example -j8

# Step 4: Check if build succeeded
if [ -f "./examples/plan_execute_example" ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    
    # Step 5: Run the example
    echo "Step 4: Running plan_execute_example..."
    echo ""
    echo "═══════════════════════════════════════════════════════════════════"
    echo ""
    
    ./examples/plan_execute_example
    
    echo ""
    echo "═══════════════════════════════════════════════════════════════════"
    echo ""
    echo "✓ Example completed successfully!"
    
else
    echo ""
    echo "✗ Build failed - executable not found"
    exit 1
fi
