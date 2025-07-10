import os
import re
import matplotlib.pyplot as plt
from collections import defaultdict

# Define both directories
data_dirs = {
    "cnn-k2col": "./var_network_bw_with_cp_bw/cnn-k2col/",
    "cnn-im2col": "./var_network_bw_with_cp_bw/cnn-im2col/",
}

# Data structure: {source: {bit_precision: {metric: {crossbar_size: value}}}}
all_data = defaultdict(lambda: defaultdict(lambda: defaultdict(dict)))

# Regex patterns
patterns = {
    "Crossbar Size": r"Crossbar Size:\s*(\d+)\*(\d+)",
    "Bit Precision": r"Bit Precision:\s*(\d+)",
    "Crossbar Usage Proportion": r"Crossbar Usage Proportion:\s*([\d.]+)",
    "Required Minimum Bandwidth": r"Required Minimum Bandwidth:\s*([\d.]+)",
    "Delay": r"Delay:\s*([\d.]+)",
    "Total Bits transferred": r"Total Bits transferred:\s*([\d.]+)",
    "Crossbar Amount": r"Crossbar Amount:\s*(\d+)",
}

# Read and parse each directory
for label, data_dir in data_dirs.items():
    for filename in os.listdir(data_dir):
        if filename.endswith(".txt"):
            with open(os.path.join(data_dir, filename), "r") as f:
                content = f.read()
                values = {}
                for key, pattern in patterns.items():
                    match = re.search(pattern, content)
                    if match:
                        if key == "Crossbar Size":
                            values[key] = (int(match.group(1)), int(match.group(2)))
                        elif key == "Bit Precision":
                            values[key] = int(match.group(1))
                        elif key == "Crossbar Amount":
                            values[key] = int(match.group(1))
                        else:
                            values[key] = float(match.group(1))
                if "Bit Precision" in values and "Crossbar Size" in values:
                    bit_precision = values["Bit Precision"]
                    crossbar_size = values["Crossbar Size"]
                    for metric in patterns:
                        if metric not in ["Crossbar Size", "Bit Precision"] and metric in values:
                            all_data[label][bit_precision][metric][crossbar_size] = values[metric]

# Metrics to plot
metrics_to_plot = [
    "Crossbar Usage Proportion",
    # "Required Minimum Bandwidth",
    # "Delay",
    "Total Bits transferred",
    "Crossbar Amount"
]

# Plot
n_metrics = len(metrics_to_plot)
n_cols = 3
n_rows = (n_metrics + n_cols - 1) // n_cols
fig, axes = plt.subplots(n_rows, n_cols, figsize=(18, 10))
axes = axes.flatten()

bit_precisions = [1, 2, 4, 8]
color_map = {bp: color for bp, color in zip(bit_precisions, ['tab:blue', 'tab:orange', 'tab:green', 'tab:red'])}
linestyles = {'cnn-k2col': '-', 'cnn-im2col': '--'}

# Plot each metric
for i, metric in enumerate(metrics_to_plot):
    ax = axes[i]
    for source_idx, (source, data) in enumerate(all_data.items()):
        for bit_precision in sorted(data.keys()):
            metric_data = data[bit_precision]
            if metric in metric_data:
                x = sorted(metric_data[metric].keys(), key=lambda t: t[0] * t[1])
                x_numeric = list(range(len(x)))
                powers = [w.bit_length() - 1 for (w, h) in x]
                y = [metric_data[metric][size] for size in x]
                ax.plot(x_numeric, y,
                        marker='o',
                        linestyle=linestyles[source],
                        color=color_map[bit_precision],
                        label=f"{bit_precision}-bit ({source})")
    ax.set_title(metric)
    ax.set_xlabel("Crossbar Size (2^n × 2^n)")
    ax.set_xticks(x_numeric)
    ax.set_xticklabels(powers)
    ax.set_ylabel(metric)
    if metric in ["Crossbar Amount", "Total Bits transferred"]:
        ax.set_yscale("log")
    ax.grid(True)

# Collect handles/labels once for shared legend
handles, labels = axes[0].get_legend_handles_labels()
fig.legend(handles, labels, title="Bit Precision and Source", loc="lower center", ncol=3, bbox_to_anchor=(0.5, 0.1))

# Hide unused axes
for j in range(i + 1, len(axes)):
    fig.delaxes(axes[j])

plt.tight_layout(rect=[0, 0.2, 1, 1])
plt.savefig("comparison_metrics.png", bbox_inches='tight')

