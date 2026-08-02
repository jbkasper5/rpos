# rpos host test harness
#
#   make test            build + run the whole suite (exit 0 == gate passes)
#   make test FILTER=x   run only tests whose suite/name contains "x"
#   make build           build only
#   make clean
#
# Compiles selected kernel memory translation units UNMODIFIED against a
# simulated-RAM host environment (host/host_env.c) and links them with the
# unit suite. No kernel source is touched.

CC      ?= cc
KROOT   := ../src/kernel
KINC    := $(KROOT)/include
KSRC    := $(KROOT)/src

# The kernel's scalar.h does `typedef u16 pid_t`, which collides with the host
# libc pid_t. We avoid it structurally: no TU includes both kernel and libc
# headers (host_sys.c is libc-only; everything else is kernel-only). See test.h.
CFLAGS  := -std=gnu11 -g -O0 -Wall -Wextra \
           -Wno-unused-parameter -Wno-unused-function \
           -Wno-incompatible-library-redeclaration \
           -I$(KINC) -Iframework -Ihost -Iunit
LDFLAGS :=

# The kernel TUs freely convert void*<->u64<->pte* (fine under the on-target
# aarch64 toolchain, which the kernel is built with). Modern clang promotes
# those to hard errors, so relax them for kernel sources ONLY -- harness code
# is still compiled with the strict CFLAGS above. No kernel source is changed.
KCFLAGS := $(CFLAGS) \
           -Wno-error=int-conversion            -Wno-int-conversion \
           -Wno-error=incompatible-pointer-types -Wno-incompatible-pointer-types \
           -Wno-error=return-mismatch           -Wno-return-mismatch \
           -Wno-error=implicit-function-declaration \
           -Wno-incompatible-library-redeclaration -Wno-sign-compare

BUILD   := build
BIN     := $(BUILD)/run_tests

# kernel TUs exercised by the harness (add more as coverage grows)
KOBJS   := $(BUILD)/paging.o
# harness TUs
HOBJS   := $(BUILD)/test.o $(BUILD)/host_env.o $(BUILD)/host_sys.o \
           $(BUILD)/test_pfa.o $(BUILD)/test_buddy.o $(BUILD)/test_buddy_invariants.o

OBJS    := $(KOBJS) $(HOBJS)

.PHONY: test build clean
test: build
	@echo "== running host test suite =="
	@./$(BIN) $(FILTER)

build: $(BIN)

debug: build
	@echo "== Starting lldb =="	
	@lldb build/run_tests

$(BIN): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# --- kernel translation units (unmodified; relaxed conversion diagnostics) ---
$(BUILD)/paging.o: $(KSRC)/memory/paging.c
	@mkdir -p $(@D)
	$(CC) $(KCFLAGS) -c -o $@ $<

# --- harness translation units ---
$(BUILD)/test.o: framework/test.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/host_env.o: host/host_env.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/host_sys.o: host/host_sys.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/test_pfa.o: unit/test_pfa.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/test_buddy.o: unit/test_buddy.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<
$(BUILD)/test_buddy_invariants.o: unit/test_buddy_invariants.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD)
