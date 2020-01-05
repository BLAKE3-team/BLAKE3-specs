#! /usr/bin/env python3

import json
import math
from matplotlib import pyplot
import os
from pathlib import Path
import pandas
import seaborn
import sys

BENCH_NAMES = [
    ("threads_01", "1 thread"),
    ("threads_02", "2 threads"),
    ("threads_04", "4 threads"),
    ("threads_08", "8 threads"),
    ("threads_16", "16 threads"),
    ("threads_32", "32 threads"),
]

SIZES = [
    (2**20, "1 MiB"),
    (2**21, "2 MiB"),
    (2**22, "4 MiB"),
    (2**23, "8 MiB"),
    (2**24, "16 MiB"),
    (2**25, "32 MiB"),
    (2**26, "64 MiB"),
    (2**27, "128 MiB"),
    (2**28, "256 MiB"),
    (2**29, "512 MiB"),
    (2**30, "1 GiB"),
]


def main():
    target = Path(sys.argv[1])
    sizes_map = dict(SIZES)
    bench_names = []
    sizes = []
    ticks = []
    tick_names = []
    throughputs = []
    for hash_name, hash_name_pretty in BENCH_NAMES:
        bench_names.append(hash_name_pretty)
        hash_dir = target / "bench_group" / hash_name
        for size_i, size in enumerate(size[0] for size in SIZES):
            estimates_path = hash_dir / str(size) / "new/estimates.json"
            try:
                estimates = json.load(estimates_path.open())
            except FileNotFoundError:
                # Some benchmark runs use longer inputs than others, so we
                # ignore missing sizes here.
                continue
            slope = estimates["Slope"]
            point = slope["point_estimate"]
            # upper = slope["confidence_interval"]["upper_bound"]
            # lower = slope["confidence_interval"]["lower_bound"]
            mbps_throughput = size / point * 1000
            if len(throughputs) == size_i:
                throughputs.append([])
                sizes.append(size)
                if size in sizes_map:
                    ticks.append(size)
                    tick_names.append(sizes_map[size])
            throughputs[size_i].append(mbps_throughput)
    print(throughputs, sizes, bench_names)
    dataframe = pandas.DataFrame(throughputs, sizes, bench_names)

    seaborn.set()
    # pyplot.rcParams["axes.labelsize"] = 20
    # pyplot.rcParams["pgf.rcfonts"] = False
    # pyplot.rcParams["font.family"] = "serif"
    # pyplot.figure(figsize=[20, 10])
    seaborn.set_context("paper")
    # seaborn.set_context("talk")
    plot = seaborn.lineplot(
        data=dataframe,
        sort=False,
    )
    plot.set(ylabel="Throughput (MB/s)\n")
    pyplot.ylim(0, 1.1 * max(max(col) for col in throughputs))
    plot.set(xscale="log")
    pyplot.legend(loc="best", framealpha=1)
    # pyplot.legend(loc="lower right", framealpha=1)
    plot.set(xticks=ticks)
    plot.set_xticklabels(tick_names, rotation=270)
    # pyplot.savefig(target.with_suffix(".pgf"), bbox_inches="tight")
    pyplot.show()


if __name__ == "__main__":
    main()
