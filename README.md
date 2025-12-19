# ✈️ In-Flight Entertainment System Simulation (ns-3)

A network simulation project using **ns-3** to model and analyze packet loss in an in-flight entertainment system. This project simulates a star topology where passengers (clients) stream media from a central server under bandwidth constraints.

## 📋 Scenario Description

This simulation models a scenario inside an aircraft where:

- **Topology:** Star topology with a central **Bottleneck Router**.
- **Nodes:** 1 Server, 1 Router, and multiple Clients (50 to 120 passengers).
- **Traffic:** UDP traffic generated using an **On/Off Application** model to simulate user streaming behavior (random viewing/idling times).
- **Constraints:**
  - Link Bandwidth: 10 Mbps
  - Link Delay: 3ms (6ms RTT)
  - Server Rate per Client: 1 Mbps

### Key Objectives

The goal is to analyze how increasing the number of active clients impacts the **Packet Loss Ratio** due to network congestion at the bottleneck link.

## 🛠️ Technologies Used

- **Simulator:** [ns-3](https://www.nsnam.org/) (Network Simulator 3)
- **Language:** C++ (Core Simulation)
- **Automation & Plotting:** Python 3, Matplotlib, NumPy
- **Protocol:** IPv4, UDP

## 🚀 How to Run

### Prerequisites

1.  ns-3 installed (e.g., ns-3.45 or later).
2.  Python 3 with `matplotlib` and `numpy` installed.

### Installation

1.  Clone this repository:
    ```bash
    git clone [https://github.com/YOUR_USERNAME/Flight-Entertainment-Simulation.git](https://github.com/YOUR_USERNAME/Flight-Entertainment-Simulation.git)
    ```
2.  Copy the simulation code to your ns-3 scratch folder:
    ```bash
    cp src/flight_entertainment_system.cc /path/to/ns-3-dev/scratch/
    ```
3.  Copy the automation script to the ns-3 root folder:
    ```bash
    cp scripts/plot_results.py /path/to/ns-3-dev/
    ```

### Execution

Run the Python automation script from the ns-3 root directory. This script will compile the C++ code, run simulations for different numbers of clients (batch processing), and generate the results graph.

```bash
python3 plot_results.py
```
