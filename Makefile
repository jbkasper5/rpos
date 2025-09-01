BIN_DIR := bin
SRC_DIR := src/kernel
SDCARD_DIR := sdcard
PROD_DIR := prod
C_SRCS = $(shell find $(SRC_DIR) -type f -name '*.c') 
ASM_SRCS = $(shell find $(SRC_DIR) -type f -name '*.S')
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(C_SRCS)) $(patsubst $(SRC_DIR)/%.S, $(BIN_DIR)/%.o, $(ASM_SRCS))

TARGET := exe
KERNEL_IMG := $(PROD_DIR)/kernel8-pi4.img
CC := aarch64-none-elf-gcc
CFLAGS := -Wall -nostdlib -nodefaultlibs -nostartfiles -fno-builtin -ffreestanding -mgeneral-regs-only
ASFLAGS := 
INCLUDES := -I $(SRC_DIR)/include/
LINKERFILE := $(SRC_DIR)/linker.ld
LINKER := aarch64-none-elf-ld
ARMSTUB_BIN := $(PROD_DIR)/armstub.bin


all: $(BIN_DIR) $(KERNEL_IMG) $(ARMSTUB_BIN)
	./scripts/mount.sh
	cp -r $(PROD_DIR)/* $(SDCARD_DIR)/
	./scripts/eject.sh

local: $(BIN_DIR) $(KERNEL_IMG)

$(KERNEL_IMG): $(TARGET)
	aarch64-none-elf-objcopy -O binary $^ $@

# @ is the rule, ^ is the prereqs
$(TARGET): $(OBJS)
	$(LINKER) -o $@ $^ -T $(LINKERFILE) $(LFLAGS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.S
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

$(BIN_DIR):
	mkdir -p $@
	mkdir -p $@/armstub

$(ARMSTUB_BIN): $(BIN_DIR)/armstub/armstub_s.o
	$(LINKER) --section-start=.text=0 -o $(BIN_DIR)/armstub/armstub.elf $^
	aarch64-none-elf-objcopy -O binary $(BIN_DIR)/armstub/armstub.elf $@


$(BIN_DIR)/armstub/armstub_s.o: src/armstub/armstub.S
	$(CC) $(CFLAGS) -MMD -c $< -o $@

clean:
	rm -rf $(BIN_DIR)
	rm -rf $(TARGET)
	rm -rf $(KERNEL_IMG)

asm:
	aarch64-none-elf-objdump -d $(TARGET) > objdump.S