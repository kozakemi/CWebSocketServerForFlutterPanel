#!/bin/bash
# 生成详细的 clang 检查报告

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

REPORT_FILE="clang_check_report.txt"
echo "Clang 代码规范检查报告" > "$REPORT_FILE"
echo "生成时间: $(date)" >> "$REPORT_FILE"
echo "==========================================" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

# 查找所有需要检查的源文件
FILES=$(find . -type f \( -name "*.c" -o -name "*.h" \) \
    ! -path "./lib/*" \
    ! -path "./build/*" \
    ! -path "./.git/*" \
    | sort)

INCLUDE_DIRS="-I./lib/civetweb/include -I./lib/cJSON"
COMPILE_FLAGS="-std=c99 -D_GNU_SOURCE"

echo "正在生成详细报告..."
echo ""

# 检查格式问题
echo "==========================================" >> "$REPORT_FILE"
echo "1. 代码格式检查 (clang-format)" >> "$REPORT_FILE"
echo "==========================================" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

FORMAT_COUNT=0
for file in $FILES; do
    if ! clang-format --dry-run --Werror "$file" > /dev/null 2>&1; then
        FORMAT_COUNT=$((FORMAT_COUNT + 1))
        echo "[格式问题] $file" >> "$REPORT_FILE"
        echo "建议运行: clang-format -i $file" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    fi
done

if [ $FORMAT_COUNT -eq 0 ]; then
    echo "✓ 所有文件格式检查通过" >> "$REPORT_FILE"
else
    echo "✗ 发现 $FORMAT_COUNT 个文件存在格式问题" >> "$REPORT_FILE"
fi

echo "" >> "$REPORT_FILE"
echo "==========================================" >> "$REPORT_FILE"
echo "2. 代码质量检查 (clang-tidy)" >> "$REPORT_FILE"
echo "==========================================" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"

TIDY_COUNT=0
for file in $FILES; do
    OUTPUT=$(clang-tidy "$file" -- $INCLUDE_DIRS $COMPILE_FLAGS 2>&1)
    # 只检查用户代码中的实际警告，排除 _GNU_SOURCE 和系统警告
    FILTERED_OUTPUT=$(echo "$OUTPUT" | grep -E "^$PROJECT_ROOT.*: (error|warning):" | grep -vE "_GNU_SOURCE" || true)
    if [ -n "$FILTERED_OUTPUT" ]; then
        TIDY_COUNT=$((TIDY_COUNT + 1))
        echo "[质量问题] $file" >> "$REPORT_FILE"
        echo "$FILTERED_OUTPUT" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    fi
done

if [ $TIDY_COUNT -eq 0 ]; then
    echo "✓ 所有文件代码质量检查通过" >> "$REPORT_FILE"
else
    echo "✗ 发现 $TIDY_COUNT 个文件存在代码质量问题" >> "$REPORT_FILE"
fi

echo "" >> "$REPORT_FILE"
echo "==========================================" >> "$REPORT_FILE"
echo "总结" >> "$REPORT_FILE"
echo "==========================================" >> "$REPORT_FILE"
echo "格式问题: $FORMAT_COUNT 个文件" >> "$REPORT_FILE"
echo "质量问题: $TIDY_COUNT 个文件" >> "$REPORT_FILE"
echo "" >> "$REPORT_FILE"
echo "详细报告已保存到: $REPORT_FILE"

cat "$REPORT_FILE"

