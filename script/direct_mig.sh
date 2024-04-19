#!/bin/bash

# 设置 abc 命令路径
ALSO_CMD=${ALSO_PATH:-also}

# 设置输入和输出目录路径
INPUT_DIR="/home/ym/also/mig_rm3_inv_opt"
OUTPUT_DIR="/home/ym/also/mig_rm3_rm3_inv"

# 创建输出目录（如果不存在）
mkdir -p "$OUTPUT_DIR"

LOG_FILE="/home/ym/also/mig_rm3_rm3_inv/rm3.log"

# 清空或创建日志文件
> "$LOG_FILE"


# 处理每个 AIG 文件
for verilog_file in "$INPUT_DIR"/*; do
    # 检查文件类型是否为 AIG
    if [[ "${verilog_file##*.}" != "v" ]]; then
        # 跳过非 AIG 文件
        continue
    fi

    # 生成输出文件名
    output_file="$OUTPUT_DIR/$(basename "${verilog_file%.v}").v"

    
    # 读取输入文件，并进行优化
    $ALSO_CMD -c "read_verilog -m $verilog_file; ps -m; exact_map -y; ps -y; rm3iginv; rm3iginv; rm3igcost; write_verilog -y $output_file">> "$LOG_FILE" 2>&1

    # 输出完成信息
    echo "优化完成。输入文件路径：$verilog_file">> "$LOG_FILE"
done
