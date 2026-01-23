CC := aarch64-none-elf-gcc
CFLAGS := -Wall -Wextra -O2
CFLAGS += \
  -ffreestanding \
  -nostdlib \
  -nostartfiles \
  -fno-pie -no-pie

# ANSI colors
GREEN  := \033[1;32m
BLUE   := \033[1;34m
YELLOW := \033[1;33m
RED    := \033[1;31m
RESET  := \033[0m

DIRS := $(filter-out bin/,$(wildcard */))
BINS := $(patsubst %/,bin/%,$(DIRS))

.PHONY: all clean
all: $(BINS)

# ==========================
# Build each project
# ==========================
bin/%: %/
	@printf "$(BLUE)==> Building $(GREEN)%s$(RESET)\n" "$*"
	@mkdir -p bin

	@C_SRC=$$(find $* -type f -name '*.c'); \
	ASM_SRC=$$(find $* -type f -name '*.S'); \
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
	$(CC) $(CFLAGS) $$OBJ_LIST -o $@ && \
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