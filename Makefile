# Compiler and flags
CC ?= gcc
CFLAGS := -O3 -Wall -Wextra -fopenmp -march=native
LDFLAGS := -static-libgcc -static-libstdc++ -static
RM ?= rm -rf
MKDIR ?= mkdir -p

# Directories
SRC_DIR = src
BUILD_DIR = build

# Skip list implementations
IMPLEMENTATIONS = skiplist_seq skiplist_gl skiplist_lazy skiplist_free

# Benchmark source (renamed from library.c)
BENCHMARK = benchmark

# Default target
.PHONY: all
all: $(BUILD_DIR) build_all
	@echo "All skip list implementations built."

# Create build directory
$(BUILD_DIR):
	$(MKDIR) $(BUILD_DIR)

# Build all implementations
.PHONY: build_all
build_all: $(foreach impl,$(IMPLEMENTATIONS),$(BUILD_DIR)/$(impl))

# Compile rule
$(BUILD_DIR)/%: $(SRC_DIR)/%.c $(SRC_DIR)/$(BENCHMARK).c
	@echo "Compiling $@"
	$(CC) $(CFLAGS) $(LDFLAGS) \
	-DSKIPLIST_HEADER='"$*.h"' \
	-o $@ $^ -I$(SRC_DIR)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build directory..."
	$(RM) $(BUILD_DIR)