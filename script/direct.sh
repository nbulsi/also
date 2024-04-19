#!/bin/bash

# 设置 abc 命令路径
ALSO_CMD=${ALSO_PATH:-also}

# 设置输入和输出目录路径
INPUT_DIR="/home/ym/also/benchmark"
OUTPUT_DIR="/home/ym/also/direct_rm3"

# 创建输出目录（如果不存在）
mkdir -p "$OUTPUT_DIR"

LOG_FILE="/home/ym/also/direct_rm3/xmg_to_rm3.log"


# 处理每个 AIG 文件
for aiger_file in "$INPUT_DIR"/*; do
    # 检查文件类型是否为 AIG
    if [[ "${aiger_file##*.}" != "aig" ]]; then
        # 跳过非 AIG 文件
        continue
    fi

    # 生成输出文件名
    output_file="$OUTPUT_DIR/$(basename "${verilog_file%.v}").v"
    
    # 读取输入文件，并进行优化
    $ALSO_CMD -c "read_aiger $aiger_file; ps -a; exact_map; xmgrw -a; ps -x; lut_mapping -x; ps -l; lut_resyn -y -e -n; ps -y; rm3iginv">> "$LOG_FILE" 2>&1

    # 输出完成信息
    echo "优化完成。输入文件路径：$verilog_file，输出文件路径：$output_file">> "$LOG_FILE"
done
