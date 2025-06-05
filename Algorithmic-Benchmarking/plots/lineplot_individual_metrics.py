import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import sys
from glob import glob

# === USAGE CHECK ===
if len(sys.argv) < 2:
    print("Usage: python3 lineplot_individual_metrics.py <benchmark_folder>")
    sys.exit(1)

# === PATH SETUP ===
DATA_FOLDER = sys.argv[1]  # Folder passed as argument
OUTPUT_FOLDER = os.path.join(DATA_FOLDER, "plots")
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

# === STYLING ===
sns.set(style="whitegrid")

# === LOAD raw_*.csv FILES ===
raw_files = glob(os.path.join(DATA_FOLDER, "raw_*.csv"))

if not raw_files:
    print(f"No raw_*.csv files found in {DATA_FOLDER}")
    sys.exit(0)

# === PLOTTING LOOP ===
for filepath in raw_files:
    data = pd.read_csv(filepath, header=None).squeeze()
    data = pd.to_numeric(data, errors="coerce").dropna()

    filename = os.path.basename(filepath).replace("raw_", "").replace(".csv", "")

    # Label y-axis depending on type
    if "us" in filename:
        y_label = "Time (μs)"
    elif "cycles" in filename:
        y_label = "CPU Cycles"
    else:
        y_label = "Value"

    # Generate line plot
    plt.figure(figsize=(10, 4))
    plt.plot(data.index, data.values, label=filename, linewidth=1)
    plt.xlabel("Repetition Index")
    plt.ylabel(y_label)
    plt.title(f"Measurement Trend: {filename.replace('_', ' ').title()}")
    plt.grid(True)
    plt.tight_layout()

    # Save output
    output_path = os.path.join(OUTPUT_FOLDER, f"line_{filename}.pdf")
    plt.savefig(output_path)
    plt.close()

    print(f"Saved line plot: {output_path}")
