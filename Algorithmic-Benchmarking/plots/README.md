# Python Plotting Environment (Raspberry Pi or Debian-based systems)


To run the Python plotting scripts (*.py in the plots/ folder), you need a few packages like pandas, matplotlib, and seaborn.

Because Raspberry Pi OS uses a protected system environment (PEP 668), we recommend using a virtual environment:

Create and activate a virtual environment
```bash
python3 -m venv venv
source venv/bin/activate
```

Install required Python packages
```bash
pip install pandas matplotlib seaborn
```

After this, you can run any plotting script, for example:

Run single plots
```bash
python3 plots/boxplot_individual_metrics.py benchmarks/kem/kyber512/2025-05-06_21-58-23
```

Run all plots for all algorithms
```bash
python3 plots/run_all_plots.py
```
This will generate:

    -Boxplots (individual + grouped)
    -Line plots (raw measurements)
    -Histograms
    -Summary bar charts (from summary.csv)

All plots are saved directly in the corresponding benchmark folder.


Exit the virtual environment
When you're done:
```bash
deactivate
```

This returns your shell to the system’s default Python environment.
