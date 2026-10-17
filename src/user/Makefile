LDSCRIPT := linker.ld

# ==========================
# Build flavor
#
# Two incompatible toolchains build the same sources into the same bin/:
#
#   freestanding  aarch64-none-elf, our own crt (start.S / syscall.S) and
#                 userlib printf, linked with linker.ld
#   musl          aarch64-linux-musl, real libc + crt startup, static non-PIE
#
# Which one produced the current bin/ is recorded in bin/.flavor, and every
# later invocation reads it back. Without that stamp, a bare `make asm` or
# `make build` re-parses with the freestanding defaults, finds the (unused)
# userlib archive missing, rebuilds it, and -- because the archive is a
# prerequisite of every binary -- relinks the whole of bin/ with the bare-metal
# toolchain. The musl binaries would be silently replaced before being dumped
# or deployed. Stamping makes the flavor sticky so the matching compiler,
# archiver and objdump are used throughout.
#
# `make clean` drops the stamp (back to freestanding). Override for one command
# with e.g. `make asm FLAVOR=musl`.
# ==========================
FLAVOR_STAMP := bin/.flavor
FLAVOR       := $(strip $(shell cat $(FLAVOR_STAMP) 2>/dev/null))
ifeq ($(FLAVOR),)
FLAVOR := freestanding
endif

INCLUDES := \
  -I /Users/jakekasper/C/RaspberryPi/rpos/src/kernel/include \
  -I /Users/jakekasper/C/RaspberryPi/rpos/src/user/include

# DWARF for `add-symbol-file` in gdb. Debug sections are non-ALLOC, so they do
# not appear in any PT_LOAD and the image the kernel actually loads is byte-for-
# byte unchanged -- only the on-disk file grows. Pass OPT=-Og for readable
# single-stepping (-O2 reorders enough to make line stepping jump around).
OPT ?= -O2
DBGFLAGS := -g

ifeq ($(FLAVOR),musl)
  TOOLPREFIX := aarch64-linux-musl-
  # RPOS_MUSL lets shared headers pick between libc's declarations and ours --
  # userlib.h must not declare our `void printf(char*, ...)` when <stdio.h> has
  # already declared `int printf(const char* restrict, ...)`.
  CFLAGS     := -Wall -Wextra $(OPT) $(DBGFLAGS) -fno-pie -no-pie -static -DRPOS_MUSL $(INCLUDES)
  # musl brings its own linker script for a static Linux ELF.
  LDFLAGS    :=
  # ...its own _start and syscall(), so our hand-rolled versions are excluded...
  ASM_FIND_EXCLUDE := ! -name start.S ! -name syscall.S
  # ...and its own printf, so the userlib archive is neither built nor linked.
  USERLIB    :=
else
  TOOLPREFIX := aarch64-none-elf-
  CFLAGS     := -Wall -Wextra $(OPT) $(DBGFLAGS) -ffreestanding -nostdlib -nostartfiles \
                -fno-pie -no-pie $(INCLUDES)
  LDFLAGS    := -T $(LDSCRIPT)
  ASM_FIND_EXCLUDE :=
  USERLIB    := bin/obj/libuser.a
endif

CC      := $(TOOLPREFIX)gcc
AR      := $(TOOLPREFIX)ar
OBJDUMP := $(TOOLPREFIX)objdump

# ANSI colors
GREEN  := \033[1;32m
BLUE   := \033[1;34m
YELLOW := \033[1;33m
RED    := \033[1;31m
RESET  := \033[0m

# Each subdirectory is one command to build, except these:
#   bin/     build output
#   include/ shared headers
#   lib/     shared runtime archive (userlib)
#   dmp/     disassembly / hexdump output from `make asm` and `make dmp`
NON_PROJECT_DIRS := bin/ include/ lib/ dmp/
DIRS := $(filter-out $(NON_PROJECT_DIRS),$(wildcard */))
BINS := $(patsubst %/,bin/%,$(DIRS))

# ==========================
# userlib: the shared user-space runtime (printf, etc). Compiled ONCE from
# lib/*.c and lib/*.S into a static archive that every command links against,
# the way a real toolchain links libc. Because it is an archive rather than a
# pile of objects, only the members a program actually references get pulled in
# -- `ls` and `pwd` don't carry printf's 2 KiB buffer unless they call it.
#
# Lives under bin/obj/ rather than bin/ so it is not picked up by the `build`
# target's bin/* glob and deployed to the rootfs as if it were a command.
#
# USERLIB is empty in the musl flavor (see the flavor block above), which makes
# the archive rule below a no-op target -- harmless, and it keeps our printf
# from colliding with libc's.
# ==========================
LIB_DIR  := lib
LIB_C    := $(wildcard $(LIB_DIR)/*.c)
LIB_ASM  := $(wildcard $(LIB_DIR)/*.S)
LIB_OBJ  := $(patsubst $(LIB_DIR)/%.c,bin/obj/$(LIB_DIR)/%.o,$(LIB_C)) \
            $(patsubst $(LIB_DIR)/%.S,bin/obj/$(LIB_DIR)/%_s.o,$(LIB_ASM))

# ==========================
# Dependency tracking
#
# The pattern rule below has the project DIRECTORY as its prerequisite, and a
# directory's mtime only changes when an entry is added/removed/renamed -- NOT
# when an existing file's contents change. Editing jsh/main.c would therefore
# never trigger a rebuild. Expand each project's real sources at parse time and
# declare them as prerequisites so edits are picked up.
# ==========================
SHARED_HDRS := $(wildcard include/*.h)

$(foreach d,$(DIRS),$(eval bin/$(patsubst %/,%,$(d)): \
    $(shell find $(d) -type f \( -name '*.c' -o -name '*.S' -o -name '*.h' \))))

# Anything shared rebuilds every command: the linker script and the userlib headers.
$(BINS): $(LDSCRIPT) $(SHARED_HDRS)

# The archive's own objects depend on the shared headers too.
$(LIB_OBJ): $(SHARED_HDRS)

.PHONY: all clean build
all: $(BINS)
	@mkdir -p bin
	@printf '%s\n' "$(FLAVOR)" > $(FLAVOR_STAMP)

# ==========================
# musl / freestanding: switch which toolchain bin/ is built with. Both do a
# clean rebuild, since the two flavors' objects are not interchangeable, and
# both leave bin/.flavor behind so `make asm` and `make build` follow suit:
#
#   make musl     then    make asm      -- dumps the musl binaries
#   make musl     then    make build    -- deploys the musl binaries
#   make freestanding                   -- back to the bare-metal toolchain
#
# musl requires the aarch64-linux-musl-* cross-toolchain on PATH:
#   brew tap messense/macos-cross-toolchains
#   brew install aarch64-unknown-linux-musl
# ==========================
MUSL_CC := aarch64-linux-musl-gcc

.PHONY: musl freestanding
musl:
	@command -v $(MUSL_CC) >/dev/null 2>&1 || { printf "$(RED)✗ $(MUSL_CC) not found on PATH — install a musl aarch64 cross-toolchain$(RESET)\n"; exit 1; }
	@printf "$(BLUE)==> Rebuilding binaries with musl ($(GREEN)$(MUSL_CC)$(BLUE))$(RESET)\n"
	@$(MAKE) clean
	@$(MAKE) all FLAVOR=musl
	@printf "$(GREEN)✓ musl build complete$(RESET)\n"

freestanding:
	@printf "$(BLUE)==> Rebuilding binaries with the bare-metal toolchain$(RESET)\n"
	@$(MAKE) clean
	@$(MAKE) all FLAVOR=freestanding
	@printf "$(GREEN)✓ freestanding build complete$(RESET)\n"

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
# Build the shared runtime archive
# ==========================
bin/obj/$(LIB_DIR)/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(dir $@)
	@printf "$(GREEN)==> CC %s$(RESET)\n" "$<"
	@$(CC) $(CFLAGS) -c $< -o $@

bin/obj/$(LIB_DIR)/%_s.o: $(LIB_DIR)/%.S
	@mkdir -p $(dir $@)
	@printf "$(GREEN)==> AS %s$(RESET)\n" "$<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(USERLIB): $(LIB_OBJ)
	@mkdir -p $(dir $@)
	@printf "$(BLUE)==> AR $(GREEN)%s$(RESET)\n" "$@"
	@$(AR) rcs $@ $(LIB_OBJ)

# ==========================
# Build each project
# ==========================
bin/%: %/ $(USERLIB)
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
	$(CC) $(CFLAGS) $(LDFLAGS) $$OBJ_LIST $(USERLIB) -o $@ && \
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


# ==========================
# asm: disassemble the built binaries into dmp/.
#   make asm              -- every command in bin/
#   make asm FILE=jsh     -- just one
#   make asm SYM=main     -- only that symbol (see below)
#
# Depends on `all` so the dump always matches current sources (same convention
# as the kernel Makefile's `asm: local`), and uses the objdump matching
# bin/.flavor so a musl build is dumped rather than relinked.
#
# A static musl binary carries the whole of libc, so a full dump is ~5800 lines
# of mostly libc. SYM= narrows it to one function (objdump --disassemble=SYM).
# To see only your own code without the link step at all, dump the objects:
#   aarch64-linux-musl-objdump -d bin/obj/jsh/*.o
# ==========================
.PHONY: asm
asm: all
	@mkdir -p dmp
	@set -e; \
	if [ -n "$(FILE)" ]; then \
		[ -f "bin/$(FILE)" ] || { printf "$(RED)✗ bin/$(FILE) does not exist$(RESET)\n"; exit 1; }; \
		TARGETS="bin/$(FILE)"; \
	else \
		TARGETS="$(BINS)"; \
	fi; \
	printf "$(BLUE)==> objdump ($(GREEN)$(FLAVOR)$(BLUE): $(OBJDUMP))$(RESET)\n"; \
	for b in $$TARGETS; do \
		n=$$(basename "$$b"); \
		$(OBJDUMP) -d $(if $(SYM),--disassemble="$(SYM)") "$$b" > "dmp/$$n.objdump"; \
		printf "$(GREEN)==> dmp/%s.objdump$(RESET) (%s lines)\n" "$$n" "$$(wc -l < dmp/$$n.objdump | tr -d ' ')"; \
	done
