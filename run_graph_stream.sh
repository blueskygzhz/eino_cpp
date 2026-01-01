#!/bin/bash

echo "=========================================="
echo "  Graph Stream Pipeline Example"
echo "  Graph 流式输出完整示例"
echo "=========================================="
echo ""

# 切换到构建目录
cd "$(dirname "$0")/build" || exit 1

# 编译
echo "📦 Compiling..."
cmake .. > /dev/null 2>&1
make graph_stream_pipeline -j4

if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
    echo ""
    echo "🚀 Running example..."
    echo "=========================================="
    echo ""
    
    # 运行示例
    ./examples/graph_stream_pipeline
    
    echo ""
    echo "=========================================="
    echo "✅ Example completed!"
else
    echo "❌ Compilation failed!"
    exit 1
fi
