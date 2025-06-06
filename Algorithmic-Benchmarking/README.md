# BA_2025_PQC_QUIC

This project benchmarks Post-Quantum Cryptography (PQC) – Key Encapsulation Mechanisms (KEMs) and Digital Signatures (SIGs) – using [`liboqs`](https://github.com/open-quantum-safe/liboqs).  
The goal is to measure the performance (time & CPU cycles) of NIST-finalist PQC algorithms and store & visualize the results in a reproducible and structured way.

---

## Prerequisites

The following tools and libraries must be installed on your Raspberry Pi (or Linux system):

### System Packages

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git python3 python3-pip libssl-dev
```

---


### Clone Github
```bash
git clone (this Repository)
cd (in this Repository)
```


---

### Build liboqs locally in this repository
```bash
git clone --recursive https://github.com/open-quantum-safe/liboqs.git
cd liboqs
cmake -S . -B build \
  -DOQS_BUILD_ONLY_LIB=ON \
  -DOQS_ENABLE_KEM_KYBER=ON \
  -DOQS_ENABLE_KEM_HQC=ON \
  -DOQS_ENABLE_SIG_DILITHIUM=ON \
  -DOQS_ENABLE_SIG_FALCON=ON
cmake --build build
cmake --install build --prefix install

```

In the benchmark C files, these paths are used:
    Include: -I./liboqs/include
    Library: -L./liboqs/build/lib -loqs

---

### Enable PMU (Performance Counter) on Raspberry Pi
To enable performance measurements using perf_event_open, the kernel parameter must be adjusted:


   Temporary (until next reboot):
   ```bash
   sudo sh -c 'echo 0 > /proc/sys/kernel/perf_event_paranoid'
   ```

  You can check the current value with:
  ```bash
  cat /proc/sys/kernel/perf_event_paranoid
  ```

  Optional: Make it permanent (persists after reboot)
  ```bash
  echo "kernel.perf_event_paranoid=0" | sudo tee -a /etc/sysctl.conf
  sudo sysctl -p
  ```

---

### Set execution permissions (important after clone)

If you cloned the repository or downloaded it as a ZIP, make sure all shell scripts are executable:

```bash
chmod +x scripts/*.sh
```
Otherwise, you might get a "permission denied" error when trying to run compile_all.sh, run_all_kem.sh, etc.

---

###  Compile & Run Benchmarks

#### Compile and Run Everything

To compile and run all KEM and Signature benchmarks in one step each:

```bash
# Compile everything
./scripts/compile_all.sh
```

```bash
# Run everything
./scripts/run_all.sh
```

#### Compile single

directory bin for organized structure.
```bash
mkdir bin
```

Compile Example (Kyber512)
```bash
gcc -O3 src/kem/benchmark_kyber512.c -I./liboqs/include -L./liboqs/build/lib -loqs -lssl -lcrypto -lm -o bin/benchmark_kyber512
```
Compile Example (Dilithium3)
```bash
gcc -O3 src/sig/benchmark_dilithium3.c -I./liboqs/include -L./liboqs/build/lib -loqs -lssl -lcrypto -lm -o bin/benchmark_dilithium3
```

#### Compiling all Benchmarks

```bash
# Compile all Key Encapsulation Mechanism (KEM) benchmarks
./scripts/compile_all_kem.sh
```
```bash
# Compile all Signature benchmarks
./scripts/compile_all_sig.sh
```
This will automatically build all .c files in src/kem/ and src/sig/ respectively.
The resulting binaries are saved either in the current directory or a bin/ folder depending on your script (adjust accordingly).



#### Run single Benchmarks

Run Benchmarks
```bash
./benchmark_kyber512
./benchmark_dilithium3
```

#### Running all Benchmarks

# Run all compiled KEM benchmarks
```bash
./scripts/run_all_kem.sh
```

# Run all compiled Signature benchmarks
```bash
./scripts/run_all_sig.sh
```
Each execution will create a new result directory in benchmarks/kem/<algorithm>/<timestamp>/ or benchmarks/sig/<algorithm>/<timestamp>/.

Each run creates a result folder in:
    Each folder contains:
    
    -metadata.txt: Information about parameters and algorithm
    -summary.csv: Statistical results (Average, Min, Max, Variance, Sigma)
    -raw_*.csv: 1000 raw measurements per operation & metric

---

### Visualizing Results
read README in plots directory.
../plots/README.md

--- 
```
BA_2025_PQC_QUIC/
├── benchmarks/             # Contains benchmark results (auto-generated)
│   ├── kem/
│   │   └── kyber512/2025-05-06_21-58-23/
│   └── sig/
│       └── dilithium3/2025-05-20_15-00-00/
│           ├── raw_keygen_us.csv
│           ├── summary.csv
│           └── *.pdf (plots)
├── liboqs/                 # Local installation of liboqs (linked manually)
├── plots/                 # Python scripts for visualizing raw data
│   ├── run_all_plots.py
│   ├── plot_histogram_metrics.py
│   ├── boxplot_individual_metrics.py
│   ├── boxplot_combined_metrics.py
│   ├── lineplot_individual_metrics.py
│   └── plot_summary_metrics.py
├── scripts/                # Shell scripts to compile and run benchmarks
│   ├── compile_all_kem.sh
│   ├── compile_all_sig.sh
│   ├── run_all_kem.sh
│   └── run_all_sig.sh
├── src/                    # All benchmark C files
│   ├── kem/
│   │   ├── benchmark_kyber512.c
│   │   └── ...
│   └── sig/
│       ├── benchmark_dilithium3.c
│       └── ...
└── README.md

