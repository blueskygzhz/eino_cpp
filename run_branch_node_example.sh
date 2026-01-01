#!/bin/bash

# 一键编译并运行 BranchNode 示例

echo "=========================================="
echo "  BranchNode 示例 - 一键运行"
echo "=========================================="
echo ""

cd /data/workspace/QQMail/eino_cpp

# 检查是否已编译
if [ ! -f "build_branch/branch_node_simple" ]; then
    echo "📦 首次运行，正在编译..."
    echo ""
    ./compile_branch_node_simple.sh
    echo ""
else
    echo "✓ 检测到已编译的可执行文件"
    echo ""
fi

echo "🚀 运行 BranchNode 示例..."
echo ""
echo "=========================================="
./build_branch/branch_node_simple
echo "=========================================="
echo ""
echo "✅ 示例运行完成!"
echo ""
echo "📖 查看完整文档: cat BRANCH_NODE_EXAMPLE_README.md"
echo ""
