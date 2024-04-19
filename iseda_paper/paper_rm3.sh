#!/bin/bash

# 设置 abc 命令路径
ALSO_CMD=${ALSO_PATH:-also}

# 设置输入和输出目录路径
INPUT_DIR="/home/ym/also/benchmark"
OUTPUT_DIR="/home/ym/also/paper1"

# 创建输出目录（如果不存在）
mkdir -p "$OUTPUT_DIR"

LOG_FILE="$OUTPUT_DIR/rm3_inv1.log"

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
    
    # 记录开始时间
    start_time=$(date +%s.%N)

    # 读取输入文件，并进行优化
    $ALSO_CMD -c "read_aiger $aig_file; ps -a; exact_map -r; exact_map --rm3_to_rm3; exact_map --rm3_to_rm3; exact_map --rm3_to_rm3; exact_map --rm3_to_rm3; rm3iginv; rm3iginv; rm3iginv; rm3igcost; write_verilog -y $output_file">> "$LOG_FILE" 2>&1

    # 记录结束时间
    end_time=$(date +%s.%N)

    # 计算处理时间
    elapsed_time=$(echo "$end_time - $start_time" | bc)

    # 输出完成信息和处理时间
    echo "优化完成。输入文件路径：$aig_file，输出文件路径：$output_file，处理时间：$elapsed_time 秒" >> "$LOG_FILE"
done
