import os
import re
import matplotlib.pyplot as plt
from collections import defaultdict

def extract_simulation_data(file_path):
    """Extract bandwidth, delay, crossbar size, and bit precision from a file."""
    with open(file_path, 'r') as file:
        content = file.read()

    data = {
        'bandwidth': None,
        'delay': None,
        'crossbar_size': None,
        'bit_precision': None,
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

    # Extract Bit Precision
    precision_match = re.search(r'Bit Precision:\s*(\d+)', content)
    if precision_match:
        data['bit_precision'] = int(precision_match.group(1))

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

        # Keep only the lowest delay per bandwidth
        for bw, bw_entries in bw_groups.items():
            min_delay_entry = min(bw_entries, key=lambda x: x['delay'])
            filtered_data[precision].append(min_delay_entry)

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
        output_filename = f'im2col_delay_ana/bandwidth_analysis_precision_{precision}.png'
        plt.tight_layout()
        plt.savefig(output_filename, dpi=300)
        print(f"Saved: {output_filename}")
        plt.close()

def main():
    folder_path = "./var_network_bw_with_cp_bw/cnn-im2col/"  # Update this path
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
    plot_and_save(filtered_data)

if __name__ == "__main__":
    main()