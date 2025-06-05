import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import sys
from glob import glob

# === USAGE CHECK ===
if len(sys.argv) < 2:
    print("Usage: python3 boxplot_individual_metrics.py <benchmark_folder>")
    sys.exit(1)

# === PATH SETUP ===
DATA_FOLDER = sys.argv[1]
OUTPUT_FOLDER = os.path.join(DATA_FOLDER, "plots")
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

# === CONFIG ===
CLIP_OUTLIERS = True
CLIP_PERCENTILE = 0.95
sns.set(style="whitegrid")

def clip_outliers(series, percentile=0.95):
    """Clip extreme outliers above the given percentile."""
    threshold = series.quantile(percentile)
    return series.clip(upper=threshold)

# === LOAD AND PLOT ===
raw_files = glob(os.path.join(DATA_FOLDER, "raw_*.csv"))

if not raw_files:
    print(f"No raw_*.csv files found in {DATA_FOLDER}")
    sys.exit(0)

for filepath in raw_files:
    filename = os.path.basename(filepath)
    label = filename.replace("raw_", "").replace(".csv", "")

    # Load data
    data = pd.read_csv(filepath, header=None).squeeze()
    data = pd.to_numeric(data, errors="coerce").dropna()

    if CLIP_OUTLIERS:
        data = clip_outliers(data, percentile=CLIP_PERCENTILE)

    # Axis labeling
    if "us" in label:
        y_label = "Time (μs)"
        operation = label.replace("_us", "")
    elif "cycles" in label:
        y_label = "CPU Cycles"
        operation = label.replace("_cycles", "")
    else:
        y_label = "Measurement"
        operation = label

    operation_title = operation.capitalize()

    # Plot
    plt.figure(figsize=(6, 4))
    sns.boxplot(y=data)
    plt.title(f"{operation_title} ({y_label}, clipped @ {int(CLIP_PERCENTILE*100)}%)")
    plt.ylabel(y_label)
    plt.tight_layout()

    output_path = os.path.join(OUTPUT_FOLDER, f"boxplot_{label}.pdf")
    plt.savefig(output_path)
    plt.close()

    print(f"Saved boxplot: {output_path}")
