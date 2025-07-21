# DHCP Spoof Experiment Runner

This script automates running the DHCP spoof enhanced example with different parameter combinations and generates analysis plots.

## Prerequisites

1. **NS-3 Environment**: Make sure you're in the ns-3 directory with the compiled DHCP spoof example
2. **Python Dependencies**: Install required Python packages

```bash
pip install -r requirements.txt
```

## Usage

Run the experiment script from the ns-3 directory:

```bash
python3 run_dhcp_experiments.py
```

## What the Script Does

### 1. Parameter Combinations
The script tests 4 different values for each parameter:
- `nClients`: [10, 25, 50, 100]
- `nAddr`: [50, 100, 200, 400]
- `starvStopTime`: [2.0, 4.0, 6.0, 8.0] seconds
- `clientStartInterval`: [0.1, 0.3, 0.5, 0.8] seconds
- `starvInterval`: [2, 5, 10, 20] milliseconds

**Total combinations**: 4^5 = 1024 simulations

### 2. CSV Data Collection
- Removes any existing `dhcp-spoof-results.csv`
- Runs each simulation and appends results to CSV
- Records all parameters and the percentage of clients assigned rogue addresses

### 3. Plot Generation
Creates 4 PNG files (one for each `nAddr` value), each containing 4 subplots:

**For each nAddr value (50, 100, 200, 400):**
- **Top Left**: Starvation Stop Time vs Rogue Percentage
- **Top Right**: Client Start Interval vs Rogue Percentage  
- **Bottom Left**: Starvation Interval vs Rogue Percentage
- **Bottom Right**: Number of Clients vs Rogue Percentage

Each subplot shows 4 curves representing different `nClients` values.

## Output Files

1. **`dhcp-spoof-results.csv`**: Raw experiment data
2. **`output/dhcp_spoof_results_nAddr_50.png`**: Plots for nAddr=50
3. **`output/dhcp_spoof_results_nAddr_100.png`**: Plots for nAddr=100
4. **`output/dhcp_spoof_results_nAddr_200.png`**: Plots for nAddr=200
5. **`output/dhcp_spoof_results_nAddr_400.png`**: Plots for nAddr=400

## CSV Format

The CSV file contains these columns:
- `nClients`: Number of clients simulated
- `nAddr`: Number of legitimate DHCP addresses
- `starvationStopTime`: Starvation attack stop time (seconds)
- `clientStartInterval`: Interval between client starts (seconds)
- `starvationInterval`: Starvation attack interval (milliseconds)
- `rogueCount`: Number of clients with rogue addresses
- `legitimateCount`: Number of clients with legitimate addresses
- `noAddressCount`: Number of clients with no address
- `roguePercentage`: Percentage of clients with rogue addresses

## Expected Runtime

With 1024 total simulations, expect the script to run for several hours depending on your system performance. Each simulation typically takes 10-30 seconds.

## Troubleshooting

1. **NS-3 not found**: Make sure you're in the ns-3 directory with compiled examples
2. **Python dependencies**: Install with `pip install -r requirements.txt`
3. **Memory issues**: The script processes large datasets; ensure sufficient RAM
4. **Plot errors**: Check that matplotlib is properly installed

## Analysis

The generated plots help analyze:
- How starvation timing affects attack success
- Impact of client arrival patterns on rogue DHCP effectiveness
- Relationship between legitimate address pool size and attack success
- Effect of client population size on attack outcomes 