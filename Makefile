BIN_DIR := bin
SRC_DIR := src/kernel
SDCARD_DIR := sdcard
PROD_DIR := prod
C_SRCS = $(shell find $(SRC_DIR) -type f -name '*.c') 
ASM_SRCS = $(shell find $(SRC_DIR) -type f -name '*.S')
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(C_SRCS)) $(patsubst $(SRC_DIR)/%.S, $(BIN_DIR)/%_s.o, $(ASM_SRCS))

TARGET := exe
KERNEL_IMG := $(PROD_DIR)/kernel8-pi4.img
CC := aarch64-none-elf-gcc
CFLAGS := -Wall -nostdlib -nodefaultlibs -nostartfiles -fno-builtin -ffreestanding -mgeneral-regs-only -O0 -mstrict-align
ASFLAGS := 
INCLUDES := -I $(SRC_DIR)/include/
LINKERFILE := $(SRC_DIR)/linker.ld
LINKER := aarch64-none-elf-ld
ARMSTUB_BIN := $(PROD_DIR)/armstub.bin

MOUNT_PREFACE:=

DEBUG ?= 0

# ANSI colors
BLUE   := \033[1;34m
GREEN  := \033[1;32m
YELLOW := \033[1;33m
RED    := \033[1;31m
RESET  := \033[0m


ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -g
endif

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_S),Linux)
  OS := linux
  MOUNT_PREFACE=u
endif


all: $(BIN_DIR) $(KERNEL_IMG) $(ARMSTUB_BIN)
	@printf "$(BLUE)==> Mounting SD card$(RESET)\n"
	./${MOUNT_PREFACE}scripts/mount.sh
	@printf "$(BLUE)==> Copying files to SD card$(RESET)\n"
	cp -r $(PROD_DIR)/* $(SDCARD_DIR)/
	@printf "$(GREEN)==> Ejecting SD card$(RESET)\n"
	./${MOUNT_PREFACE}scripts/eject.sh

local: $(BIN_DIR) $(KERNEL_IMG)

debug: clean
	@printf "$(YELLOW)==> Rebuilding with DEBUG=1$(RESET)\n"
	$(MAKE) DEBUG=1



$(KERNEL_IMG): $(TARGET)
	@printf "$(BLUE)==> Generating kernel image: %s$(RESET)\n" "$@"
	aarch64-none-elf-objcopy -O binary $^ $@

$(TARGET): $(OBJS) $(BIN_DIR)/font.o
	@printf "$(BLUE)==> Linking kernel executable$(RESET)\n"
	$(LINKER) -o $@ $^ -T $(LINKERFILE) $(LFLAGS)


$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	@printf "$(GREEN)==> CC %s$(RESET)\n" "$<"
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(BIN_DIR)/%_s.o: $(SRC_DIR)/%.S
	@printf "$(GREEN)==> AS %s$(RESET)\n" "$<"
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)


$(BIN_DIR):
	@printf "$(BLUE)==> Creating build directories$(RESET)\n"
	mkdir -p $@
	mkdir -p $@/armstub

$(ARMSTUB_BIN): $(BIN_DIR)/armstub/armstub_s.o
	@printf "$(BLUE)==> Linking armstub$(RESET)\n"
	$(LINKER) --section-start=.text=0 -o $(BIN_DIR)/armstub/armstub.elf $^
	aarch64-none-elf-objcopy -O binary $(BIN_DIR)/armstub/armstub.elf $@

$(BIN_DIR)/armstub/armstub_s.o: src/armstub/armstub.S
	@printf "$(GREEN)==> CC %s$(RESET)\n" "$<"
	$(CC) $(CFLAGS) -MMD -c $< -o $@


$(BIN_DIR)/font.o: $(SRC_DIR)/fonts/tamzen10x20.psf
	@printf "$(BLUE)==> Embedding font$(RESET)\n"
	$(LINKER) -r -b binary $< -o $@


clean:
	@printf "$(YELLOW)==> Cleaning build artifacts$(RESET)\n"
	rm -rf $(BIN_DIR)
	rm -rf $(TARGET)
	rm -rf objdump.S
	rm -rf screenlog.0
	rm -rf $(KERNEL_IMG)

asm: local
	@printf "$(BLUE)==> Generating disassembly$(RESET)\n"
	aarch64-none-elf-objdump -d $(TARGET) > objdump.S

read: local
	@printf "$(BLUE)==> Reading ELF sections$(RESET)\n"
	aarch64-none-elf-readelf -S $(TARGET)