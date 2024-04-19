#!/bin/bash

# 设置 abc 命令路径
ALSO_CMD=${ALSO_PATH:-also}

# 设置输入和输出目录路径
INPUT_DIR="/home/ym/also/benchmark"
OUTPUT_DIR="/home/ym/also/exact_m3ig"

# 创建输出目录（如果不存在）
mkdir -p "$OUTPUT_DIR"

LOG_FILE="/home/ym/also/exact_m3ig/optimization_mig.log"

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
    output_file="$OUTPUT_DIR/$(basename "${aig_file%.aig}").v"
    
    # 读取输入文件，并进行优化
    $ALSO_CMD -c "read_aiger $aig_file; ps -a; lut_mapping; ps -l; lut_resyn -n; ps -m; write_verilog -m $output_file">> "$LOG_FILE" 2>&1

    # 输出完成信息
    echo "优化完成。输入文件路径：$aig_file，输出文件路径：$output_file">> "$LOG_FILE"
done
