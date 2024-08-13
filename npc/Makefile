TOPNAME = top
NXDC_FILES = constr/top.nxdc
INC_PATH ?=

VERILATOR = verilator
VERILATOR_CFLAGS += --MMD --build -cc \
					-O3 --x-assign fast --x-initial fast --noassert \
					--trace

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)


# constraint file
SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

# project source
VSRCS = $(shell find $(abspath ./) -name "*.v")
CSRCS = $(shell find $(abspath ./) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
CSRCS += $(SRC_AUTO_BIND)

# rules for NVBoard
include $(NVBOARD_HOME)/scripts/nvboard.mk    

# rules for verilator
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

#传递给preproject连接器的信息，添加SDL2库信息
#LDFLAGS += -lSDL2 -lSDL2_image


$(shell mkdir -p $(BUILD_DIR))
default: $(BIN)

all: default

$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) \
		--exe -o $(abspath $(BIN))

run: $(BIN)
	@$^

wave: $(BIN)
	@gtkwave $(BUILD_DIR)/wave.vcd

.PHONY: default all clean run
clean:
	rm -rf $(BUILD_DIR) 

