#!/bin/bash

# Graph Stream Branch Example Build Script
# 编译并运行Graph流式分支路由示例

set -e  # 遇到错误立即退出

echo "========================================================================"
echo "  Building Graph Stream Branch Example"
echo "========================================================================"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Project root: $PROJECT_ROOT"
echo "Script directory: $SCRIPT_DIR"

# 设置编译选项
BUILD_DIR="$SCRIPT_DIR/build_stream_branch"
SOURCE_FILE="$SCRIPT_DIR/graph_stream_branch_example.cpp"
EXECUTABLE="$BUILD_DIR/graph_stream_branch_example"

# 包含路径
INCLUDE_DIRS="-I$PROJECT_ROOT/include"

# 编译标志
CXX_FLAGS="-std=c++11 -Wall -Wextra -pthread"

# 创建构建目录
echo ""
echo "Creating build directory..."
mkdir -p "$BUILD_DIR"

# 编译
echo ""
echo -e "${YELLOW}Compiling...${NC}"
echo "Command: g++ $CXX_FLAGS $INCLUDE_DIRS $SOURCE_FILE -o $EXECUTABLE"

if g++ $CXX_FLAGS $INCLUDE_DIRS "$SOURCE_FILE" -o "$EXECUTABLE"; then
    echo -e "${GREEN}✅ Compilation successful!${NC}"
else
    echo -e "${RED}❌ Compilation failed!${NC}"
    exit 1
fi

# 运行
echo ""
echo "========================================================================"
echo "  Running Example"
echo "========================================================================"
echo ""

if [ -f "$EXECUTABLE" ]; then
    "$EXECUTABLE"
    EXIT_CODE=$?
    
    echo ""
    echo "========================================================================"
    if [ $EXIT_CODE -eq 0 ]; then
        echo -e "${GREEN}✅ Example completed successfully!${NC}"
    else
        echo -e "${RED}❌ Example failed with exit code: $EXIT_CODE${NC}"
    fi
    echo "========================================================================"
    
    exit $EXIT_CODE
else
    echo -e "${RED}❌ Executable not found: $EXECUTABLE${NC}"
    exit 1
fi
