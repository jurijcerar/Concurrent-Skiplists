# Compiler and flags
CC ?= gcc
CFLAGS := -O3 -Wall -Wextra -fopenmp
LDFLAGS := -static-libgcc -static-libstdc++ -static
RM ?= @rm -rf
MKDIR ?= @mkdir -p

# Directories
SRC_DIR = src
BUILD_DIR = build
DATA_DIR = data

# Skip list implementations
IMPLEMENTATIONS = skiplist skiplist3 skiplist4 skiplist5

# Benchmark executable
BENCHMARK = library

# Default target
.PHONY: all
all: $(BUILD_DIR) $(DATA_DIR) build_all
	@echo "All skip list implementations built."

# Create directories
$(BUILD_DIR):
	@echo "Creating build directory: $(BUILD_DIR)"
	$(MKDIR) $(BUILD_DIR)

$(DATA_DIR):
	@echo "Creating data directory: $(DATA_DIR)"
	$(MKDIR) $(DATA_DIR)

# Build all implementations
.PHONY: build_all
build_all: $(foreach impl, $(IMPLEMENTATIONS), $(BUILD_DIR)/$(impl))

$(BUILD_DIR)/%: $(SRC_DIR)/%.c $(SRC_DIR)/$(BENCHMARK).c
	@echo "Compiling $@"
	$(CC) $(CFLAGS) $(LDFLAGS) -DSKIPLIST_HEADER='"$*.h"' -o $@ $^ -I$(SRC_DIR)

# Run benchmarks for all implementations and save results in unique files
.PHONY: small-bench
small-bench: $(foreach impl, $(IMPLEMENTATIONS), run_$(impl))

run_skiplist: $(BUILD_DIR)/skiplist
	@echo "Running benchmarks for skiplist..."
	@mkdir -p $(DATA_DIR)/skiplist
	for operation_mix in "10 10 80" "40 40 20"; do \
		RESULT=$$(./$(BUILD_DIR)/skiplist --threads 1 --experiments 1 --duration 1 --operations $$operation_mix \
			--key_range 0 100000 --strategy RANDOM --seed 12345 --prefill 0 --range_type COMMON); \
		summary_line=$$(echo "$$RESULT" | grep "Summary:"); \
		echo "Threads,Duration,Total_Operations,Successful_Operations,Insert_Success,Delete_Success,Contains_Success,Throughput" > $(DATA_DIR)/skiplist/results-threads-1-duration-$$duration-mix-$$(echo $$operation_mix | tr ' ' '-').csv; \
		echo "$$summary_line" | awk -F', ' ' \
			BEGIN { OFS=","; } \
			{ \
				for (i = 1; i <= NF; i++) { \
					split($$i, pair, "="); \
					values[i] = pair[2]; \
				} \
				print values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8]; \
			}' >> $(DATA_DIR)/skiplist/results-threads-1-duration-$$duration-mix-$$(echo $$operation_mix | tr ' ' '-').csv; \
	done; \

run_skiplist3: $(BUILD_DIR)/skiplist3
	@echo "Running benchmarks for skiplist3..."
	@mkdir -p $(DATA_DIR)/skiplist3
	for threads in 1 2 4 8 16 64; do \
		for operation_mix in "10 10 80"; do \
			RESULT=$$(./$(BUILD_DIR)/skiplist3 --threads $$threads --experiments 1 --duration 1 --operations $$operation_mix \
				--key_range 0 100000 --strategy RANDOM --seed 12345 --prefill 0 --range_type COMMON); \
			summary_line=$$(echo "$$RESULT" | grep "Summary:"); \
			echo "Threads,Duration,Total_Operations,Successful_Operations,Insert_Success,Delete_Success,Contains_Success,Throughput" > $(DATA_DIR)/skiplist3/results-threads-$$threads-duration-$$duration-mix-$$(echo $$operation_mix | tr ' ' '-').csv; \
			echo "$$summary_line" | awk -F', ' ' \
				BEGIN { OFS=","; } \
				{ \
					for (i = 1; i <= NF; i++) { \
						split($$i, pair, "="); \
						values[i] = pair[2]; \
					} \
					print values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8]; \
				}' >> $(DATA_DIR)/skiplist3/results-threads-$$threads-duration-$$duration-mix-$$(echo $$operation_mix | tr ' ' '-').csv; \
		done; \
	done

run_skiplist4: $(BUILD_DIR)/skiplist4
	@echo "Running benchmarks for skiplist4..."
	@mkdir -p $(DATA_DIR)/skiplist4
	for threads in 1 8 16 64; do \
		for operation_mix in "10 10 80"; do \
			for range_type in COMMON DISJOINT; do \
				RESULT=$$(./$(BUILD_DIR)/skiplist4 --threads $$threads --experiments 3 --duration 1 --operations $$operation_mix \
					--key_range 0 100000 --strategy RANDOM --seed 12345 --prefill 0 --range_type $$range_type); \
				summary_line=$$(echo "$$RESULT" | grep "Summary:"); \
				echo "Threads,Duration,Total_Operations,Successful_Operations,Insert_Success,Delete_Success,Contains_Success,Throughput" > $(DATA_DIR)/skiplist4/results-threads-$$threads-duration-$$duration-mix-$$(echo $$operation_mix | tr ' ' '-')-$$range_type.csv; \
				echo "$$summary_line" | awk -F', ' ' \
					BEGIN { OFS=","; } \
					{ \
						for (i = 1; i <= NF; i++) { \
							split($$i, pair, "="); \
							values[i] = pair[2]; \
						} \
						print values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8]; \
					}' >> $(DATA_DIR)/skiplist4/results-threads-$$threads-duration-$$duration-mix-$$(echo $$operation_mix | tr ' ' '-')-$$range_type.csv; \
			done; \
		done; \
	done

run_skiplist5: $(BUILD_DIR)/skiplist5
	@echo "Running benchmarks for skiplist5..."
	@mkdir -p $(DATA_DIR)/skiplist5
	for threads in 1 8 16 64; do \
		for operation_mix in "10 10 80"; do \
			for range_type in COMMON DISJOINT; do \
				RESULT=$$(./$(BUILD_DIR)/skiplist4 --threads $$threads --experiments 3 --duration 1 --operations $$operation_mix \
					--key_range 0 100000 --strategy RANDOM --seed 12345 --prefill 0 --range_type $$range_type); \
				summary_line=$$(echo "$$RESULT" | grep "Summary:"); \
				echo "Threads,Duration,Total_Operations,Successful_Operations,Insert_Success,Delete_Success,Contains_Success,Throughput" > $(DATA_DIR)/skiplist5/results-threads-$$threads-duration-$$duration-mix-$$(echo $$operation_mix | tr ' ' '-')-$$range_type.csv; \
				echo "$$summary_line" | awk -F', ' ' \
					BEGIN { OFS=","; } \
					{ \
						for (i = 1; i <= NF; i++) { \
							split($$i, pair, "="); \
							values[i] = pair[2]; \
						} \
						print values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8]; \
					}' >> $(DATA_DIR)/skiplist5/results-threads-$$threads-duration-$$duration-mix-$$(echo $$operation_mix | tr ' ' '-')-$$range_type.csv; \
			done; \
		done; \
	done


.PHONY: small-plot
small-plot:
	@echo "Generating plots using benchmark.py..."
	python3 benchmark.py
	@echo "Plots generated successfully! Check the plots in $(DATA_DIR)/plots"


.PHONY: clean
clean:
	@echo "Cleaning up..."
	$(RM) $(BUILD_DIR)/*
	$(RM) $(DATA_DIR)/*
