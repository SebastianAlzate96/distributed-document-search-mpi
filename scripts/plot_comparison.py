import csv
import matplotlib.pyplot as plt

INPUT_FILE = "results/timing_results_full.csv"

data = {
    "MPI Simple": {
        "processes": [],
        "times": [],
        "speedups": [],
        "efficiencies": []
    },
    "MPI TF-IDF": {
        "processes": [],
        "times": [],
        "speedups": [],
        "efficiencies": []
    }
}

with open(INPUT_FILE, "r", newline="") as file:
    reader = csv.DictReader(file)

    for row in reader:
        model = row["Model"]

        data[model]["processes"].append(int(row["Processes"]))
        data[model]["times"].append(float(row["Time"]))
        data[model]["speedups"].append(float(row["Speedup"]))
        data[model]["efficiencies"].append(float(row["Efficiency"]))

# Runtime comparison
plt.figure(figsize=(9, 6))

for model in data:
    plt.plot(
        data[model]["processes"],
        data[model]["times"],
        marker="o",
        linewidth=2,
        label=model
    )

plt.title("Runtime Comparison: MPI Simple vs MPI TF-IDF")
plt.xlabel("Number of MPI Processes")
plt.ylabel("Time in Seconds")
plt.xticks([1, 2, 4, 8])
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig("results/runtime_comparison.png", dpi=300)

# Speedup comparison
plt.figure(figsize=(9, 6))

for model in data:
    plt.plot(
        data[model]["processes"],
        data[model]["speedups"],
        marker="o",
        linewidth=2,
        label=model
    )

plt.title("Speedup Comparison: MPI Simple vs MPI TF-IDF")
plt.xlabel("Number of MPI Processes")
plt.ylabel("Speedup")
plt.xticks([1, 2, 4, 8])
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig("results/speedup_comparison.png", dpi=300)

# Efficiency comparison
plt.figure(figsize=(9, 6))

for model in data:
    plt.plot(
        data[model]["processes"],
        data[model]["efficiencies"],
        marker="o",
        linewidth=2,
        label=model
    )

plt.title("Efficiency Comparison: MPI Simple vs MPI TF-IDF")
plt.xlabel("Number of MPI Processes")
plt.ylabel("Efficiency")
plt.xticks([1, 2, 4, 8])
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig("results/efficiency_comparison.png", dpi=300)

print("Comparison plots saved in results/")
print("Generated:")
print("results/runtime_comparison.png")
print("results/speedup_comparison.png")
print("results/efficiency_comparison.png")
