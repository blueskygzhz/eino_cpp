#!/bin/bash
# 快速运行 Stream Mode 示例

set -e

echo "=================================="
echo "  Eino C++ Stream Mode Example"
echo "=================================="
echo ""

# 检查是否已编译
if [ ! -f "build/examples/stream_mode_simple" ]; then
    echo "📦 Compiling stream_mode_simple..."
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make stream_mode_simple -j4
    cd ..
    echo "✅ Compilation completed!"
    echo ""
fi

# 运行示例
echo "🚀 Running stream_mode_simple..."
echo ""
./build/examples/stream_mode_simple

echo ""
echo "=================================="
echo "  ✅ Example completed!"
echo "=================================="
