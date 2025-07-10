import os
import re
import matplotlib.pyplot as plt
from collections import defaultdict

# --- Directories to compare ---
data_dirs = {
    "cnn-im2col": "./var_network_bw_with_cp_bw/cnn-im2col/",
    "cnn-k2col": "./var_network_bw_with_cp_bw/cnn-k2col/",
}

# Regex patterns
patterns = {
    "Crossbar Size": r"Crossbar Size:\s*(\d+)\*(\d+)",
    "Bit Precision": r"Bit Precision:\s*(\d+)",
    "Bandwidth": r"Bandwidth:\s*(\d+)",
    "Delay": r"Delay:\s*(\d+)",
}

# Data: {(dir_label, size, precision): {bandwidth: delay}}
data = defaultdict(dict)

# Parse all folders
for label, folder in data_dirs.items():
    for filename in os.listdir(folder):
        if filename.endswith(".txt"):
            with open(os.path.join(folder, filename)) as f:
                content = f.read()
                match_size = re.search(patterns["Crossbar Size"], content)
                match_precision = re.search(patterns["Bit Precision"], content)
                match_bandwidth = re.search(patterns["Bandwidth"], content)
                match_delay = re.search(patterns["Delay"], content)

                if match_size and match_precision and match_bandwidth and match_delay:
                    size = (int(match_size.group(1)), int(match_size.group(2)))
                    precision = int(match_precision.group(1))
                    bandwidth = int(match_bandwidth.group(1))
                    delay = int(match_delay.group(1))
                    key = (label, size, precision)
                    data[key][bandwidth] = delay

# --- Plot ---
plt.figure(figsize=(10, 6))
linestyles = {
    "cnn-im2col": "solid",
    "cnn-k2col": "dashed",
}
colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red']

for idx, ((label, size, precision), bw_delay) in enumerate(data.items()):
    x = sorted(bw_delay.keys())
    y = [bw_delay[bw] for bw in x]
    line_label = f"{label}, {size[0]}×{size[1]}, {precision}-bit"
    plt.plot(x, y, marker='o',
             label=line_label,
             linestyle=linestyles[label],
             color=colors[idx % len(colors)])

plt.title("Delay - Bandwidth Comparison")
plt.yscale("log") 
plt.xlabel("Bandwidth (bits/unit time)")
plt.ylabel("Delay (unit time)")
plt.grid(True)
plt.legend(title="Config & Source", bbox_to_anchor=(1.05, 1), loc="upper left")
plt.tight_layout()
plt.savefig("delay-bandwidth_comparison.png", bbox_inches="tight")
