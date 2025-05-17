SRC_DIR=src
BIN_DIR=bin
# BOOTCSRCS=$(shell find bootloader/$(SRC_DIR)/*.c 2>/dev/null)
# BOOTASMSRCS=$(shell find bootloader/$(SRC_DIR)/*.s 2>/dev/null)
# SCHEDCSRCS=$(shell find scheduler/$(SRC_DIR)/*.c 2>/dev/null)
# SCHEDASMSRCS=$(shell find scheduler/$(SRC_DIR)/*.s 2>/dev/null)
# SRCS:=$(SCHEDCSRCS) $(SCHEDASMSRCS) $(BOOTCSRCS) $(BOOTASMSRCS)
# TOBJS:=$(patsubst bootloader/$(SRC_DIR)/%, $(BIN_DIR)/%, $(TOBJS))
# OBJS=$(patsubst scheduler/$(SRC_DIR)/%, $(BIN_DIR)/%, $(TOBJS))

SRCS=$(shell find $(SRC_DIR)/*.s) $(shell find $(SRC_DIR)/*.c)
TOBJS=$(patsubst %.c, %.o, $(SRCS))
TOBJS:=$(patsubst %.s, %_s.o, $(TOBJS))
TOBJS:=$(patsubst %_start_s.o, %_start.o, $(TOBJS))
OBJS=$(patsubst $(SRC_DIR)/%, $(BIN_DIR)/%, $(TOBJS))

INCLUDES= -I include/
LINKERFILE=linker.ld

CC=				aarch64-none-elf-gcc
LINKER=			aarch64-none-elf-ld
OPT=			-O3
AARCHFLAGS=		-mcmodel=large
CFLAGS=			-g -std=c11 -nostdlib -nodefaultlibs -nostartfiles $(AARCHFLAGS)
TARGET=			exe
LFLAGS=			 
IMG=			kernel.img

all: $(BIN_DIR) $(TARGET) $(IMG)

re: clean all

.PHONY: debug
debug: $(DEBUG_BIN_DIR) $(DTARGET)

$(IMG): $(TARGET)
	aarch64-none-elf-objcopy -O binary $^ $@

# @ is the rule, ^ is the prereqs
$(TARGET): $(OBJS)
	$(LINKER) -o $@ $^ -T $(LINKERFILE) $(LFLAGS) --entry=0x80000000

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

# append an _s to the assembly object files to allow same-name files
$(BIN_DIR)/%_s.o: $(SRC_DIR)/%.s
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

# unknown ELF requires a start.o object, so we need to compile this specially
$(BIN_DIR)/_start.o: $(SRC_DIR)/_start.s
	$(CC) $(CFLAGS) -o $@ -c $< $(INCLUDES)

$(BIN_DIR):
	mkdir -p $@

.PHONY: clean
clean:
	$(RM) -rf $(TARGET)
	$(RM) -rf **/$(BIN_DIR)
	$(RM) -rf $(BIN_DIR)
	$(RM) -rf objdump.s
	$(RM) -rf *.txt
	$(RM) -rf $(IMG)

.PHONY: asm
asm: all
	aarch64-none-elf-objdump -d $(TARGET) > objdump.s

run: all
	qemu-system-aarch64 -M raspi3b -kernel $(IMG) -serial null -serial stdio -d in_asm

elf: all
	aarch64-none-elf-readelf -a $(TARGET) > elf.txt

.PHONY: gdb
gdb: all
	aarch64-none-elf-gdb $(TARGET) -ex "target remote localhost:1234"

wait: all
	qemu-system-aarch64 -machine raspi3b -serial null -serial stdio -d in_asm -kernel $(IMG) -S -s 