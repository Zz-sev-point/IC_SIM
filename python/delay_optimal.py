import os
import re
import matplotlib.pyplot as plt
from collections import defaultdict
import numpy as np

def extract_simulation_data(file_path):
    """Extract simulation parameters from files with error handling."""
    with open(file_path, 'r') as file:
        content = file.read()
    
    data = {
        'bandwidth': None,
        'delay': None,
        'crossbar_size': None,
        'bit_precision': None,
    }

    try:
        # Extract with regex
        data['bandwidth'] = int(re.search(r'Bandwidth:\s*(\d+)', content).group(1))
        data['delay'] = int(re.search(r'Delay:\s*(\d+)', content).group(1))
        data['crossbar_size'] = int(re.search(r'Crossbar Size:\s*(\d+)\*\d+', content).group(1))
        data['bit_precision'] = int(re.search(r'Bit Precision:\s*(\d+)', content).group(1))
    except (AttributeError, ValueError):
        return None
    
    return data if all(data.values()) else None

def find_optimal_configs(data_entries, target_precision):
    """Find optimal (smallest crossbar @ lowest delay) for each bandwidth."""
    # Filter valid data
    valid_data = [d for d in data_entries if d and d['bit_precision'] == target_precision]
    
    # Group by bandwidth
    bw_groups = defaultdict(list)
    for entry in valid_data:
        bw_groups[entry['bandwidth']].append(entry)
    
    # Process each bandwidth group
    plot_data = {}
    optimal_points = []
    bandwidth_order = [16, 32, 64, 128, 256, 512, 1024]  # Enforced order
    
    for bw in bandwidth_order:
        if bw not in bw_groups:
            continue
            
        # Find minimal delay
        min_delay = min(e['delay'] for e in bw_groups[bw])
        
        # Among minimal delay configs, select smallest crossbar
        candidates = [e for e in bw_groups[bw] if e['delay'] == min_delay]
        optimal_config = min(candidates, key=lambda x: x['crossbar_size'])
        
        optimal_points.append((optimal_config['crossbar_size'], optimal_config['delay']))
        
        # Prepare all points for plotting
        bw_groups[bw].sort(key=lambda x: x['crossbar_size'])
        plot_data[bw] = {
            'crossbar_sizes': [e['crossbar_size'] for e in bw_groups[bw]],
            'delays': [e['delay'] for e in bw_groups[bw]]
        }
    
    return plot_data, list(zip(*optimal_points)) if optimal_points else ([], [])

def generate_plot(plot_data, optimal_points, target_precision):
    """Generate the final optimized plot."""
    plt.figure(figsize=(13, 8))
    
    # Plot all bandwidth curves
    for bw in sorted(plot_data.keys()):
        data = plot_data[bw]
        line, = plt.plot(data['crossbar_sizes'], data['delays'],
                        marker='o', linestyle='-', alpha=0.7,
                        label=f'BW={bw} bits')
        
        # Mark optimal point (smallest crossbar @ lowest delay)
        min_delay = min(data['delays'])
        min_indices = [i for i, d in enumerate(data['delays']) if d == min_delay]
        smallest_cs = min([data['crossbar_sizes'][i] for i in min_indices])
        opt_idx = data['crossbar_sizes'].index(smallest_cs)
        
        plt.plot(data['crossbar_sizes'][opt_idx], data['delays'][opt_idx],
                marker='*', markersize=15, color=line.get_color(),
                markeredgecolor='k', linestyle='')

    # Plot optimal frontier
    opt_cs, opt_delays = optimal_points
    plt.plot(opt_cs, opt_delays, 'k--', marker='*', markersize=12,
             linewidth=1.5, label='Optimal Frontier')
    
    # Formatting
    plt.xlabel('Crossbar Size', fontsize=13)
    plt.ylabel('Delay (unit time)', fontsize=13)
    plt.title(f'Delay vs Crossbar Size ({target_precision}-bit Precision)\n'
              f'★ = Optimal (Smallest Crossbar @ Lowest Delay)', fontsize=14)
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend(fontsize=11, bbox_to_anchor=(1.02, 1), loc='upper left')
    plt.yscale('log')
    
    # Save and show
    plt.tight_layout()
    output_file = f'./fc_delay_ana/delay_vs_crossbar_bp{target_precision}.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Saved: {output_file}")

def main():
    folder_path = "./var_network_bw_with_cp_bw/fc/"
    
    # Load data
    data_entries = []
    for filename in os.listdir(folder_path):
        if filename.endswith(".txt"):
            data = extract_simulation_data(os.path.join(folder_path, filename))
            if data:
                data_entries.append(data)
    
    if not data_entries:
        print("No valid data found!")
        return
    
    # Process and plot
    for target_precision in [1, 4, 8]:
        plot_data, optimal_points = find_optimal_configs(data_entries, target_precision)
        generate_plot(plot_data, optimal_points, target_precision)

if __name__ == "__main__":
    main()
