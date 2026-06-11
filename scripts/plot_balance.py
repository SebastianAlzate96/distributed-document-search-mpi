import csv
import matplotlib.pyplot as plt

INPUT_FILE = "results/balance_results.csv"

rows = []

with open(INPUT_FILE, "r", newline="") as file:
    reader = csv.DictReader(file)

    for row in reader:
        rows.append(row)

labels = [f"{row['Version']}\n{row['Processes']} proc" for row in rows]
times = [float(row["Time"]) for row in rows]
word_imbalance = [float(row["WordImbalance"]) for row in rows]
time_imbalance = [float(row["TimeImbalance"]) for row in rows]

# Runtime comparison
plt.figure(figsize=(10, 6))
plt.bar(labels, times)

for i, value in enumerate(times):
    plt.text(i, value, f"{value:.3f}s", ha="center", va="bottom")

plt.title("TF-IDF Runtime: Static vs Balanced Partition")
plt.xlabel("Version")
plt.ylabel("Time in Seconds")
plt.xticks(rotation=15)
plt.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig("results/tfidf_balance_runtime.png", dpi=300)

# Word imbalance comparison
plt.figure(figsize=(10, 6))
plt.bar(labels, word_imbalance)

for i, value in enumerate(word_imbalance):
    plt.text(i, value, f"{value:.4f}", ha="center", va="bottom")

plt.title("Word Imbalance: Static vs Balanced Partition")
plt.xlabel("Version")
plt.ylabel("Word Imbalance Ratio")
plt.xticks(rotation=15)
plt.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig("results/tfidf_word_imbalance.png", dpi=300)

# Time imbalance comparison
plt.figure(figsize=(10, 6))
plt.bar(labels, time_imbalance)

for i, value in enumerate(time_imbalance):
    plt.text(i, value, f"{value:.4f}", ha="center", va="bottom")

plt.title("Time Imbalance: Static vs Balanced Partition")
plt.xlabel("Version")
plt.ylabel("Time Imbalance Ratio")
plt.xticks(rotation=15)
plt.grid(axis="y", alpha=0.3)
plt.tight_layout()
plt.savefig("results/tfidf_time_imbalance.png", dpi=300)

print("Balance plots saved in results/")
print("Generated:")
print("results/tfidf_balance_runtime.png")
print("results/tfidf_word_imbalance.png")
print("results/tfidf_time_imbalance.png")
