#!/bin/bash

# Directory containing the benchmarks
BENCHMARK_DIR="$HOME/HeCBench/src"

# Log file
LOG_FILE="$HOME/HeCBench/benchmark.log"

# List of benchmarks to run
BENCHMARKS=(
    "remap" "relu"
)

# Function to run a benchmark
run_benchmark() {
    local benchmark=$1
    echo "Running benchmark: $benchmark-sycl"
    echo "------------------------------" >> "$LOG_FILE"
    echo "Benchmark: $benchmark-sycl" >> "$LOG_FILE"
    echo "Start time: $(date)" >> "$LOG_FILE"

    # Change to the benchmark directory and run the make command
    if [ -d "$BENCHMARK_DIR/${benchmark}-sycl" ]; then
        (cd "$BENCHMARK_DIR/${benchmark}-sycl" && make CC=icpx run) >> "$LOG_FILE" 2>&1
    else
        echo "Directory not found: $BENCHMARK_DIR/${benchmark}-sycl" >> "$LOG_FILE"
    fi

    echo "End time: $(date)" >> "$LOG_FILE"
    echo "------------------------------" >> "$LOG_FILE"
    echo "" >> "$LOG_FILE"
}

# Run each benchmark
for benchmark in "${BENCHMARKS[@]}"; do
    run_benchmark "$benchmark"
done

echo "All benchmarks completed. Results are in $LOG_FILE"