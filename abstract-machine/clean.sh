#!/bin/bash

# 警告信息，提醒用户操作的危险性
echo "Warning: This script will delete all found directories named 'build'."
read -p "Are you sure you want to continue? (y/n): " -n 1 -r
echo    # (optional) 在输入后移动到新行

if [[ $REPLY =~ ^[Yy]$ ]]
then
    # 查找当前目录及子目录下所有名为 'build' 的目录
    build_dirs=$(find . -type d -name 'build')

    # 列出找到的 'build' 目录
    echo "Find the following directory named 'build':"
    echo "$build_dirs"

    # 删除这些 'build' 目录
    echo "Deleting these directories, please wait..."
    for dir in $build_dirs
    do
        # 使用红色文本打印删除信息
        printf "\033[0;31mDeleted: %s\033[0m\n" "$dir"
        # 执行删除操作
        rm -rf "$dir"
    done

    echo "All 'build' directories have been deleted."
else
    echo "The operation has been canceled."
fi
