import sys
import os
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from glob import glob

# === Style Configuration ===
sns.set(style="whitegrid")

# === Argument Parsing ===
if len(sys.argv) < 2:
    print("Usage: python3 boxplot_combined_metrics.py <DATA_FOLDER>")
    sys.exit(1)

# Folder containing raw_*.csv files
DATA_FOLDER = sys.argv[1]
RAW_PATTERN = os.path.join(DATA_FOLDER, "raw_*.csv")

# Output path for generated plots
OUTPUT_FOLDER = os.path.join(DATA_FOLDER, "plots")
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

# === Helper Function: Clip outliers above 95th percentile ===
def clip_outliers(df, percentile=0.95):
    df_clipped = df.copy()
    for col in df.columns:
        threshold = df[col].quantile(percentile)
        df_clipped[col] = df[col].clip(upper=threshold)
    return df_clipped

# === Load and Separate Raw Data ===
cycles_data = {}
us_data = {}

for filepath in glob(RAW_PATTERN):
    name = os.path.basename(filepath).replace("raw_", "").replace(".csv", "")
    data = pd.read_csv(filepath, header=None).squeeze()
    data = pd.to_numeric(data, errors="coerce").dropna()

    if name.endswith("cycles"):
        cycles_data[name] = data.reset_index(drop=True)
    elif name.endswith("us"):
        us_data[name] = data.reset_index(drop=True)

# === Combined Boxplot: *_cycles ===
if cycles_data:
    df_cycles = pd.DataFrame(cycles_data).dropna()
    df_cycles_clipped = clip_outliers(df_cycles)

    plt.figure(figsize=(14, 6))
    sns.boxplot(data=df_cycles_clipped)
    plt.xticks(rotation=45)
    plt.ylabel("CPU Cycles")
    plt.title("Combined Boxplot: All *_cycles metrics (95% Clipping)")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_FOLDER, "boxplot_combined_cycles.pdf"))
    plt.close()

# === Combined Boxplot: *_us ===
if us_data:
    df_us = pd.DataFrame(us_data).dropna()
    df_us_clipped = clip_outliers(df_us)

    plt.figure(figsize=(14, 6))
    sns.boxplot(data=df_us_clipped)
    plt.xticks(rotation=45)
    plt.ylabel("Time (µs)")
    plt.title("Combined Boxplot: All *_us metrics (95% Clipping)")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_FOLDER, "boxplot_combined_us.pdf"))
    plt.close()
