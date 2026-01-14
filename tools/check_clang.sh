#!/bin/bash
# clang 代码检查脚本

# 检查工具是否已安装
check_tool() {
    if command -v $1 &> /dev/null; then
        echo "✓ $1 已安装: $(which $1)"
        $1 --version | head -1
        return 0
    else
        echo "✗ $1 未安装"
        return 1
    fi
}

echo "=== 检查 clang 工具 ==="
check_tool clang-format
check_tool clang-tidy

echo ""
echo "=== 如果未安装，请运行以下命令安装 ==="
echo "sudo apt-get update"
echo "sudo apt-get install -y clang-format clang-tidy"
echo ""
echo "或者安装完整 LLVM 工具链："
echo "sudo apt-get install -y clang-format clang-tidy clang"

