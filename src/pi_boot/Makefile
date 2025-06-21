MAKEFLAGS += --no-print-directory

SRC_DIR=src
BIN_DIR=bin
# BOOTCSRCS=$(shell find bootloader/$(SRC_DIR)/*.c 2>/dev/null)
# BOOTASMSRCS=$(shell find bootloader/$(SRC_DIR)/*.s 2>/dev/null)
# SCHEDCSRCS=$(shell find scheduler/$(SRC_DIR)/*.c 2>/dev/null)
# SCHEDASMSRCS=$(shell find scheduler/$(SRC_DIR)/*.s 2>/dev/null)
# SRCS:=$(SCHEDCSRCS) $(SCHEDASMSRCS) $(BOOTCSRCS) $(BOOTASMSRCS)
# TOBJS:=$(patsubst bootloader/$(SRC_DIR)/%, $(BIN_DIR)/%, $(TOBJS))
# OBJS=$(patsubst scheduler/$(SRC_DIR)/%, $(BIN_DIR)/%, $(TOBJS))

SRCS=$(wildcard $(SRC_DIR)/*.s) $(wildcard $(SRC_DIR)/*.c)
TOBJS=$(patsubst %.c, %.o, $(SRCS))
TOBJS:=$(patsubst %.s, %_s.o, $(TOBJS))
TOBJS:=$(patsubst %_start_s.o, %_start.o, $(TOBJS))

FONT_SRCS=$(shell find fonts/*)
FONTOBJS=$(patsubst %.sfn, %_sfn.o, $(FONT_SRCS))
FONTOBJS:=$(patsubst %.psf, %_psf.o, $(FONTOBJS))
FONTOBJS:=$(patsubst fonts/%, $(BIN_DIR)/%, $(FONTOBJS))

OBJS=$(FONTOBJS) $(patsubst $(SRC_DIR)/%, $(BIN_DIR)/%, $(TOBJS))

INCLUDES= -I include/
LINKERFILE=linker.ld

CC=				aarch64-none-elf-gcc
LINKER=			aarch64-none-elf-ld
OPT=			-O3
AARCHFLAGS=		-mcmodel=large
CFLAGS=			-g -std=c11 -nostdlib -Wno-builtin-declaration-mismatch -nodefaultlibs -nostartfiles $(AARCHFLAGS)
TARGET=			exe
LFLAGS=			 
IMG=			kernel.img
SD_IMG=			rpos.img

all: $(BIN_DIR) $(TARGET) $(IMG) $(SD_IMG)

re: clean all

.PHONY: debug
debug: $(DEBUG_BIN_DIR) $(DTARGET)

$(IMG): $(TARGET)
	aarch64-none-elf-objcopy -O binary $^ $@

# @ is the rule, ^ is the prereqs
$(TARGET): $(OBJS)
	@echo $(OBJS)
	$(LINKER) -o $@ $^ -T $(LINKERFILE) $(LFLAGS) --entry=0x80000000

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(BIN_DIR)/%_s.o: $(SRC_DIR)/%.s
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

$(BIN_DIR)/_start.o: $(SRC_DIR)/_start.s
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

$(BIN_DIR)/%_psf.o: fonts/%.psf
	aarch64-none-elf-ld -r -b binary -o $@ $^
$(BIN_DIR)/%_sfn.o: fonts/%.sfn
	aarch64-none-elf-ld -r -b binary -o $@ $^

$(BIN_DIR):
	mkdir -p $@

.PHONY: clean
clean:
	$(RM) -rf $(TARGET)
	$(RM) -rf **/$(BIN_DIR)
	$(RM) -rf $(BIN_DIR)
	$(RM) -rf objdump.s
	$(RM) -rf *.txt
	$(RM) -rf kernel.img

.PHONY: asm
asm: all
	aarch64-none-elf-objdump -d $(TARGET) > objdump.s

run: all
	qemu-system-aarch64 -M raspi3b -kernel $(IMG) -drive file=/dev/disk4,if=sd,format=raw -serial stdio

elf: all
	aarch64-none-elf-readelf -a $(TARGET) > elf.txt

.PHONY: gdb
gdb: all
	aarch64-none-elf-gdb $(TARGET) -ex "target remote localhost:1234"

wait: all
	qemu-system-aarch64 -machine raspi3b -kernel $(IMG) -drive file=/dev/disk4,if=sd,format=raw -serial stdio -S -s 

$(SD_IMG):
	# create a 1 GiB virtual SD card image
	dd if=/dev/zero of=$@ bs=1M count=1024
	hdiutil attach -nomount $@
	diskutil eraseDisk FAT32 SDCARD MBRFormat /dev/disk4
	diskutil partitionDisk /dev/disk4 MBR FAT32 one R FAT32 two 250MB 
	diskutil unmountDisk /dev/disk4

.PHONY: mount
mount: $(SD_IMG)
	mount -t msdos /dev/disk4s1 sdcard/

.PHONY: unmount
unmount: 
	umount /dev/disk4s1

boot: $(BIN_DIR) $(IMG)
	$(MAKE) mount
	cp $(IMG) sdcard/
	echo "" > sdcard/start.elf
	echo "" > sdcard/config.txt
	$(MAKE) unmount
