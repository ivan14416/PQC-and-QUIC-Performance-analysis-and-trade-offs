import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import sys

# === USAGE CHECK ===
if len(sys.argv) < 2:
    print("Usage: python3 plot_summary_metrics.py <benchmark_folder>")
    sys.exit(1)

# === PATH SETUP ===
DATA_FOLDER = sys.argv[1]  # Get benchmark folder path from CLI argument
OUTPUT_FOLDER = os.path.join(DATA_FOLDER, "plots")
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

# === METRICS TO PLOT ===
STAT_COLUMNS = ["Average", "Min", "Max", "Variance", "Sigma"]

# === LOAD SUMMARY ===
summary_path = os.path.join(DATA_FOLDER, "summary.csv")
if not os.path.isfile(summary_path):
    print(f"summary.csv not found in {DATA_FOLDER}")
    sys.exit(1)

df = pd.read_csv(summary_path)

# === CLEAN AND PREPARE ===
df["Label"] = df["Operation"] + " (" + df["Metric"] + ")"

# === PLOTTING ===
for stat in STAT_COLUMNS:
    if stat not in df.columns:
        print(f"Column '{stat}' not found. Skipping.")
        continue

    plt.figure(figsize=(10, 5))
    sns.barplot(data=df, x="Label", y=stat, hue="Metric", palette="Set2")

    plt.xticks(rotation=45, ha="right")
    plt.title(f"{stat} per Operation (us / cycles)")
    plt.xlabel("Operation + Metric")
    plt.ylabel(stat)
    plt.tight_layout()

    output_path = os.path.join(OUTPUT_FOLDER, f"{stat.lower()}_summary_plot.pdf")
    plt.savefig(output_path)
    plt.close()

    print(f"Saved summary plot: {output_path}")
