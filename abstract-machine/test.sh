#!/bin/bash

# 创建输出文件
output_file="output.txt"
echo "Combined Makefile contents:" > "$output_file"

# 文件列表
files=(
  "./am/Makefile"
  "./klib/Makefile"
  "./Makefile"
  "./scripts/isa/riscv.mk"
  "./scripts/native.mk"
  "./scripts/platform/nemu.mk"
  "./scripts/riscv32-nemu.mk"
)

# 循环遍历文件列表
for file in "${files[@]}"; do
  # 检查文件是否存在
  if [[ -f "$file" ]]; then
    # 将文件路径和名称添加到输出文件
    echo "========== $file ==========" >> "$output_file"
    # 打印文件内容到输出文件
    cat "$file" >> "$output_file"
    # 添加分隔行，以便区分不同文件的内容
    echo "" >> "$output_file"
  else
    echo "File not found: $file"
  fi
done
