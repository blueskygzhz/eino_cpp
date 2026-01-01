#!/bin/bash

# 编译 branch_node_simple 示例
# 只编译必需的源文件，避免其他模块的编译错误

set -e

echo "=========================================="
echo "  编译 BranchNode 简单示例"
echo "=========================================="
echo ""

PROJECT_ROOT="/data/workspace/QQMail/eino_cpp"
cd "$PROJECT_ROOT"

# 创建输出目录
mkdir -p build_branch

echo "[1/3] 编译 branch_node.cpp..."
g++-12 -c -std=c++17 \
    -I./include \
    -I./third_party \
    -o build_branch/branch_node.o \
    src/compose/branch_node.cpp

echo "[2/3] 编译 branch_node_simple.cpp..."
g++-12 -c -std=c++17 \
    -I./include \
    -I./third_party \
    -o build_branch/branch_node_simple.o \
    examples/branch_node_simple.cpp

echo "[3/3] 链接生成可执行文件..."
g++-12 -std=c++17 \
    -o build_branch/branch_node_simple \
    build_branch/branch_node.o \
    build_branch/branch_node_simple.o

echo ""
echo "✅ 编译成功!"
echo ""
echo "运行示例:"
echo "./build_branch/branch_node_simple"
echo ""
