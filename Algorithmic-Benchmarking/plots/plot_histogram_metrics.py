import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import sys
from glob import glob

# === USAGE CHECK ===
if len(sys.argv) < 2:
    print("Usage: python3 plot_histogram_metrics.py <benchmark_folder>")
    sys.exit(1)

# === PATH CONFIGURATION ===
DATA_FOLDER = sys.argv[1]
OUTPUT_FOLDER = os.path.join(DATA_FOLDER, "plots")
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

# === HISTOGRAM SETTINGS ===
NUM_BINS = 40
CLIP_OUTLIERS = True
CLIP_PERCENTILE = 0.99  # Clip top 1% of values

# === SEABORN STYLE ===
sns.set(style="whitegrid")

# === PROCESS RAW FILES ===
raw_files = glob(os.path.join(DATA_FOLDER, "raw_*.csv"))

for filepath in raw_files:
    # Load CSV data
    data = pd.read_csv(filepath, header=None).squeeze()
    data = pd.to_numeric(data, errors="coerce").dropna()

    # Clip outliers (optional)
    if CLIP_OUTLIERS:
        threshold = data.quantile(CLIP_PERCENTILE)
        data = data[data <= threshold]

    # Parse filename for label and axis
    filename = os.path.basename(filepath).replace("raw_", "").replace(".csv", "")
    if "us" in filename:
        x_label = "Time (μs)"
    elif "cycles" in filename:
        x_label = "CPU Cycles"
    else:
        x_label = "Value"

    # Build histogram
    plt.figure(figsize=(8, 4))
    sns.histplot(data, bins=NUM_BINS, kde=True)
    plt.title(f"{filename.replace('_', ' ').title()} (clipped @ {int(CLIP_PERCENTILE * 100)}%)")
    plt.xlabel(x_label)
    plt.ylabel("Frequency")
    plt.tight_layout()

    # Save to file
    output_path = os.path.join(OUTPUT_FOLDER, f"hist_{filename}.pdf")
    plt.savefig(output_path)
    plt.close()

    print(f"Saved histogram: {output_path}")
