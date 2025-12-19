import subprocess
import numpy as np
import matplotlib.pyplot as plt

# client_counts = [50,60,70,80,90,100,110,120]
client_counts = [5,10,15,20]
runs_per_scenario = 10 # minutes

results = {}

print("-- Starting Simulation --")
print(f"Scenarios: {client_counts},\nRuns per scenario: {runs_per_scenario}")

for clients in client_counts:
    print(f"\n-- Simulating scenario with {clients} clients --", end="")
    losses = []

    for run in range(1, runs_per_scenario + 1):
        cmd = [
            "./ns3", "run",
            f'scratch/fleight_entertainment_system_simulation.cc --nClients={clients}'
        ]  

        process = subprocess.run(cmd, capture_output=True, text=True)
        output = process.stdout.strip().split('\n')
        
        found = False
        
        for line in output:
                if line.startswith("CSV_RESULT"):
                    parts = line.split(',')
                    if len(parts) == 4:
                        loss = float(parts[3])
                        losses.append(loss)
                        print(".", end="", flush=True)
                        found = True
                        break
        
        if not found:
            print(f"\n[Warning] No valid result found for clients={clients}, run={run}")
        
    if losses:
        results[clients] = np.mean(losses)
    else:
        results[clients] = 0.0
    
print("\n-- Simulation Complete --")

# Plotting the results
clients = list(results.keys())
avg_losses = list(results.values())

for c, loss in results.items():
    print(f"Clients: {c}, Average Packet Loss: {loss:.2f}%")
    
plt.figure(figsize=(10, 6))
plt.plot(clients, avg_losses, marker='o', linestyle='-', color='b', linewidth=2, markersize=8)

plt.title('Average Packet Loss vs Number of Clients', fontsize=16)
plt.xlabel('Number of Clients', fontsize=14)
plt.ylabel('Average Packet Loss (%)', fontsize=14)
plt.grid(True, linestyle='--', alpha=0.7)

plt.xticks(clients)
plt.ylim(bottom=0)

plt.savefig('packet_loss_vs_clients.png')
print("Plot saved as 'packet_loss_vs_clients.png'")
        