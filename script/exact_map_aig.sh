#!/bin/bash

# 设置 abc 命令路径
ALSO_CMD=${ALSO_PATH:-also}

# 设置输入和输出目录路径
INPUT_DIR="/home/ym/also/techmap/abc_delay_map"
OUTPUT_DIR="/home/ym/also/techmap/exact_map_aig"

# 创建输出目录（如果不存在）
mkdir -p "$OUTPUT_DIR"

LOG_FILE="/home/ym/also/techmap/exact_map_aig/abc_delay_exact_aig.log"

# 清空或创建日志文件
> "$LOG_FILE"


# 处理每个 AIG 文件
for aig_file in "$INPUT_DIR"/*; do
    # 检查文件类型是否为 AIG
    if [[ "${aig_file##*.}" != "aig" ]]; then
        # 跳过非 AIG 文件
        continue
    fi

    # 生成输出文件名
    output_file="$OUTPUT_DIR/$(basename "${aig_file%.aig}").aig"

    
    # 读取输入文件，并进行优化
    $ALSO_CMD -c "read_aiger $aig_file; ps -a; exact_map -a; xmgrs; xmgrw; xmgrw; exact_map -x; write_aiger $output_file">> "$LOG_FILE" 2>&1

    # 输出完成信息
    echo "优化完成。输入文件路径：$aig_file">> "$LOG_FILE"
done
