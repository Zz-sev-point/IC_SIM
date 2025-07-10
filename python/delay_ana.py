import os
import re
import matplotlib.pyplot as plt
from collections import defaultdict
import numpy as np

def extract_simulation_data(file_path):
    """Extract simulation parameters from files."""
    with open(file_path, 'r') as file:
        content = file.read()
    
    params = ['bandwidth', 'delay', 'crossbar_size', 'bit_precision']
    patterns = {
        'bandwidth': r'Bandwidth:\s*(\d+)',
        'delay': r'Delay:\s*(\d+)',
        'crossbar_size': r'Crossbar Size:\s*(\d+)\*\d+',
        'bit_precision': r'Bit Precision:\s*(\d+)'
    }
    
    data = {}
    for param in params:
        match = re.search(patterns[param], content)
        data[param] = int(match.group(1)) if match else None
    
    return data

def prepare_plot_data(data_entries, target_precision):
    """Organize data with strict bandwidth ordering."""
    # Filter and validate data
    valid_data = [d for d in data_entries 
                 if d['bit_precision'] == target_precision 
                 and all(d.values())]
    
    # Define required bandwidth sequence
    bandwidth_seq = [16, 32, 64, 128, 256, 512]
    
    plot_data = {}
    optimal_points = []
    
    for bw in bandwidth_seq:
        bw_data = [d for d in valid_data if d['bandwidth'] == bw]
        if not bw_data:
            continue
            
        # Find optimal (min delay) configuration
        optimal = min(bw_data, key=lambda x: x['delay'])
        optimal_points.append((optimal['crossbar_size'], optimal['delay']))
        
        # Prepare all points for this bandwidth
        bw_data.sort(key=lambda x: x['crossbar_size'])
        plot_data[bw] = {
            'crossbar_sizes': [d['crossbar_size'] for d in bw_data],
            'delays': [d['delay'] for d in bw_data]
        }
    
    return plot_data, list(zip(*optimal_points)) if optimal_points else ([], [])

def plot_delay_vs_crossbar(plot_data, optimal_points, target_precision):
    """Create the final plot with consistent star markers."""
    plt.figure(figsize=(12, 8))
    
    # Plot all bandwidth curves
    for bw in sorted(plot_data.keys()):
        data = plot_data[bw]
        line, = plt.plot(data['crossbar_sizes'], data['delays'],
                        marker='o', linestyle='-',
                        label=f'BW={bw} bits')
        
        # Highlight optimal point with star
        min_idx = np.argmin(data['delays'])
        plt.plot(data['crossbar_sizes'][min_idx], data['delays'][min_idx],
                marker='*', markersize=12, color=line.get_color(),
                markeredgecolor='k', linestyle='')
    
    # Plot optimal frontier with stars only
    opt_cs, opt_delays = optimal_points
    plt.plot(opt_cs, opt_delays, 'k--', marker='*', markersize=12,
             linewidth=1.5, label='Optimal Frontier')
    
    # Formatting
    plt.xlabel('Crossbar Size', fontsize=12)
    plt.ylabel('Delay (unit time)', fontsize=12)
    plt.title(f'Delay vs Crossbar Size ({target_precision}-bit Precision)', fontsize=14)
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend(fontsize=10, bbox_to_anchor=(1.02, 1), loc='upper left')
    plt.yscale('log')
    
    # Save output
    plt.tight_layout()
    output_file = f'im2col_delay_ana/delay_vs_crossbar_bp{target_precision}.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Saved: {output_file}")

def main():
    folder_path = "./var_network_bw_with_cp_bw/cnn-im2col/"
    
    # Load and process data
    data_entries = []
    for filename in os.listdir(folder_path):
        if filename.endswith(".txt"):
            data = extract_simulation_data(os.path.join(folder_path, filename))
            data_entries.append(data)
    
    if not data_entries:
        print("No valid data found!")
        return
    for target_precision in [1, 4, 8]:
        plot_data, optimal_points = prepare_plot_data(data_entries, target_precision)
        plot_delay_vs_crossbar(plot_data, optimal_points, target_precision)

if __name__ == "__main__":
    main()