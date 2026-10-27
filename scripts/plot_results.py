
import json
import matplotlib.pyplot as plt

def main():
    with open("results/raw_results.json", "r") as f:
        data = json.load(f)

    benchmarks_data = {}

    for bm in data.get("benchmarks", []):
        name = bm.get("name", "")

        if bm.get("run_type") == "aggregate":
            continue
        if "BigO" in name or "RMS" in name:
            continue
        if any(x in name for x in ["_mean", "_median", "_stddev", "_cv"]):
            continue

        parts = name.split("/")
        if len(parts) < 2 or not parts[1].isdigit():
            continue

        kernel = parts[0]
        N = int(parts[1])

        if "manual_time" in name:
            time = bm["real_time"]
        else:
            time = bm["cpu_time"]

        unit = bm.get("time_unit", "ns")
        if unit == "ns":
            time /= 1000
        elif unit == "ms":
            time *= 1000
        elif unit == "s":
            time *= 1000000

        if kernel not in benchmarks_data:
            benchmarks_data[kernel] = {}

        if N not in benchmarks_data[kernel]:
            benchmarks_data[kernel][N] = []

        benchmarks_data[kernel][N].append(time)

    styles = {
        "BM_search": ("Search - O(logN)", "#1f77b4", "o", "-"),
        "BM_insert": ("Insert - O(logN)", "#2ca02c", "s", "--"),
        "BM_split_structural": ("Split Structural - O(logN)", "#d62728", "^", "-"),
        "BM_split_rebuilding": ("Split Rebuilding - O(N*logN)", "#9467bd", "x", ":"),
        "BM_union": ("Union - O(log N)", "#ff7f0e", "d", "-")
    }

    plt.figure(figsize=(10, 6), dpi=300)

    for kernel, values in benchmarks_data.items():
        sizes = sorted(values)
        times = [sum(values[N]) / len(values[N]) for N in sizes]
        label, color, marker, linestyle = styles[kernel]

        plt.plot(sizes, times, label=label, color=color,
                 marker=marker, linestyle=linestyle, linewidth=2)

    plt.xscale("log", base=2)
    plt.yscale("log")
    plt.xlabel("Input Size N (log2 scale)")
    plt.ylabel("Time per operation (microseconds, log scale)")
    plt.title("Skip List Performance Scaling & Structural vs Rebuilding")
    plt.grid(True, which="both", linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()

    plt.savefig("results/scaling_plot.png")
    print("Graph saved in results/scaling_plot.png")

if __name__ == "__main__":
    main()
