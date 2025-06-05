#!/bin/bash

# Navigate to repo root (assumes script is in scripts/)
cd "$(dirname "$0")/.."

# List of all benchmark programs
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

# Run each benchmark
for bench in "${benchmarks[@]}"; do
    echo "Running $bench..."
    ./bin/$bench
    if [ $? -ne 0 ]; then
        echo "Execution failed for $bench!"
        exit 1
    fi
done

echo "All KEM benchmarks completed successfully."

