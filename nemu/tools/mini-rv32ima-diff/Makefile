# 定义编译器
CC = gcc

# 定义编译选项
CFLAGS = -O2 -Wall -fPIC

#-fvisibility=hidden
# 定义链接选项
LDFLAGS = -shared

BUILD_DIR = ./build
$(shell mkdir -p $(BUILD_DIR))

# 定义库文件名
LIBRARY = $(BUILD_DIR)/riscv32-miniRV32ima.so

# 定义源文件和目标文件
SRC = difftest.c
OBJ = $(SRC:.c=.o)


# 默认目标
all: $(LIBRARY)

# 规则：编译动态库
$(LIBRARY): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# 规则：编译C源文件为.o目标文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR) *.o

.PHONY: all clean
