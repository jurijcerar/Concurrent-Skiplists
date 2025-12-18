import os
import pandas as pd
import matplotlib.pyplot as plt

def set_scientific_yaxis():
    """Helper function to format y-axis in scientific notation."""
    plt.ticklabel_format(axis="y", style="scientific", scilimits=(0, 0))

def plot_skiplist(data_dir, output_dir):
    files = sorted([f for f in os.listdir(data_dir + "/skiplist") if "results" in f])

    mixes = []
    throughputs = []

    for file in files:
        try:
            operation_mix = file.split("--mix-")[1].split(".csv")[0]
            df = pd.read_csv(f"{data_dir}/skiplist/{file}")

            if "Throughput" in df.columns:
                mixes.append(operation_mix)
                throughputs.append(df["Throughput"].iloc[0])
            else:
                print(f"Warning: 'Throughput' column not found in {file}")

        except Exception as e:
            print(f"Error processing file {file}: {e}")
            continue

    if not mixes or not throughputs:
        print("No valid data to plot.")
        return

    plt.figure()
    plt.bar(mixes, throughputs, color='blue')
    plt.xlabel("Operation Mix")
    plt.ylabel("Throughput")
    plt.title("Throughput for Different Operation Mixes (Skiplist)")
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(f"{output_dir}/skiplist_comparison.png")
    plt.close()

def plot_skiplist_bar(data_dir, output_dir):
    operation_types = ["Insert_Success", "Delete_Success", "Contains_Success"]
    data = {}

    # Locate the relevant file for skiplist results
    files = [f for f in os.listdir(data_dir + "/skiplist") if "results" in f]

    if not files:
        print("No files found for skiplist success percentages.")
        return

    for file in files:
        try:
            df = pd.read_csv(f"{data_dir}/skiplist/{file}")
            operation_mix = file.split("--mix-")[1].split(".csv")[0]

            if not all(col in df.columns for col in operation_types):
                print(f"Missing required columns in {file}")
                continue

            # Store the success percentages for this operation mix
            data[operation_mix] = {op: df[op].iloc[0] for op in operation_types}

        except Exception as e:
            print(f"Error processing file {file}: {e}")

    if not data:
        print("No valid data to plot for skiplist success percentages.")
        return

    # Plot the bar chart
    x_labels = operation_types
    mixes = list(data.keys())
    bar_width = 0.15

    plt.figure(figsize=(10, 6))
    for i, mix in enumerate(mixes):
        values = [data[mix].get(op, 0) for op in operation_types]
        plt.bar(
            [p + (i * bar_width) for p in range(len(x_labels))],
            values,
            width=bar_width,
            label=f"Mix {mix}",
        )

    plt.xticks(
        [p + (bar_width * (len(mixes) / 2)) for p in range(len(x_labels))], x_labels
    )
    plt.ylabel("Success Percentage")
    plt.title("Operation Success Percentage (Skiplist)")
    plt.legend(title="Operation Mix")
    plt.grid(axis="y", linestyle="--", alpha=0.7)
    plt.tight_layout()
    plt.savefig(f"{output_dir}/skiplist_operation_success.png")
    plt.close()

def plot_skiplist3(data_dir, output_dir):
    files = sorted([f for f in os.listdir(data_dir + "/skiplist3") if "results" in f])
    plt.figure()

    thread_data = []

    for file in files:
        try:
            thread_count = int(file.split("-")[2])
            df = pd.read_csv(f"{data_dir}/skiplist3/{file}")
            thread_data.append((thread_count, df["Threads"], df["Throughput"]))
        except Exception as e:
            print(f"Error processing file {file}: {e}")
            continue

    thread_data.sort(key=lambda x: x[0])

    all_threads = []
    all_throughputs = []

    for _, threads, throughput in thread_data:
        all_threads.extend(threads)
        all_throughputs.extend(throughput)

    plt.plot(all_threads, all_throughputs, marker="o", color="blue")
    plt.xlabel("Threads")
    plt.ylabel("Throughput")
    plt.title("Throughput vs Threads for Skiplist3")
    set_scientific_yaxis()
    plt.grid()
    plt.savefig(f"{output_dir}/skiplist3_throughput.png")
    plt.close()

def plot_skiplist4(data_dir, output_dir):
    for range_type in ["COMMON", "DISJOINT"]:
        files = sorted([f for f in os.listdir(data_dir + "/skiplist4") if range_type in f])
        plt.figure()

        thread_data = []

        for file in files:
            try:
                thread_count = int(file.split("-")[2])
                df = pd.read_csv(f"{data_dir}/skiplist4/{file}")
                thread_data.append((thread_count, df["Threads"], df["Throughput"]))
            except Exception as e:
                print(f"Error processing file {file}: {e}")
                continue

        thread_data.sort(key=lambda x: x[0])

        all_threads = []
        all_throughputs = []

        for _, threads, throughput in thread_data:
            all_threads.extend(threads)
            all_throughputs.extend(throughput)

        plt.plot(all_threads, all_throughputs, marker="o", color="blue")
        plt.xlabel("Threads")
        plt.ylabel("Throughput")
        plt.title(f"Skiplist4 Throughput ({range_type})")
        set_scientific_yaxis()
        plt.grid()
        plt.savefig(f"{output_dir}/skiplist4_{range_type}.png")
        plt.close()

def plot_skiplist4_bar(data_dir, output_dir):
    operation_types = ["Insert_Success", "Delete_Success", "Contains_Success"]
    data_common = {}
    data_disjoint = {}

    for range_type in ["COMMON", "DISJOINT"]:
        files = [f for f in os.listdir(data_dir + "/skiplist4") if f"-threads-64-" in f and range_type in f]

        if not files:
            print(f"No file found for {range_type} with 64 threads.")
            continue

        file = files[0]
        df = pd.read_csv(f"{data_dir}/skiplist4/{file}")

        if not all(col in df.columns for col in operation_types):
            print(f"Missing required columns in {file}")
            continue

        if range_type == "COMMON":
            data_common = {op: df[op].iloc[0] for op in operation_types}
        elif range_type == "DISJOINT":
            data_disjoint = {op: df[op].iloc[0] for op in operation_types}

    if not data_common or not data_disjoint:
        print("No valid data to plot for COMMON or DISJOINT.")
        return

    x_labels = ["Insert", "Delete", "Contains"]
    common_values = [data_common.get(op, 0) for op in operation_types]
    disjoint_values = [data_disjoint.get(op, 0) for op in operation_types]

    x = range(len(x_labels))

    plt.figure(figsize=(8, 6))
    bar_width = 0.35
    plt.bar(x, common_values, width=bar_width, label="COMMON", color="blue")
    plt.bar([p + bar_width for p in x], disjoint_values, width=bar_width, label="DISJOINT", color="orange")

    plt.xticks([p + bar_width / 2 for p in x], x_labels)
    plt.ylabel("Success Percentage")
    plt.title("Operation Success Percentage (64 Threads)")
    plt.legend()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(f"{output_dir}/skiplist4_operation_success.png")
    plt.close()

def plot_skiplist5(data_dir, output_dir):
    for range_type in ["COMMON", "DISJOINT"]:
        files = sorted([f for f in os.listdir(data_dir + "/skiplist5") if range_type in f])
        plt.figure()

        thread_data = []

        for file in files:
            try:
                thread_count = int(file.split("-")[2])
                df = pd.read_csv(f"{data_dir}/skiplist5/{file}")
                thread_data.append((thread_count, df["Threads"], df["Throughput"]))
            except Exception as e:
                print(f"Error processing file {file}: {e}")
                continue

        thread_data.sort(key=lambda x: x[0])

        all_threads = []
        all_throughputs = []

        for _, threads, throughput in thread_data:
            all_threads.extend(threads)
            all_throughputs.extend(throughput)

        plt.plot(all_threads, all_throughputs, marker="o", color="green")
        plt.xlabel("Threads")
        plt.ylabel("Throughput")
        plt.title(f"Skiplist5 Throughput ({range_type})")
        set_scientific_yaxis()
        plt.grid()
        plt.savefig(f"{output_dir}/skiplist5_{range_type}.png")
        plt.close()

def plot_skiplist5_bar(data_dir, output_dir):
    operation_types = ["Insert_Success", "Delete_Success", "Contains_Success"]
    data_common = {}
    data_disjoint = {}

    for range_type in ["COMMON", "DISJOINT"]:
        files = [f for f in os.listdir(data_dir + "/skiplist5") if f"-threads-64-" in f and range_type in f]

        if not files:
            print(f"No file found for {range_type} with 64 threads.")
            continue

        file = files[0]
        df = pd.read_csv(f"{data_dir}/skiplist5/{file}")

        if not all(col in df.columns for col in operation_types):
            print(f"Missing required columns in {file}")
            continue

        if range_type == "COMMON":
            data_common = {op: df[op].iloc[0] for op in operation_types}
        elif range_type == "DISJOINT":
            data_disjoint = {op: df[op].iloc[0] for op in operation_types}

    if not data_common or not data_disjoint:
        print("No valid data to plot for COMMON or DISJOINT.")
        return

    x_labels = ["Insert", "Delete", "Contains"]
    common_values = [data_common.get(op, 0) for op in operation_types]
    disjoint_values = [data_disjoint.get(op, 0) for op in operation_types]

    x = range(len(x_labels))

    plt.figure(figsize=(8, 6))
    bar_width = 0.35
    plt.bar(x, common_values, width=bar_width, label="COMMON", color="green")
    plt.bar([p + bar_width for p in x], disjoint_values, width=bar_width, label="DISJOINT", color="purple")

    plt.xticks([p + bar_width / 2 for p in x], x_labels)
    plt.ylabel("Success Percentage")
    plt.title("Operation Success Percentage (64 Threads)")
    plt.legend()
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(f"{output_dir}/skiplist5_operation_success.png")
    plt.close()

def main():
    data_dir = "data"
    output_dir = "plots"
    os.makedirs(output_dir, exist_ok=True)

    plot_skiplist(data_dir, output_dir)
    plot_skiplist_bar(data_dir, output_dir)
    plot_skiplist3(data_dir, output_dir)
    plot_skiplist4(data_dir, output_dir)
    plot_skiplist4_bar(data_dir, output_dir)
    plot_skiplist5(data_dir, output_dir)
    plot_skiplist5_bar(data_dir, output_dir)

if __name__ == "__main__":
    main()

