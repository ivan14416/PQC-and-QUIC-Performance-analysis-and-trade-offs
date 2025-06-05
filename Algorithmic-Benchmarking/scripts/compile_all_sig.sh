#!/bin/bash

# Navigate to repo root (assumes script is in scripts/)
cd "$(dirname "$0")/.."

# Create output directory if it doesn't exist
mkdir -p bin

# List of signature benchmark source files (without .c extension)
benchmarks=(
    benchmark_dilithium2
    benchmark_dilithium3
    benchmark_dilithium5
    benchmark_falcon-512
    benchmark_falcon-1024
    benchmark_falcon-padded-512
    benchmark_falcon-padded-1024
    benchmark_ml-dsa-44
    benchmark_ml-dsa-65
    benchmark_ml-dsa-87
)

# Compile each benchmark
for bench in "${benchmarks[@]}"; do
    echo "Compiling $bench..."
    gcc -O3 src/sig/$bench.c -I./liboqs/include -L./liboqs/build/lib -loqs -lssl -lcrypto -lm -o ./bin/$bench
    if [ $? -ne 0 ]; then
        echo "Compilation failed for $bench!"
        exit 1
    fi
done

echo "All signature benchmarks compiled successfully."

