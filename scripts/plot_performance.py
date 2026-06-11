import csv
import matplotlib.pyplot as plt

INPUT_FILE = "results/timing_results.csv"

processes = []
times = []
speedups = []
efficiencies = []

with open(INPUT_FILE, "r", newline="") as file:
    reader = csv.DictReader(file)

    for row in reader:
        if row["Model"] == "MPI":
            processes.append(int(row["Processes"]))
            times.append(float(row["Time"]))
            speedups.append(float(row["Speedup"]))
            efficiencies.append(float(row["Efficiency"]))

# Gráfica 1: tiempo
plt.figure(figsize=(9, 6))
plt.plot(processes, times, marker="o", linewidth=2)

for i in range(len(processes)):
    plt.annotate(
        f"{times[i]:.3f}s",
        (processes[i], times[i]),
        textcoords="offset points",
        xytext=(0, 8),
        ha="center"
    )

plt.title("MPI Distributed Search Runtime")
plt.xlabel("Number of MPI Processes")
plt.ylabel("Time in Seconds")
plt.xticks(processes)
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig("results/runtime_results.png", dpi=300)

# Gráfica 2: speedup
plt.figure(figsize=(9, 6))
plt.plot(processes, speedups, marker="o", linewidth=2)

for i in range(len(processes)):
    plt.annotate(
        f"{speedups[i]:.2f}x",
        (processes[i], speedups[i]),
        textcoords="offset points",
        xytext=(0, 8),
        ha="center"
    )

plt.title("MPI Distributed Search Speedup")
plt.xlabel("Number of MPI Processes")
plt.ylabel("Speedup")
plt.xticks(processes)
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig("results/speedup_results.png", dpi=300)

# Gráfica 3: eficiencia
plt.figure(figsize=(9, 6))
plt.plot(processes, efficiencies, marker="o", linewidth=2)

for i in range(len(processes)):
    plt.annotate(
        f"{efficiencies[i]:.2f}",
        (processes[i], efficiencies[i]),
        textcoords="offset points",
        xytext=(0, 8),
        ha="center"
    )

plt.title("MPI Distributed Search Efficiency")
plt.xlabel("Number of MPI Processes")
plt.ylabel("Efficiency")
plt.xticks(processes)
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.savefig("results/efficiency_results.png", dpi=300)

print("Plots saved in results/")
print("Generated:")
print("results/runtime_results.png")
print("results/speedup_results.png")
print("results/efficiency_results.png")
