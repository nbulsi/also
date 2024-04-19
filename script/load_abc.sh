#!/bin/bash

# 设置 abc 命令路径
ABC_CMD=${ABC_PATH:-abc}

# 设置输入和输出目录路径
INPUT_FILE="/home/ym/npn1.txt"
OUTPUT_DIR="/home/ym/also/truth_abc"

# 创建输出目录（如果不存在）
mkdir -p "$OUTPUT_DIR"

# 设定日志文件路径
LOG_FILE="$OUTPUT_DIR/npn.log"

# 清空或创建日志文件
> "$LOG_FILE"

# 逐行遍历文件并执行命令
while IFS= read -r line || [[ -n "$line" ]]; do
    # 如果行为空，则跳过
    if [ -z "$line" ]; then
        continue
    fi

    # 去除前两个字符
    input="${line:2}"

    # 生成输出文件名，将特殊字符替换为下划线
    output_file="$OUTPUT_DIR/$(echo "$input" | tr -cd '[:alnum:]._-').v"

    echo "真值表：$line" >> "$LOG_FILE"

    # 执行命令，加载当前行的真值表并进行处理，将输出重定向到日志文件
    $ABC_CMD -c "read_truth $input; write_verilog $output_file" >> "$LOG_FILE" 2>&1
done < "$INPUT_FILE"
