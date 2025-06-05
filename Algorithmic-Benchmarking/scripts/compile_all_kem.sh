#!/bin/bash


# Navigate to repo root (assumes script is in scripts/)
cd "$(dirname "$0")/.."

# Create output directory if it doesn't exist
mkdir -p bin

# List of benchmark source files (without .c extension)
benchmarks=(
    benchmark_hqc128
    benchmark_hqc192
    benchmark_hqc256
    benchmark_kyber512
    benchmark_kyber768
    benchmark_kyber1024
    benchmark_ml-kem-512
    benchmark_ml-kem-768
    benchmark_ml-kem-1024
)

# Compile each benchmark
for bench in "${benchmarks[@]}"; do
    echo "Compiling $bench..."
    gcc -O3 src/kem/$bench.c -I./liboqs/include -L./liboqs/build/lib -loqs -lssl -lcrypto -lm -o ./bin/$bench
    if [ $? -ne 0 ]; then
        echo "Compilation failed for $bench!"
        exit 1
    fi
done

echo "All KEM benchmarks compiled successfully."

