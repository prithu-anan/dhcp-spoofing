#!/usr/bin/env python3
"""
DHCP Spoof Experiment Runner and Plot Generator

This script runs the DHCP spoof enhanced example with different parameter combinations
and generates plots showing the relationship between parameters and rogue address percentage.
"""

import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import sys
from itertools import product

def remove_csv_file():
    """Remove the existing CSV file if it exists."""
    if os.path.exists("dhcp-spoof-results.csv"):
        os.remove("dhcp-spoof-results.csv")
        print("Removed existing dhcp-spoof-results.csv")

def run_simulation(nClients, nAddr, starvStopTime, clientStartInterval, starvInterval, logEnabled=False):
    """Run a single simulation with the given parameters."""
    cmd = [
        "./ns3", "run", 
        "dhcp-spoof-enhanced-example",
        "--",
        f"--nClients={nClients}",
        f"--nAddr={nAddr}",
        f"--starvStopTime={starvStopTime}",
        f"--clientStartInterval={clientStartInterval}",
        f"--starvInterval={starvInterval}"
    ]
    
    if logEnabled:
        cmd.append("--logEnabled=true")
    
    print(f"Running: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        print(f"✓ Simulation completed successfully")
        return True
    except subprocess.CalledProcessError as e:
        print(f"✗ Simulation failed: {e}")
        print(f"Error output: {e.stderr}")
        return False

def run_all_experiments():
    """Run all parameter combinations."""
    # Parameter values to test
    nClients_values = [10, 20, 30, 40]
    nAddr_values = [50, 100, 150, 200]
    starvStopTime_values = [2.0, 4.0, 6.0, 8.0]
    clientStartInterval_values = [0.2, 0.4, 0.6, 0.8]
    starvInterval_values = [5, 10, 15, 20]
    
    # Remove existing CSV file
    remove_csv_file()
    
    # Run all combinations
    total_combinations = len(nClients_values) * len(nAddr_values) * len(starvStopTime_values) * len(clientStartInterval_values) * len(starvInterval_values)
    current_combination = 0
    
    print(f"Starting experiments with {total_combinations} total combinations...")
    
    for nClients in nClients_values:
        for nAddr in nAddr_values:
            for starvStopTime in starvStopTime_values:
                for clientStartInterval in clientStartInterval_values:
                    for starvInterval in starvInterval_values:
                        current_combination += 1
                        print(f"\n[{current_combination}/{total_combinations}] Testing parameters:")
                        print(f"  nClients={nClients}, nAddr={nAddr}, starvStopTime={starvStopTime}, clientStartInterval={clientStartInterval}, starvInterval={starvInterval}")
                        
                        success = run_simulation(nClients, nAddr, starvStopTime, clientStartInterval, starvInterval)
                        if not success:
                            print("Skipping remaining combinations due to failure")
                            return False
    
    print(f"\n✓ All {total_combinations} experiments completed successfully!")
    return True

def load_and_validate_data():
    """Load the CSV data and validate it."""
    if not os.path.exists("dhcp-spoof-results.csv"):
        print("Error: dhcp-spoof-results.csv not found!")
        return None
    
    try:
        df = pd.read_csv("dhcp-spoof-results.csv")
        print(f"Loaded {len(df)} records from CSV file")
        print(f"Columns: {list(df.columns)}")
        return df
    except Exception as e:
        print(f"Error loading CSV file: {e}")
        return None

def create_plots(df):
    """Create the requested plots."""
    if df is None or df.empty:
        print("No data to plot!")
        return
    
    # Parameter values used in experiments
    nClients_values = sorted(df['nClients'].unique())
    nAddr_values = sorted(df['nAddr'].unique())
    starvStopTime_values = sorted(df['starvationStopTime'].unique())
    clientStartInterval_values = sorted(df['clientStartInterval'].unique())
    starvInterval_values = sorted(df['starvationInterval'].unique())
    
    print(f"Found parameter values:")
    print(f"  nClients: {nClients_values}")
    print(f"  nAddr: {nAddr_values}")
    print(f"  starvStopTime: {starvStopTime_values}")
    print(f"  clientStartInterval: {clientStartInterval_values}")
    print(f"  starvInterval: {starvInterval_values}")
    
    # Create plots for each nAddr value (4 plots total)
    for nAddr in nAddr_values:
        create_subplot_for_naddr(df, nAddr, nClients_values)

def create_subplot_for_naddr(df, nAddr, nClients_values):
    """Create a 2x2 subplot for a specific nAddr value."""
    # Filter data for this nAddr
    df_naddr = df[df['nAddr'] == nAddr]
    
    if df_naddr.empty:
        print(f"No data for nAddr={nAddr}")
        return
    
    # Create output directory if it doesn't exist
    output_dir = "output"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"Created output directory: {output_dir}")
    
    # Create figure with 2x2 subplots
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    fig.suptitle(f'DHCP Spoof Attack Results (nAddr={nAddr})', fontsize=16)
    
    # Plot 1: starvStopTime vs roguePercentage
    ax1 = axes[0, 0]
    for nClients in nClients_values:
        data = df_naddr[df_naddr['nClients'] == nClients]
        if not data.empty:
            # Group by starvStopTime and calculate mean
            grouped = data.groupby('starvationStopTime')['roguePercentage'].mean()
            ax1.plot(grouped.index, grouped.values, 'o-', label=f'nClients={nClients}')
    ax1.set_xlabel('Starvation Stop Time (s)')
    ax1.set_ylabel('Rogue Address Percentage (%)')
    ax1.set_title('Starvation Stop Time vs Rogue Percentage')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: clientStartInterval vs roguePercentage
    ax2 = axes[0, 1]
    for nClients in nClients_values:
        data = df_naddr[df_naddr['nClients'] == nClients]
        if not data.empty:
            # Group by clientStartInterval and calculate mean
            grouped = data.groupby('clientStartInterval')['roguePercentage'].mean()
            ax2.plot(grouped.index, grouped.values, 'o-', label=f'nClients={nClients}')
    ax2.set_xlabel('Client Start Interval (s)')
    ax2.set_ylabel('Rogue Address Percentage (%)')
    ax2.set_title('Client Start Interval vs Rogue Percentage')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # Plot 3: starvInterval vs roguePercentage
    ax3 = axes[1, 0]
    for nClients in nClients_values:
        data = df_naddr[df_naddr['nClients'] == nClients]
        if not data.empty:
            # Group by starvInterval and calculate mean
            grouped = data.groupby('starvationInterval')['roguePercentage'].mean()
            ax3.plot(grouped.index, grouped.values, 'o-', label=f'nClients={nClients}')
    ax3.set_xlabel('Starvation Interval (ms)')
    ax3.set_ylabel('Rogue Address Percentage (%)')
    ax3.set_title('Starvation Interval vs Rogue Percentage')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    # Plot 4: nClients vs roguePercentage (for this nAddr)
    ax4 = axes[1, 1]
    # Group by nClients and calculate mean
    grouped = df_naddr.groupby('nClients')['roguePercentage'].mean()
    ax4.plot(grouped.index, grouped.values, 'o-', color='red', linewidth=2, markersize=8)
    ax4.set_xlabel('Number of Clients')
    ax4.set_ylabel('Rogue Address Percentage (%)')
    ax4.set_title('Number of Clients vs Rogue Percentage')
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Save the plot in output directory
    filename = os.path.join(output_dir, f'dhcp_spoof_results_nAddr_{nAddr}.png')
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f"Saved plot: {filename}")
    plt.close()

def main():
    """Main function to run experiments and generate plots."""
    print("DHCP Spoof Experiment Runner and Plot Generator")
    print("=" * 50)
    
    # Check if ns3 is available
    if not os.path.exists("./ns3"):
        print("Error: ./ns3 executable not found in current directory!")
        print("Please run this script from the ns-3 directory.")
        return
    
    # Run all experiments
    print("\nStep 1: Running all experiments...")
    success = run_all_experiments()
    
    if not success:
        print("Experiments failed. Exiting.")
        return
    
    # Load and validate data
    print("\nStep 2: Loading experiment data...")
    df = load_and_validate_data()
    
    if df is None:
        print("Failed to load data. Exiting.")
        return
    
    # Create plots
    print("\nStep 3: Generating plots...")
    create_plots(df)
    
    print("\n✓ All tasks completed successfully!")
    print("Generated files:")
    print("  - dhcp-spoof-results.csv (experiment data)")
    print("  - output/dhcp_spoof_results_nAddr_*.png (plots)")

if __name__ == "__main__":
    main() 