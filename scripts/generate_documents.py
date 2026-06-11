import csv
import random
import sys

OUTPUT_FILE = "data/documents.csv"

TOPICS = {
    "parallel_computing": [
        "parallel", "threads", "processes", "speedup", "efficiency",
        "openmp", "mpi", "communication", "synchronization"
    ],
    "distributed_systems": [
        "distributed", "nodes", "cluster", "network", "message",
        "fault", "replication", "load", "balance"
    ],
    "machine_learning": [
        "model", "training", "features", "classification", "clustering",
        "neural", "prediction", "dataset", "learning"
    ],
    "databases": [
        "query", "index", "transaction", "table", "database",
        "join", "storage", "record", "search"
    ],
    "cybersecurity": [
        "security", "attack", "encryption", "malware", "authentication",
        "firewall", "threat", "risk", "privacy"
    ],
    "operating_systems": [
        "kernel", "memory", "scheduler", "process", "thread",
        "file", "system", "cpu", "resource"
    ],
    "data_mining": [
        "pattern", "mining", "data", "association", "knowledge",
        "analysis", "large", "dataset", "similarity"
    ],
    "sports_analytics": [
        "team", "goals", "points", "season", "performance",
        "players", "match", "statistics", "ranking"
    ]
}

FILLER_WORDS = [
    "the", "system", "method", "result", "value", "input", "output",
    "performance", "analysis", "problem", "solution", "experiment",
    "implementation", "approach", "data", "time", "memory", "algorithm"
]

def generate_document(topic, doc_id):
    keywords = TOPICS[topic]

    # La mayoría de documentos son medianos.
    word_count = random.randint(80, 250)

    # Algunos documentos son más largos para simular desbalance de carga.
    if random.random() < 0.08:
        word_count = random.randint(600, 1200)

    words = random.choices(FILLER_WORDS, k=word_count)

    # Insertamos palabras importantes del tema varias veces.
    repetitions = random.randint(8, 25)
    for _ in range(repetitions):
        position = random.randint(0, len(words) - 1)
        words[position] = random.choice(keywords)

    title = f"{topic.replace('_', ' ').title()} Document {doc_id}"
    text = " ".join(words)

    return title, text, word_count

def main():
    if len(sys.argv) >= 2:
        n = int(sys.argv[1])
    else:
        n = 100000

    random.seed(42)

    with open(OUTPUT_FILE, "w", newline="", encoding="utf-8") as file:
        fieldnames = ["DocID", "Title", "Topic", "WordCount", "Text"]
        writer = csv.DictWriter(file, fieldnames=fieldnames)
        writer.writeheader()

        topics = list(TOPICS.keys())

        for doc_id in range(n):
            topic = random.choice(topics)
            title, text, word_count = generate_document(topic, doc_id)

            writer.writerow({
                "DocID": doc_id,
                "Title": title,
                "Topic": topic,
                "WordCount": word_count,
                "Text": text
            })

    print(f"Generated {n} documents in {OUTPUT_FILE}")

if __name__ == "__main__":
    main()
