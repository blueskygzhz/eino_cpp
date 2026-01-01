#!/bin/bash

# 编译完整的 Graph + BranchNode 示例
# 使用 GCC 12.2.0 支持 C++17

set -e

echo "========================================"
echo "  编译 Graph + BranchNode 完整示例"
echo "========================================"

# 创建构建目录
BUILD_DIR="build_complete_graph"
mkdir -p $BUILD_DIR

# 需要编译的源文件列表
SRC_FILES=(
    "src/compose/branch_node.cpp"
    "src/compose/graph.cpp"
    "src/compose/graph_run.cpp"
    "src/compose/graph_manager.cpp"
    "src/compose/graph_node.cpp"
    "src/compose/graph_addedge.cpp"
    "src/compose/graph_validation.cpp"
    "src/compose/dag.cpp"
    "src/compose/generic_graph.cpp"
    "src/compose/generic_helper.cpp"
    "src/compose/runnable.cpp"
    "src/compose/type_registry.cpp"
    "src/compose/utils.cpp"
    "src/compose/value_merge.cpp"
    "src/compose/values_merge.cpp"
    "src/compose/state_handler.cpp"
    "src/adk/context.cpp"
)

# 编译选项
CXX="g++-12"
CXXFLAGS="-std=c++17 -I./include -I./third_party -O2"

# 编译所有依赖的源文件
echo ""
echo "[步骤 1] 编译依赖文件..."
OBJECT_FILES=()

for src in "${SRC_FILES[@]}"; do
    if [ -f "$src" ]; then
        obj_name=$(basename "$src" .cpp).o
        obj_path="$BUILD_DIR/$obj_name"
        echo "  编译: $src → $obj_path"
        $CXX -c $CXXFLAGS -o "$obj_path" "$src"
        OBJECT_FILES+=("$obj_path")
    else
        echo "  ⚠️  警告: 文件不存在，跳过: $src"
    fi
done

# 编译示例主文件
echo ""
echo "[步骤 2] 编译示例主文件..."
EXAMPLE_SRC="examples/complete_graph_branch_example.cpp"
EXAMPLE_OBJ="$BUILD_DIR/complete_graph_branch_example.o"

if [ -f "$EXAMPLE_SRC" ]; then
    echo "  编译: $EXAMPLE_SRC → $EXAMPLE_OBJ"
    $CXX -c $CXXFLAGS -o "$EXAMPLE_OBJ" "$EXAMPLE_SRC"
    OBJECT_FILES+=("$EXAMPLE_OBJ")
else
    echo "❌ 错误: 示例文件不存在: $EXAMPLE_SRC"
    exit 1
fi

# 链接生成可执行文件
echo ""
echo "[步骤 3] 链接生成可执行文件..."
OUTPUT="$BUILD_DIR/complete_graph_branch_example"
echo "  链接: ${OBJECT_FILES[@]} → $OUTPUT"
$CXX $CXXFLAGS -o "$OUTPUT" "${OBJECT_FILES[@]}"

echo ""
echo "========================================"
echo "  ✅ 编译成功!"
echo "========================================"
echo ""
echo "可执行文件: $OUTPUT"
echo ""
echo "运行示例:"
echo "  ./$OUTPUT"
echo ""
