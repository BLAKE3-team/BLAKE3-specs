#! /usr/bin/env python3

import json
from matplotlib import pyplot
from pathlib import Path
import pandas
import seaborn
import sys

BENCH_NAMES = [
    ("threads_48", "48 threads"),
    ("threads_32", "32 threads"),
    ("threads_16", "16 threads"),
    ("threads_08", "8 threads"),
    ("threads_04", "4 threads"),
    ("threads_02", "2 threads"),
    ("threads_01", "1 thread"),
]

SIZES = [
    (2**15, "32 KiB"),
    (2**16, "64 KiB"),
    (2**17, "128 KiB"),
    (2**18, "256 KiB"),
    (2**19, "512 KiB"),
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
    for bench_name, bench_name_pretty in BENCH_NAMES:
        bench_names.append(bench_name_pretty)
        hash_dir = target / "bench_group" / bench_name
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
            seconds = point / 1e9
            bps_throughput = size / seconds
            gibps_throughput = bps_throughput / (2 ** 30)
            if len(throughputs) == size_i:
                throughputs.append([])
                sizes.append(size)
                if size in sizes_map:
                    ticks.append(size)
                    tick_names.append(sizes_map[size])
            throughputs[size_i].append(gibps_throughput)
    dataframe = pandas.DataFrame(throughputs, sizes, bench_names)

    seaborn.set()
    # pyplot.rcParams["axes.labelsize"] = 20
    # pyplot.rcParams["pgf.rcfonts"] = False
    # pyplot.rcParams["font.family"] = "serif"
    # pyplot.figure(figsize=[20, 10])
    seaborn.set_context("paper")
    # seaborn.set_context("talk")
    dash_styles = [
        "", (4, 1.5), (1, 1), (3, 1, 1.5, 1), (5, 1, 1, 1), (5, 1, 2, 1, 2, 1),
        (2, 2, 3, 1.5), (1, 2.5, 3, 1.2), (2, 2)
    ]
    plot = seaborn.lineplot(
        data=dataframe,
        sort=False,
        dashes=dash_styles,
    )
    plot.set(ylabel="Throughput (GB/s)\n")
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
