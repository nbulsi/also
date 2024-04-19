#!/bin/bash

# 设置 abc 命令路径
ALSO_CMD=${ALSO_PATH:-also}

# 获取用户输入的 AIG 文件路径
read -p "请输入 AIG 文件的路径: " AIG_FILE

# 确认输入文件存在
if [ ! -f "$AIG_FILE" ]; then
    echo "错误：指定的文件不存在。请重新运行脚本并提供有效的文件路径。"
    exit 1
fi

LOG_FILE="/home/ym/also/cut_tt_npn/tt_npn_square.log"

# 清空或创建日志文件
> "$LOG_FILE"

# 读取输入文件，并进行优化
$ALSO_CMD -c "read_aiger $AIG_FILE; ps -a; exact_map -r" >> "$LOG_FILE" 2>&1

# 输出完成信息
echo "优化完成。输入文件路径：$AIG_FILE" >> "$LOG_FILE"
