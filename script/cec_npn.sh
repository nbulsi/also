#!/bin/bash

# 设置 abc 命令路径
ABC_CMD=${ABC_PATH:-abc}

# 设置输入文件夹路径
INPUT_DIR1="/home/ym/also/truth"
INPUT_DIR2="/home/ym/also/truth_abc"

# 生成输出文件名
OUTPUT_FILE="/home/ym/also/cec_abc_npn.txt"

# 清空或创建日志文件
> "$OUTPUT_FILE"

# 遍历输入文件夹1中的所有.v文件
for input_file1 in "$INPUT_DIR1"/*.v; do
    # 检查文件类型是否为 .v
    if [[ "${input_file1##*.}" != "v" ]]; then
        # 跳过非 .v 文件
        continue
    fi
    
    # 获取第二个文件路径
    input_file2=$(echo "$input_file1" | sed "s|$INPUT_DIR1|$INPUT_DIR2|; s|\.v|.v|")
    
    # 使用 abc 命令检查等价性
    result=$($ABC_CMD -c "cec -n $input_file1 $input_file2")
    
    # 检查输出结果是否包含 "Networks are equivalent" 字符串
    if grep -q "Networks are equivalent" <<< "$result"; then
        echo "$(basename "$input_file1") 和 $(basename "$input_file2") 等价" >> $OUTPUT_FILE
    else
        echo "$(basename "$input_file1") 和 $(basename "$input_file2") 不等价" >> $OUTPUT_FILE
    fi
    
    # 输出输出结果到文件
    echo "$result" >> $OUTPUT_FILE
done
