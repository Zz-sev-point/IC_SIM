import os
import re
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict
from matplotlib import rcParams
from matplotlib.ticker import ScalarFormatter, FuncFormatter

def extract_simulation_data(file_path):
    """Extract bandwidth, delay, crossbar size, and bit precision from a file."""
    with open(file_path, 'r') as file:
        content = file.read()

    data = {
        'bandwidth': None,
        'delay': None,
        'crossbar_size': None,
        'crossbar_amount': None,
        'bit_precision': None,
        'utilization': None,
        'total_bits': None,
    }

    # Extract Bandwidth
    bw_match = re.search(r'Bandwidth:\s*(\d+)\s*bits per unit time', content)
    if bw_match:
        data['bandwidth'] = int(bw_match.group(1))

    # Extract Delay
    delay_match = re.search(r'Delay:\s*(\d+)\s*unit time', content)
    if delay_match:
        data['delay'] = int(delay_match.group(1))

    # Extract Crossbar Size
    crossbar_match = re.search(r'Crossbar Size:\s*(\d+)\*\d+', content)
    if crossbar_match:
        data['crossbar_size'] = int(crossbar_match.group(1))

    # Extract Crossbar Amount
    amount_match = re.search(r'Crossbar Amount:\s*(\d+)', content)
    if amount_match:
        data['crossbar_amount'] = int(amount_match.group(1))

    # Extract Bit Precision
    precision_match = re.search(r'Bit Precision:\s*(\d+)', content)
    if precision_match:
        data['bit_precision'] = int(precision_match.group(1))

    # Extract Crossbar Utilization
    utilization_match = re.search(r'Crossbar Usage Proportion:\s*([\d.]+)', content)
    if utilization_match:
        data['utilization'] = float(utilization_match.group(1))

    # Extract Total Transferred Bits
    bits_match = re.search(r'Total Bits transferred:\s*([\d.]+)', content)
    if bits_match:
        data['total_bits'] = int(bits_match.group(1))

    return data

def filter_lowest_delay_by_precision(data_entries):
    """Group by bit precision, then for each bandwidth keep the lowest delay entry."""
    precision_groups = defaultdict(list)
    for entry in data_entries:
        if all(val is not None for val in entry.values()):
            precision_groups[entry['bit_precision']].append(entry)

    filtered_data = defaultdict(list)
    for precision, entries in precision_groups.items():
        # Group by bandwidth for this precision
        bw_groups = defaultdict(list)
        for entry in entries:
            bw_groups[entry['bandwidth']].append(entry)

        # For each bandwidth, find minimal delay config with smallest crossbar
        for bw, bw_entries in bw_groups.items():
            # Find minimum delay
            min_delay = min(entry['delay'] for entry in bw_entries)
            
            # Filter entries with this delay
            min_delay_entries = [e for e in bw_entries if e['delay'] == min_delay]
            
            # Among these, select the one with smallest crossbar
            optimal_entry = min(min_delay_entries, key=lambda x: x['crossbar_size'])
            filtered_data[precision].append(optimal_entry)

    return filtered_data

def plot_and_save(filtered_data):
    """Generate and save plots for each bit precision."""
    for precision, entries in filtered_data.items():
        if not entries:
            continue

        # Sort by bandwidth
        entries.sort(key=lambda x: x['bandwidth'])
        bandwidths = [e['bandwidth'] for e in entries]
        delays = [e['delay'] for e in entries]
        crossbar_sizes = [e['crossbar_size'] for e in entries]

        # Create figure with two subplots
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 10))
        fig.suptitle(f'Bandwidth Analysis (Bit Precision = {precision})', fontsize=14)

        # Subplot 1: Bandwidth vs. Lowest Delay
        ax1.plot(bandwidths, delays, 'bo-', label='Lowest Delay')
        # ax1.set_xscale('log')
        # ax1.set_yscale('log')
        ax1.set_ylabel('Lowest Delay (unit time)', color='b')
        ax1.tick_params(axis='y', labelcolor='b')
        ax1.grid(True, which="both", ls="--")
        ax1.legend(loc='upper right')

        # Subplot 2: Bandwidth vs. Crossbar Size
        ax2.plot(bandwidths, crossbar_sizes, 'rs--', label='Crossbar Size')
        # ax2.set_xscale('log')
        ax2.set_xlabel('Bandwidth (bits/unit time)')
        ax2.set_ylabel('Crossbar Size', color='r')
        ax2.tick_params(axis='y', labelcolor='r')
        ax2.grid(True, which="both", ls="--")
        ax2.legend(loc='upper right')

        # Save the figure
        output_filename = f'fc_delay_ana/bandwidth_analysis_precision_{precision}.png'
        plt.tight_layout()
        plt.savefig(output_filename, dpi=300)
        print(f"Saved: {output_filename}")
        plt.close()

def plot_combined_chart(filtered_data):
    """Generate a single chart with two y-axes for delay and crossbar size."""
    for precision, entries in filtered_data.items():
        if not entries:
            continue

        # Sort by bandwidth
        entries.sort(key=lambda x: x['bandwidth'])
        bandwidths = [e['bandwidth'] for e in entries]
        delays = [e['delay'] for e in entries]
        crossbar_sizes = [e['crossbar_size'] for e in entries]

        # Create figure with dual y-axes
        fig, ax1 = plt.subplots(figsize=(10, 6))
        fig.suptitle(f'Bandwidth Analysis (Bit Precision = {precision})', fontsize=14)

        # Plot Delay on primary axis (left)
        color = 'tab:blue'
        ax1.set_xlabel('Bandwidth (bits/unit time)')
        ax1.set_ylabel('Lowest Delay (unit time)', color=color)
        line1 = ax1.plot(bandwidths, delays, 'o-', color=color, label='Lowest Delay')
        ax1.tick_params(axis='y', labelcolor=color)
        ax1.grid(True, which="both", ls="--")

        # Create second y-axis for Crossbar Size
        ax2 = ax1.twinx()
        color = 'tab:red'
        ax2.set_ylabel('Crossbar Size', color=color)
        line2 = ax2.plot(bandwidths, crossbar_sizes, 's--', color=color, label='Crossbar Size')
        ax2.tick_params(axis='y', labelcolor=color)

        # Combine legends from both axes
        lines = line1 + line2
        labels = [l.get_label() for l in lines]
        ax1.legend(lines, labels, loc='upper right')

        # Save the figure
        output_filename = f'k2col_delay_ana/combined_analysis_precision_{precision}.png'
        plt.tight_layout()
        plt.savefig(output_filename, dpi=300)
        print(f"Saved: {output_filename}")
        plt.close()
    
def plot_combined_complete_chart(filtered_data):
    """Generate a single chart with five y-axes with proper scientific notation."""
    # Configure plot style
    rcParams.update({
        'font.size': 12,
        'axes.titlepad': 20
    })
    
    # Custom formatter for scientific notation
    def sci_formatter(x, pos):
        if x == 0:
            return "0"
        exponent = int(np.log10(abs(x)))
        coeff = x / 10**exponent
        return f"{coeff:g}×10$^{{{exponent}}}$"

    for precision, entries in filtered_data.items():
        if not entries:
            continue

        # Sort by bandwidth
        entries.sort(key=lambda x: x['bandwidth'])
        bandwidths = [e['bandwidth'] for e in entries]
        delays = [e['delay'] for e in entries]
        crossbar_sizes = [e['crossbar_size'] for e in entries]
        crossbar_amounts = [e['crossbar_amount'] for e in entries]
        utilizations = [e['utilization'] for e in entries]
        total_bits = [e['total_bits'] for e in entries]

        # Create figure with primary y-axis
        fig, ax1 = plt.subplots(figsize=(16, 8))
        fig.suptitle(f'Comprehensive Bandwidth Analysis ({precision}-bit Precision)', 
                    fontsize=14, y=0.96)

        # Plot Delay on primary axis (left)
        color = 'tab:blue'
        ax1.set_xlabel('Bandwidth (bits/unit time)', fontsize=12)
        ax1.set_ylabel('Delay (unit time)', color=color, fontsize=12)
        line1 = ax1.plot(bandwidths, delays, 'o-', color=color, label='Delay')
        ax1.tick_params(axis='y', labelcolor=color, labelsize=10)
        ax1.grid(True, which="both", ls="--", alpha=0.3)

        # Create additional y-axes (right side) with increased spacing
        axes = [ax1]
        colors = ['tab:red', 'tab:green', 'tab:purple', 'tab:orange']
        styles = ['s--', '^:', 'D-.', 'v-']
        labels = ['Crossbar Size', 'Utilization (%)', 'Crossbar Amount', 'Total Bits']
        data_sets = [
            crossbar_sizes,
            [u*100 for u in utilizations],
            crossbar_amounts,
            total_bits
        ]
        scale_factors = [1, 1, 1, 1e-6]  # Scale total bits to millions
        
        # Increased base offset and spacing between axes
        base_offset = 80
        spacing = 80
        
        for i, (color, style, label, data, scale) in enumerate(zip(
            colors, styles, labels, data_sets, scale_factors)):
            
            ax = ax1.twinx()
            ax.spines['right'].set_position(('outward', base_offset + spacing*i))
            ax.set_ylabel(label, color=color, fontsize=12)
            
            scaled_data = [d/scale for d in data]
            line = ax.plot(bandwidths, scaled_data, style, color=color, label=label)
            
            # Special formatting for crossbar amount
            if label == 'Crossbar Amount':
                ax.yaxis.set_major_formatter(FuncFormatter(sci_formatter))
            elif scale != 1:
                ax.set_ylabel(f"{label} (×10^{int(np.log10(scale))})", color=color, fontsize=12)
            
            ax.tick_params(axis='y', labelcolor=color, labelsize=10)
            axes.append(ax)

        # Combine legends and place below with more space
        lines = []
        for ax in axes:
            lines += ax.get_lines()
        
        labels = [line.get_label() for line in lines]
        fig.legend(lines, labels, loc='upper center', 
                 bbox_to_anchor=(0.5, -0.1), ncol=5, fontsize=10)

        plt.tight_layout()
        
        # Save the figure
        os.makedirs('k2col_delay_ana', exist_ok=True)
        output_filename = f'fc_delay_ana/combined_analysis_precision_{precision}.png'
        plt.savefig(output_filename, dpi=300, bbox_inches='tight')
        print(f"Saved: {output_filename}")
        plt.close()

def main():
    folder_path = "./var_network_bw_with_cp_bw/fc/"  # Update this path
    data_entries = []

    # Read all simulation files
    for filename in os.listdir(folder_path):
        if filename.endswith(".txt"):
            file_path = os.path.join(folder_path, filename)
            data = extract_simulation_data(file_path)
            data_entries.append(data)

    if not data_entries:
        print("No valid simulation files found!")
        return

    # Filter data by precision and bandwidth
    filtered_data = filter_lowest_delay_by_precision(data_entries)
    plot_combined_complete_chart(filtered_data)
    # plot_and_save(filtered_data)

if __name__ == "__main__":
    main()

