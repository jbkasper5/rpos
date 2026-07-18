CC := aarch64-none-elf-gcc
CFLAGS := -Wall -Wextra -O2
CFLAGS += \
  -ffreestanding \
  -nostdlib \
  -nostartfiles \
  -fno-pie -no-pie \
  -I /Users/jakekasper/C/RaspberryPi/rpos/src/kernel/include

# ANSI colors
GREEN  := \033[1;32m
BLUE   := \033[1;34m
YELLOW := \033[1;33m
RED    := \033[1;31m
RESET  := \033[0m

DIRS := $(filter-out bin/,$(wildcard */))
BINS := $(patsubst %/,bin/%,$(DIRS))

LDSCRIPT := linker.ld
LDFLAGS  := -T $(LDSCRIPT)

# musl cross-toolchain (real libc + crt startup). See the `musl` target below.
MUSL_CC := aarch64-linux-musl-gcc

# Extra `find` predicates for excluding ASM sources. Empty for the freestanding
# build; the musl build drops the hand-rolled crt/syscall stubs so libc's own
# _start / syscall() are used instead.
ASM_FIND_EXCLUDE :=


.PHONY: all clean build
all: $(BINS)

# ==========================
# musl: build the same binaries against a musl libc cross-toolchain (real crt
# startup + C library) instead of the bare-metal freestanding toolchain.
# Static, non-PIE. Excludes each project's start.S / syscall.S so musl's own
# _start and syscall() are used. Does a clean rebuild (flags differ from the
# freestanding build). Requires aarch64-linux-musl-gcc on PATH.
#   make musl    then    make build
# ==========================
.PHONY: musl
musl:
	@command -v $(MUSL_CC) >/dev/null 2>&1 || { printf "$(RED)✗ $(MUSL_CC) not found on PATH — install a musl aarch64 cross-toolchain$(RESET)\n"; exit 1; }
	@printf "$(BLUE)==> Rebuilding binaries with musl ($(GREEN)$(MUSL_CC)$(BLUE))$(RESET)\n"
	@$(MAKE) clean
	@$(MAKE) all \
		CC="$(MUSL_CC)" \
		CFLAGS="-Wall -Wextra -O2 -fno-pie -no-pie -static -I /Users/jakekasper/C/RaspberryPi/rpos/src/kernel/include" \
		LDFLAGS="" \
		ASM_FIND_EXCLUDE="! -name start.S ! -name syscall.S"
	@printf "$(GREEN)✓ musl build complete$(RESET)\n"

# ==========================
# build: compile the commands, then deploy them into the rootfs /bin by
# writing the ext4 partition directly with debugfs (userspace, no mount).
# Disk numbers shift on replug, so override:  make build ROOTFS_DEV=/dev/disk5s2
# ==========================
ROOTFS_DEV ?= /dev/disk5s2

build: all
	@set -e; \
	PART="$(ROOTFS_DEV)"; RAW="/dev/r$${PART#/dev/}"; \
	DBG="$$(brew --prefix e2fsprogs 2>/dev/null)/sbin/debugfs"; \
	[ -x "$$DBG" ] || { printf "$(RED)✗ debugfs not found — run: brew install e2fsprogs$(RESET)\n"; exit 1; }; \
	printf "$(BLUE)==> Deploying to $(GREEN)%s$(BLUE):/bin via debugfs$(RESET)\n" "$$RAW"; \
	{ for f in "$(CURDIR)"/bin/*; do \
		[ -f "$$f" ] || continue; \
		n=$$(basename "$$f"); \
		printf "$(GREEN)==> write %s$(RESET)\n" "$$n" >&2; \
		printf 'rm /bin/%s\nwrite %s /bin/%s\n' "$$n" "$$f" "$$n"; \
	  done; } | sudo "$$DBG" -w "$$RAW"; \
	printf "$(GREEN)✓ Deployed to %s:/bin$(RESET)\n" "$$RAW"; \
	DISK=$$(echo "$$PART" | sed -E 's/s[0-9]+$$//'); \
	printf "$(BLUE)==> Ejecting %s$(RESET)\n" "$$DISK"; \
	sync; diskutil eject "$$DISK" || printf "$(YELLOW)⚠ could not eject %s — unmount it manually$(RESET)\n" "$$DISK"

# fsck: reconcile the rootfs / clear the dirty flag after writes (run occasionally;
# can be slow on a large fs). Override the device the same way as build.
.PHONY: fsck
fsck:
	@PART="$(ROOTFS_DEV)"; RAW="/dev/r$${PART#/dev/}"; \
	FSCK="$$(brew --prefix e2fsprogs 2>/dev/null)/sbin/e2fsck"; \
	printf "$(YELLOW)==> e2fsck -fy %s$(RESET)\n" "$$RAW"; \
	sudo "$$FSCK" -fy "$$RAW" || true; \
	printf "$(GREEN)✓ fsck complete$(RESET)\n"

# ==========================
# Build each project
# ==========================
bin/%: %/
	@printf "$(BLUE)==> Building $(GREEN)%s$(RESET)\n" "$*"
	@mkdir -p bin

	@C_SRC=$$(find $* -type f -name '*.c'); \
	ASM_SRC=$$(find $* -type f -name '*.S' $(ASM_FIND_EXCLUDE)); \
	if [ -z "$$C_SRC" ] && [ -z "$$ASM_SRC" ]; then \
		printf "$(RED)✗ No source files found in %s$(RESET)\n" "$*"; \
		exit 1; \
	fi; \
	OBJ_DIR=bin/obj/$*; \
	mkdir -p $$OBJ_DIR; \
	OBJ_LIST=""; \
	for f in $$C_SRC; do \
		o=$$(echo $$f | sed "s|^$*/|$$OBJ_DIR/|;s|\.c$$|.o|"); \
		mkdir -p $$(dirname $$o); \
		printf "$(GREEN)==> CC %s$(RESET)\n" "$$f"; \
		$(CC) $(CFLAGS) -c $$f -o $$o; \
		OBJ_LIST="$$OBJ_LIST $$o"; \
	done; \
	for f in $$ASM_SRC; do \
		o=$$(echo $$f | sed "s|^$*/|$$OBJ_DIR/|;s|\.S$$|_s.o|"); \
		mkdir -p $$(dirname $$o); \
		printf "$(GREEN)==> AS %s$(RESET)\n" "$$f"; \
		$(CC) $(CFLAGS) -c $$f -o $$o; \
		OBJ_LIST="$$OBJ_LIST $$o"; \
	done; \
	$(CC) $(CFLAGS) $(LDFLAGS) $$OBJ_LIST -o $@ && \
	printf "$(GREEN)✓ Built %s$(RESET)\n" "$@"

clean:
	@printf "$(YELLOW)Cleaning build artifacts$(RESET)\n"
	rm -rf bin
	rm -rf dmp



# make dmp FILE=filename
.PHONY: dmp
dmp:
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make dmp FILE=<executable_name>"; \
		exit 1; \
	fi

	@if [ ! -f "bin/$(FILE)" ]; then \
		echo "Error: bin/$(FILE) does not exist"; \
		exit 1; \
	fi

	@mkdir -p dmp
	@hexdump -C "bin/$(FILE)" > "dmp/$(FILE).dmp"
	@echo "Created dmp/$(FILE).dmp"


# make objdump FILE=filename
.PHONY: asm
asm:
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make objdump FILE=<executable_name>"; \
		exit 1; \
	fi

	@if [ ! -f "bin/$(FILE)" ]; then \
		echo "Error: bin/$(FILE) does not exist"; \
		exit 1; \
	fi

	@mkdir -p dmp
	@aarch64-none-elf-objdump -d "bin/$(FILE)" > "dmp/$(FILE).objdump"
	@echo "Created dmp/$(FILE).objdump"
