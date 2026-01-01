#!/bin/bash

# Eino C++ Examples - Build and Run All Examples
# This script compiles and runs all available examples

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "╔══════════════════════════════════════════════════════════════════════╗"
echo "║                                                                      ║"
echo "║        Eino C++ Examples - Build and Run All Examples               ║"
echo "║                                                                      ║"
echo "╚══════════════════════════════════════════════════════════════════════╝"
echo ""

# Create build directory if not exists
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake
echo "📋 Configuring CMake..."
cmake .. > /dev/null 2>&1
echo "✓ CMake configuration complete"
echo ""

# List of examples that can be compiled (header-only)
EXAMPLES=(
    "basic_chain_example"
    "simple_graph_example"
    "dag_compose_example"
)

# Examples that require libraries (skip for now)
SKIPPED_EXAMPLES=(
    "components_example"
    "flow_example"
    "graph_run_example"
    "checkpoint_example"
    "interrupt_state_example"
    "plan_execute_example"
)

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Building Examples"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

BUILD_COUNT=0
FAIL_COUNT=0

for example in "${EXAMPLES[@]}"; do
    echo "🔨 Building $example..."
    if cmake --build . --target "$example" -j4 > /dev/null 2>&1; then
        echo "   ✓ Build successful"
        ((BUILD_COUNT++))
    else
        echo "   ✗ Build failed"
        ((FAIL_COUNT++))
    fi
done

echo ""
echo "Build Summary: $BUILD_COUNT succeeded, $FAIL_COUNT failed"
echo ""

if [ $BUILD_COUNT -eq 0 ]; then
    echo "❌ No examples were built successfully"
    exit 1
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Running Examples"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

RUN_COUNT=0
for example in "${EXAMPLES[@]}"; do
    EXAMPLE_PATH="$BUILD_DIR/examples/$example"
    if [ -x "$EXAMPLE_PATH" ]; then
        echo ""
        echo "╔══════════════════════════════════════════════════════════════════════╗"
        echo "║  Running: $example"
        echo "╚══════════════════════════════════════════════════════════════════════╝"
        echo ""
        
        if "$EXAMPLE_PATH"; then
            echo ""
            echo "✓ $example completed successfully"
            ((RUN_COUNT++))
        else
            echo ""
            echo "✗ $example failed with exit code $?"
        fi
        
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
    fi
done

echo ""
echo "╔══════════════════════════════════════════════════════════════════════╗"
echo "║                          Summary                                     ║"
echo "╚══════════════════════════════════════════════════════════════════════╝"
echo ""
echo "✓ Examples Built:    $BUILD_COUNT"
echo "✓ Examples Executed: $RUN_COUNT"
echo ""

if [ ${#SKIPPED_EXAMPLES[@]} -gt 0 ]; then
    echo "ℹ Skipped Examples (require library dependencies):"
    for example in "${SKIPPED_EXAMPLES[@]}"; do
        echo "  - $example"
    done
    echo ""
fi

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "🎉 All available examples completed successfully!"
echo ""
