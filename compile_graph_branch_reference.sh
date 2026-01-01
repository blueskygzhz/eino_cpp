#!/bin/bash

# 编译 Graph with BranchNode Reference 示例

set -e

echo "=========================================="
echo "  编译 Graph + BranchNode 引用示例"
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

echo "[2/3] 编译 graph_with_branch_reference.cpp..."
g++-12 -c -std=c++17 \
    -I./include \
    -I./third_party \
    -o build_branch/graph_with_branch_reference.o \
    examples/graph_with_branch_reference.cpp

echo "[3/3] 链接生成可执行文件..."
g++-12 -std=c++17 \
    -o build_branch/graph_with_branch_reference \
    build_branch/branch_node.o \
    build_branch/graph_with_branch_reference.o

echo ""
echo "✅ 编译成功!"
echo ""
echo "运行示例:"
echo "./build_branch/graph_with_branch_reference"
echo ""
