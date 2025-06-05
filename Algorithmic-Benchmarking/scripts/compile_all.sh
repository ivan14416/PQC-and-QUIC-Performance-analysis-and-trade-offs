#!/bin/bash

echo "Compiling all KEM and SIG benchmarks..."

# Compile KEM benchmarks
./scripts/compile_all_kem.sh
if [ $? -ne 0 ]; then
    echo "KEM compilation failed."
    exit 1
fi

# Compile SIG benchmarks
./scripts/compile_all_sig.sh
if [ $? -ne 0 ]; then
    echo "SIG compilation failed."
    exit 1
fi

echo "All benchmarks compiled successfully."
