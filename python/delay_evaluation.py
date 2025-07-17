import os
import re
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict
from matplotlib import rcParams
from matplotlib.ticker import ScalarFormatter

def extract_simulation_data(file_path):
    """Extract all relevant parameters from simulation files."""
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
    bits_match = re.search(r'Total Bits transferred:\s*(\d+)', content)
    if bits_match:
        data['total_bits'] = int(bits_match.group(1))

    return data if all(val is not None for val in data.values()) else None

def calculate_metric_ranges(data_entries):
    """Automatically determine normalization ranges from data"""
    metrics = ['delay', 'crossbar_size', 'crossbar_amount', 'total_bits']
    ranges = {}
    
    # Initialize with zeros
    for metric in metrics:
        ranges[metric] = {'max': 0}
    
    # Find maximum observed values
    for entry in data_entries:
        if entry is None:
            continue
        for metric in metrics:
            if entry[metric] > ranges[metric]['max']:
                ranges[metric]['max'] = entry[metric]
    
    # Add utilization (already normalized)
    ranges['utilization'] = {'max': 1.0}
    
    # Apply safety margins (10% buffer)
    for metric in metrics:
        ranges[metric]['max'] *= 1.1
    
    return ranges

def evaluate_configuration(entry, weights, metric_ranges):
    """Evaluation with auto-scaling normalization"""
    # Normalize metrics (0-1 range)
    norm_delay = 1 - min(entry['delay'] / metric_ranges['delay']['max'], 1)
    norm_size = 1 - min(entry['crossbar_size'] / metric_ranges['crossbar_size']['max'], 1)
    norm_amount = 1 - min(entry['crossbar_amount'] / metric_ranges['crossbar_amount']['max'], 1)
    norm_util = entry['utilization']  # Already 0-1
    norm_bits = 1 - min(entry['total_bits'] / metric_ranges['total_bits']['max'], 1)
    
    # Weighted sum
    score = (weights['delay'] * norm_delay +
             weights['size'] * norm_size +
             weights['amount'] * norm_amount +
             weights['util'] * norm_util +
             weights['bits'] * norm_bits)
    
    return score

def filter_and_evaluate(data_entries, weights):
    """Enhanced evaluation with automatic range detection"""
    # First pass: calculate ranges from all data
    metric_ranges = calculate_metric_ranges(data_entries)
    
    # Second pass: evaluate configurations
    precision_groups = defaultdict(list)
    for entry in data_entries:
        if entry is not None:
            precision_groups[entry['bit_precision']].append(entry)

    filtered_data = defaultdict(list)
    evaluation_scores = defaultdict(list)
    
    for precision, entries in precision_groups.items():
        bw_groups = defaultdict(list)
        for entry in entries:
            bw_groups[entry['bandwidth']].append(entry)

        for bw, bw_entries in bw_groups.items():
            min_delay = min(e['delay'] for e in bw_entries)
            min_delay_entries = [e for e in bw_entries if e['delay'] == min_delay]
            optimal_entry = min(min_delay_entries, key=lambda x: x['crossbar_size'])
            
            score = evaluate_configuration(optimal_entry, weights, metric_ranges)
            print(precision, optimal_entry['bandwidth'], score)
            
            filtered_data[precision].append(optimal_entry)
            evaluation_scores[precision].append(score)

    return filtered_data, evaluation_scores, metric_ranges

def plot_evaluation_results(filtered_data, evaluation_scores, weights, metric_ranges):
    """Plot results with automatic ranges annotation"""
    for precision in filtered_data.keys():
        entries = filtered_data[precision]
        scores = evaluation_scores[precision]
        
        combined = sorted(zip(entries, scores), key=lambda x: x[0]['bandwidth'])
        entries, scores = zip(*combined)
        
        bandwidths = [e['bandwidth'] for e in entries]
        delays = [e['delay'] for e in entries]
        
        fig, ax1 = plt.subplots(figsize=(12, 6))
        title = (f'Configuration Evaluation ({precision}-bit Precision)\n'
                f'Weights: {weights}\n'
                f"Normalization ranges: Delay≤{metric_ranges['delay']['max']:.1f}, "
                f"Size≤{metric_ranges['crossbar_size']['max']}, "
                f"Amount≤{metric_ranges['crossbar_amount']['max']}, "
                f"Bits≤{metric_ranges['total_bits']['max']:.1e}")
        fig.suptitle(title, fontsize=12, y=0.98)

        color = 'tab:blue'
        ax1.set_xlabel('Bandwidth (bits/unit time)')
        ax1.set_ylabel('Delay (unit time)', color=color)
        line1 = ax1.plot(bandwidths, delays, 'o-', color=color, label='Delay')
        ax1.tick_params(axis='y', labelcolor=color)
        ax1.grid(True, which="both", ls="--", alpha=0.3)

        ax2 = ax1.twinx()
        color = 'tab:red'
        ax2.set_ylabel('Evaluation Score', color=color)
        line2 = ax2.plot(bandwidths, scores, 's--', color=color, label='Evaluation Score')
        ax2.tick_params(axis='y', labelcolor=color)

        lines = line1 + line2
        labels = [l.get_label() for l in lines]
        ax1.legend(lines, labels, loc='upper center', 
                  bbox_to_anchor=(0.5, -0.2), ncol=2)

        plt.tight_layout()
        output_filename = f'k2col_delay_ana/evaluation_precision_{precision}.png'
        plt.savefig(output_filename, dpi=300, bbox_inches='tight')
        plt.close()

def main():
    folder_path = "./var_network_bw_with_cp_bw/cnn-k2col/"
    data_entries = []

    weights = {
        'delay': 0.2,
        'size': 0.2,
        'amount': 0.2,
        'util': 0.2,
        'bits': 0.2
    }

    for filename in sorted(os.listdir(folder_path)):
        if filename.endswith(".txt"):
            file_path = os.path.join(folder_path, filename)
            data = extract_simulation_data(file_path)
            if data:
                data_entries.append(data)

    if not data_entries:
        print("No valid simulation files found!")
        return

    filtered_data, evaluation_scores, metric_ranges = filter_and_evaluate(data_entries, weights)
    plot_evaluation_results(filtered_data, evaluation_scores, weights, metric_ranges)

if __name__ == "__main__":
    main()