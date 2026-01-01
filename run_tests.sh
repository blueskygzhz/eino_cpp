#!/bin/bash

# ADK Unit Tests Run Script
# This script runs the ADK unit tests with various options

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
TEST_EXECUTABLE="${BUILD_DIR}/tests/adk_tests"

echo "=========================================="
echo "ADK Unit Tests Runner"
echo "=========================================="
echo ""

# Check if test executable exists
if [ ! -f "$TEST_EXECUTABLE" ]; then
    echo "❌ Test executable not found: $TEST_EXECUTABLE"
    echo ""
    echo "Please build the tests first:"
    echo "  ./build_tests.sh"
    exit 1
fi

# Default to running all tests
TEST_FILTER="${1:-.}"

echo "Running tests with filter: $TEST_FILTER"
echo ""

# Run tests with verbose output
"$TEST_EXECUTABLE" --gtest_filter="$TEST_FILTER" --gtest_print_time=1

# Capture exit code
EXIT_CODE=$?

echo ""
echo "=========================================="
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ All tests passed!"
else
    echo "❌ Some tests failed (exit code: $EXIT_CODE)"
fi
echo "=========================================="

exit $EXIT_CODE
