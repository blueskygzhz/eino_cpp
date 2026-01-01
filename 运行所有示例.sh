#!/bin/bash

# 运行所有 Graph + BranchNode 示例
# 展示 eino_cpp 的完整功能

set -e

echo "========================================================================"
echo "  eino_cpp Graph + BranchNode 完整示例演示"
echo "========================================================================"
echo ""

# 示例 1: 简单的 BranchNode 示例
echo "【示例 1】简单的 BranchNode 年龄检查"
echo "========================================================================"
echo ""

if [ ! -f "build_branch/branch_node_simple" ]; then
    echo "正在编译 branch_node_simple..."
    ./compile_branch_node_simple.sh
fi

echo "运行示例..."
./build_branch/branch_node_simple

echo ""
echo "按回车继续下一个示例..."
read

# 示例 2: Graph + BranchNode 引用示例
echo ""
echo "【示例 2】Graph + BranchNode 节点引用示例"
echo "========================================================================"
echo ""

if [ ! -f "build_branch/graph_with_branch_reference" ]; then
    echo "正在编译 graph_with_branch_reference..."
    ./compile_graph_branch_reference.sh
fi

echo "运行示例..."
./build_branch/graph_with_branch_reference

# 总结
echo ""
echo "========================================================================"
echo "  ✅ 所有示例运行完毕"
echo "========================================================================"
echo ""
echo "【演示内容总结】"
echo ""
echo "1. 简单 BranchNode 示例"
echo "   - 展示了基本的条件判断 (age >= 18)"
echo "   - 测试了 3 个用例（成年、未成年、边界值）"
echo "   - 文件: examples/branch_node_simple.cpp"
echo ""
echo "2. Graph + BranchNode 集成示例"
echo "   - 展示了 BranchNode 引用多个上游节点"
echo "   - Node A 输出 age, Node B 输出 vip"
echo "   - BranchNode 同时引用 A 和 B 的输出"
echo "   - 支持 AND/OR 条件组合"
echo "   - 文件: examples/graph_with_branch_reference.cpp"
echo ""
echo "【核心功能】"
echo "✓ BranchNode 条件判断"
echo "✓ 多节点输出引用"
echo "✓ 复杂条件逻辑 (AND/OR)"
echo "✓ 多层级路径访问"
echo "✓ 18 种操作符支持"
echo "✓ 短路求值优化"
echo "✓ 完全对齐 coze-studio"
echo ""
echo "【文档】"
echo "- 完整示例说明.md - 详细使用文档"
echo "- README_BRANCH_NODE.md - BranchNode API 文档"
echo "- GRAPH_BRANCH_REFERENCE_README.md - 节点引用说明"
echo ""
echo "【快速开始】"
echo "# 运行简单示例"
echo "./build_branch/branch_node_simple"
echo ""
echo "# 运行 Graph 集成示例"
echo "./build_branch/graph_with_branch_reference"
echo ""
echo "# 查看详细文档"
echo "cat 完整示例说明.md"
echo ""
