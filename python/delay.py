import os
import re
import matplotlib.pyplot as plt
from collections import defaultdict

data_dir = "./var_network_bw_with_cp_bw/cnn-k2col/"  

# Data structure: {(size, precision): {bandwidth: delay}}
data = defaultdict(dict)

# Regex patterns
patterns = {
    "Crossbar Size": r"Crossbar Size:\s*(\d+)\*(\d+)",
    "Bit Precision": r"Bit Precision:\s*(\d+)",
    "Bandwidth": r"Bandwidth:\s*(\d+)",
    "Delay": r"Delay:\s*(\d+)",
}

# Parse files
for filename in os.listdir(data_dir):
    if filename.endswith(".txt"):
        with open(os.path.join(data_dir, filename)) as f:
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
                data[(size, precision)][bandwidth] = delay

# Plot
plt.figure(figsize=(10, 6))
for (size, precision), bw_delay in data.items():
    x = sorted(bw_delay.keys())
    y = [bw_delay[bw] for bw in x]
    label = f"{size[0]}×{size[1]}, {precision}-bit"
    plt.plot(x, y, marker='o', label=label)

plt.title("Delay vs Bandwidth")
plt.xlabel("Bandwidth (bits/unit time)")
plt.ylabel("Delay (unit time)")
plt.yscale("log") 
plt.grid(True)
plt.legend(title="Crossbar Size & Bit Precision", bbox_to_anchor=(1.05, 1), loc="upper left")
plt.tight_layout()
plt.savefig("delay-bandwidth.png")