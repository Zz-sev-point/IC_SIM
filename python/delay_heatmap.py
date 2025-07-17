import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import os
import plotly.express as px
import plotly.graph_objects as go
import numpy as np
from matplotlib.colors import LogNorm

# Step 1: List all files in the folder
folder_path = "./var_network_bw_with_cp_bw/fc/"
files = [f for f in os.listdir(folder_path) if f.endswith('.txt') or f.endswith('.csv')]

# Step 2: Parse each file and extract data
data = []
for file in files:
    with open(os.path.join(folder_path, file), 'r') as f:
        content = f.read()
        # Extract parameters (adjust regex/parsing based on your file format)
        crossbar_size = int(content.split("Crossbar Size: ")[1].split()[0].split("*")[0])
        bit_precision = int(content.split("Bit Precision: ")[1].split()[0])
        crossbar_amount = int(content.split("Crossbar Amount: ")[1].split()[0])
        delay = int(content.split("Delay: ")[1].split()[0])
        total_bits = int(content.split("Total Bits transferred: ")[1].split()[0])
        usage = float(content.split("Crossbar Usage Proportion: ")[1].split()[0])
        bandwidth = int(content.split("Bandwidth: ")[1].split()[0])
        data.append({
            "Crossbar Size": crossbar_size,
            "Bit Precision": bit_precision,
            "Crossbar Amount": crossbar_amount,
            "Delay": delay,
            "Usage": usage,
            "Total Bits": total_bits,
            "Bandwidth": bandwidth
        })

# Step 3: Convert to DataFrame
df = pd.DataFrame(data)

slice_val_1 = 1
slice_mask_1 = (df['Bit Precision'] >= slice_val_1-0.5) & (df['Bit Precision'] <= slice_val_1+0.5)
slice_df_1 = df[slice_mask_1]

slice_val_4 = 4
slice_mask_4 = (df['Bit Precision'] >= slice_val_4-0.5) & (df['Bit Precision'] <= slice_val_4+0.5)
slice_df_4 = df[slice_mask_4]

slice_val_8 = 8
slice_mask_8 = (df['Bit Precision'] >= slice_val_8-0.5) & (df['Bit Precision'] <= slice_val_8+0.5)
slice_df_8 = df[slice_mask_8]

# Pivot the data to create a matrix for the heatmap
heatmap_data_1 = slice_df_1.pivot_table(
    index='Bandwidth', 
    columns='Crossbar Size', 
    values='Delay',
    aggfunc='mean'  # Use mean if duplicates exist
)
heatmap_data_4 = slice_df_4.pivot_table(
    index='Bandwidth', 
    columns='Crossbar Size', 
    values='Delay',
    aggfunc='mean'  # Use mean if duplicates exist
)
heatmap_data_8 = slice_df_8.pivot_table(
    index='Bandwidth', 
    columns='Crossbar Size', 
    values='Delay',
    aggfunc='mean'  # Use mean if duplicates exist
)

fig, axes = plt.subplots(1, 3, figsize=(20, 6))

# 1. Apply logarithmic scaling
log_norm = LogNorm(vmin=max(1, min(heatmap_data_1.min().min(), heatmap_data_4.min().min(), heatmap_data_8.min().min())), 
                  vmax=max(heatmap_data_1.max().max(), heatmap_data_4.max().max(), heatmap_data_8.max().max()))

# 2. Configure plots
for ax, data, title in zip(axes, [heatmap_data_1, heatmap_data_4, heatmap_data_8], ["1-bit", "4-bit", "8-bit"]):
    sns.heatmap(data, ax=ax, cmap="rocket", annot=True, fmt=".0f",
                norm=log_norm, cbar=False,
                annot_kws={"size": 8}, linewidths=0.5)
    ax.set_title(f"Bit Precision = {title}")
    ax.set_xlabel("Crossbar Size")
    ax.set_ylabel("Bandwidth" if ax == axes[0] else "")
plt.tight_layout(rect=[0, 0, 0.85, 1])

# 3. Add single precise colorbar
cbar = fig.colorbar(axes[0].collections[0], ax=axes, location='right', pad=0.02)
cbar.set_label('Delay (log scale)', rotation=270, labelpad=10)
plt.savefig("enhanced_heatmaps_fc.png", dpi=300, bbox_inches='tight')
