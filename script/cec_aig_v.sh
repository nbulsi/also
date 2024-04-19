#!/bin/bash

# 设置 abc 命令路径
ABC_CMD=${ABC_PATH:-abc}

# 设置输入文件夹路径
INPUT_DIR_AIGER="/home/ym/also/benchmark"
INPUT_DIR_VERILOG="/home/ym/also/paper1"

# 生成输出文件名
OUTPUT_FILE="/home/ym/also/paper1/cec_rm3.txt"

# 清空或创建日志文件
> "$OUTPUT_FILE"

# 遍历 Verilog 文件夹中的所有文件
for input_verilog in "$INPUT_DIR_VERILOG"/*.v; do
    # 检查文件类型是否为 .v
    if [[ "${input_verilog##*.}" != "v" ]]; then
        # 跳过非 .v 文件
        continue
    fi
    
    # 获取 Verilog 文件的基本文件名
    verilog_filename=$(basename "$input_verilog")
    
    # 构建对应的 AIGER 文件路径
    input_aiger="$INPUT_DIR_AIGER/${verilog_filename%.v}.aig"
    
    # 检查对应的 AIGER 文件是否存在
    if [ -e "$input_aiger" ]; then
        # 使用 abc 命令检查等价性
        result=$($ABC_CMD -c "cec -n $input_verilog $input_aiger")
        
        # 检查输出结果是否包含 "Networks are equivalent" 字符串
        if grep -q "Networks are equivalent" <<< "$result"; then
            echo "$verilog_filename 和 ${verilog_filename%.v}.aig 等价" >> "$OUTPUT_FILE"
        else
            echo "$verilog_filename 和 ${verilog_filename%.v}.aig 不等价" >> "$OUTPUT_FILE"
        fi

        # 输出输出结果到文件
        echo "$result" >> "$OUTPUT_FILE"
    else
        echo "未找到对应的 AIGER 文件: ${verilog_filename%.v}.aig" >> "$OUTPUT_FILE"
    fi
done
