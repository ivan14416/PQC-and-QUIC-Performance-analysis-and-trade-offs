#!/bin/bash

# Navigate to repo root (assumes script is in scripts/)
cd "$(dirname "$0")/.."

# List of all signature benchmark binaries
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

# Run each benchmark
for bench in "${benchmarks[@]}"; do
    echo "Running $bench..."
    ./bin/$bench
    if [ $? -ne 0 ]; then
        echo "Execution failed for $bench!"
        exit 1
    fi
done

echo "All signature benchmarks completed successfully."

