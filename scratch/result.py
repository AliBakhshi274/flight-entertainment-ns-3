import pandas as pd
import matplotlib.pyplot as plt
import io

with open("results.csv", "r") as file:
    content = file.read()

lines = content.strip().split('\n')
header = lines[0].split(',')

data = []
for line in lines[1:]:
    parts = line.replace(' ', '').split(',')
    if len(parts) == len(header):
        data.append(parts)

df = pd.DataFrame(data, columns=header)

numeric_columns = ['MeanIPD', 'OfferedLoad', 'QueueCap', 'AvgQueue', 'LossRate', 'Delay', 'Throughput']
df[numeric_columns] = df[numeric_columns].apply(pd.to_numeric)

# Filter by a fixed queue size (50 packets, ...) to vary the load (Task 2)
df_fixed_queue = df[df['QueueCap'] == 50].sort_values(by='OfferedLoad')

# Plot 1: Offered Load vs. Packet Loss (Task 2)
plt.figure(figsize=(10, 5))
plt.plot(df_fixed_queue['OfferedLoad'], df_fixed_queue['LossRate'], marker='o', label='Queue=50')
plt.axvline(x=5.0, color='r', linestyle='--', label='Bottleneck Capacity (5 Mbps)')
plt.title('Packet Loss Rate vs. Offered Load')
plt.xlabel('Offered Load (Mbps)')
plt.ylabel('Packet Loss Rate (%)')
plt.grid(True)
plt.legend()
plt.savefig('plot_loss.png')
plt.show()

# Plot 2: Offered Load vs. Average Delay (Task 2)
plt.figure(figsize=(10, 5))
plt.plot(df_fixed_queue['OfferedLoad'], df_fixed_queue['Delay'], marker='o', color='orange', label='Queue=50')
plt.axvline(x=5.0, color='r', linestyle='--', label='Bottleneck Capacity')
plt.title('Average Delay vs. Offered Load')
plt.xlabel('Offered Load (Mbps)')
plt.ylabel('Average Delay (ms)')
plt.grid(True)
plt.legend()
plt.savefig('plot_delay.png')
plt.show()

# Plot 3: Queue Capacity vs. Avg Queue Size & Delay (Task 3)
high_load = df[df['OfferedLoad'] > 6.0].iloc[0]['OfferedLoad']
df_var_queue = df[abs(df['OfferedLoad'] - high_load) < 0.5].sort_values(by='QueueCap')

plt.figure(figsize=(10, 5))
plt.plot(df_var_queue['QueueCap'], df_var_queue['AvgQueue'], marker='s', color='green')
plt.title(f'Average Queue Size vs. Queue Capacity (at ~{high_load:.1f} Mbps Load)')
plt.xlabel('Queue Capacity (Packets)')
plt.ylabel('Average Queue Fill Level (Packets)')
plt.grid(True)
plt.savefig('plot_queue.png')
plt.show()