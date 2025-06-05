#!/bin/bash

echo "Running all KEM and SIG benchmarks..."

# Run KEM benchmarks
./scripts/run_all_kem.sh
if [ $? -ne 0 ]; then
    echo "KEM benchmark execution failed."
    exit 1
fi

# Run SIG benchmarks
./scripts/run_all_sig.sh
if [ $? -ne 0 ]; then
    echo "SIG benchmark execution failed."
    exit 1
fi

echo "All benchmarks executed successfully."
