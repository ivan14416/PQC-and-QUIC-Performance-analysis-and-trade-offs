import os
from glob import glob
import subprocess

# === CONFIGURATION ===

BENCHMARK_ROOT = "benchmarks"
PLOT_SCRIPTS = [
    "plots/plot_histogram_metrics.py",
    "plots/lineplot_individual_metrics.py",
    "plots/boxplot_individual_metrics.py",
    "plots/boxplot_combined_metrics.py",
    "plots/plot_summary_metrics.py"
]

# ======================

# Find all benchmark folders for kem and sig
benchmark_folders = glob(os.path.join(BENCHMARK_ROOT, "kem", "*", "*")) + \
                    glob(os.path.join(BENCHMARK_ROOT, "sig", "*", "*"))

for folder in benchmark_folders:
    print(f"\nGenerating plots for: {folder}")

    for script in PLOT_SCRIPTS:
        print(f" → Running {script} ...")
        try:
            # Call each plotting script with DATA_FOLDER as argument
            subprocess.run(["python3", script, folder], check=True)
        except subprocess.CalledProcessError:
            print(f"Plotting failed in {script} for {folder}")
