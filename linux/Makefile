### Create the destination directory (`build/$ARCH`)
WORK_DIR  = $(shell pwd)
DST_DIR   = $(WORK_DIR)/build/$(ARCH)
BUILD_DIR = $(WORK_DIR)/build
$(shell mkdir -p $(DST_DIR))

### Compilation targets (a binary image or archive)
IMAGE_REL = build/$(NAME)-$(ARCH)
IMAGE     = $(abspath $(IMAGE_REL))
ARCHIVE   = $(WORK_DIR)/build/$(NAME)-$(ARCH).a

ISA = riscv32
DTB_FILE = $(NEMU_HOME)/src/isa/riscv32/dts/nemu.dtb
IMAGE = $(NEMU_HOME)/../linux/buildroot/output/images/Image


#NEMUFLAGS += -l $(shell dirname $(IMAGE).elf)/nemu-log.txt
#NEMUFLAGS += -b
NEMUFLAGS += -t $(DTB_FILE)

buildroot:
	git clone https://github.com/cnlohr/buildroot --recurse-submodules --depth 1

toolchain: buildroot
	cp -a configs/custom_kernel_config buildroot/kernel_config
	cp -a configs/buildroot_config buildroot/.config
	cp -a configs/busybox_config buildroot/busybox_config
	cp -a configs/uclibc_config buildroot/uclibc_config
	cp -a configs/uclibc_config buildroot/uclibc_config_extra
	true || cp -a configs/rootfsoverlay/* buildroot/output/target/
	$(MAKE) -C buildroot

app: toolchain
	make -C hello_linux deploy
	#make -C packages/duktapetest deploy
	make -C packages/coremark deploy
	cp -a configs/rootfsoverlay/* buildroot/output/target/
	$(MAKE) -C buildroot

$(DTB_FILE):
	$(MAKE) -C $(NEMU_HOME)/src/isa/riscv32/dts dts

run: app $(DTB_FILE)
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="$(NEMUFLAGS)" IMG=$(IMAGE)

gdb: app $(DTB_FILE)
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) gdb ARGS="$(NEMUFLAGS)" IMG=$(IMAGE)


clean:
	rm -rf ./build

.PHONY: run gdb app clean
