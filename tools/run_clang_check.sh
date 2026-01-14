#!/bin/bash
# clang 代码规范检查脚本

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "=========================================="
echo "Clang 代码规范检查"
echo "=========================================="
echo ""

# 查找所有需要检查的源文件（排除 lib 和 build 目录）
echo "正在查找源文件..."
FILES=$(find . -type f \( -name "*.c" -o -name "*.h" \) \
    ! -path "./lib/*" \
    ! -path "./build/*" \
    ! -path "./.git/*" \
    | sort)

if [ -z "$FILES" ]; then
    echo "未找到需要检查的源文件"
    exit 1
fi

echo "找到 $(echo "$FILES" | wc -l) 个文件需要检查"
echo ""

# 创建临时目录存储检查结果
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

FORMAT_ERRORS="$TEMP_DIR/format_errors.txt"
TIDY_ERRORS="$TEMP_DIR/tidy_errors.txt"

# 1. 检查代码格式 (clang-format)
echo "=========================================="
echo "1. 检查代码格式 (clang-format)"
echo "=========================================="
FORMAT_FAILED=0
for file in $FILES; do
    if clang-format --dry-run --Werror "$file" > /dev/null 2>&1; then
        echo "✓ $file"
    else
        echo "✗ $file (格式不符合规范)"
        echo "$file" >> "$FORMAT_ERRORS"
        FORMAT_FAILED=1
    fi
done

if [ $FORMAT_FAILED -eq 0 ]; then
    echo ""
    echo "✓ 所有文件格式检查通过"
else
    echo ""
    echo "✗ 发现格式问题，文件列表："
    cat "$FORMAT_ERRORS"
fi

echo ""

# 2. 检查代码质量 (clang-tidy)
echo "=========================================="
echo "2. 检查代码质量 (clang-tidy)"
echo "=========================================="

# 准备编译数据库参数
INCLUDE_DIRS="-I./lib/civetweb/include -I./lib/cJSON"
COMPILE_FLAGS="-std=c99 -D_GNU_SOURCE"

TIDY_FAILED=0
for file in $FILES; do
    echo "检查: $file"
    if clang-tidy "$file" -- $INCLUDE_DIRS $COMPILE_FLAGS > "$TEMP_DIR/tidy_${file//\//_}.txt" 2>&1; then
        # 只检查用户代码中的实际警告（包含项目路径的行）
        # 排除：_GNU_SOURCE 警告、统计信息行（"warnings generated"、"Suppressed"）
        if grep -E "^$PROJECT_ROOT.*: (error|warning):" "$TEMP_DIR/tidy_${file//\//_}.txt" 2>/dev/null | grep -qvE "_GNU_SOURCE"; then
            echo "  ✗ 发现问题"
            grep -E "^$PROJECT_ROOT.*: (error|warning):" "$TEMP_DIR/tidy_${file//\//_}.txt" | grep -vE "_GNU_SOURCE" >> "$TIDY_ERRORS"
            TIDY_FAILED=1
        else
            echo "  ✓ 通过"
        fi
    else
        echo "  ✗ 检查失败"
        grep -E "^$PROJECT_ROOT.*: (error|warning):" "$TEMP_DIR/tidy_${file//\//_}.txt" | grep -vE "_GNU_SOURCE" >> "$TIDY_ERRORS"
        TIDY_FAILED=1
    fi
done

if [ $TIDY_FAILED -eq 0 ]; then
    echo ""
    echo "✓ 所有文件代码质量检查通过"
else
    echo ""
    echo "✗ 发现代码质量问题："
    cat "$TIDY_ERRORS"
fi

echo ""
echo "=========================================="
echo "检查总结"
echo "=========================================="
if [ $FORMAT_FAILED -eq 0 ] && [ $TIDY_FAILED -eq 0 ]; then
    echo "✓ 所有检查通过！"
    exit 0
else
    echo "✗ 发现问题："
    [ $FORMAT_FAILED -ne 0 ] && echo "  - 代码格式问题"
    [ $TIDY_FAILED -ne 0 ] && echo "  - 代码质量问题"
    echo ""
    echo "详细错误信息已保存到临时文件"
    exit 1
fi

