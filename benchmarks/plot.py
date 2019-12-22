#! /usr/bin/env python3

import json
from matplotlib import pyplot
from pathlib import Path
import pandas
import seaborn
import sys

HASH_NAMES = [
    # Note that --features=rayon determines whether this bench uses
    # multithreading.
    ("blake3", "BLAKE3"),

    # Use this instead for the AVX-512 results, with --features=c_detect:
    # ("blake3_c", "BLAKE3"),
    ("blake2s", "BLAKE2s"),
    ("blake2b", "BLAKE2b"),
    ("blake2sp", "BLAKE2sp"),
    ("blake2bp", "BLAKE2bp"),
    ("sha256", "OpenSSL SHA-256"),
    ("sha512", "OpenSSL SHA-512"),
    ("sha3-256", "OpenSSL SHA3-256"),
]

SIZES = [
    (2**6, "64"),
    (2**7, "128"),
    (2**8, "256"),
    (2**9, "512"),
    (2**10, "1 KiB"),
    (2**11, "2 KiB"),
    (2**12, "4 KiB"),
    (2**13, "8 KiB"),
    (2**14, "16 KiB"),
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
]


def dense_sizes():
    sizes = []
    for (size, _) in SIZES[:-1]:
        sizes.append(size)
        sizes.append(size * 5 // 4)
        sizes.append(size * 6 // 4)
        sizes.append(size * 7 // 4)
    sizes.append(SIZES[-1][0])
    return sizes


def main():
    target = Path(sys.argv[1])
    sizes_map = dict(SIZES)
    hash_names = []
    sizes = []
    ticks = []
    tick_names = []
    throughputs = []
    for hash_name, hash_name_pretty in HASH_NAMES:
        hash_names.append(hash_name_pretty)
        hash_dir = target / "bench_group" / hash_name
        for size_i, size in enumerate(dense_sizes()):
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
    dataframe = pandas.DataFrame(throughputs, sizes, hash_names)

    seaborn.set()
    # pyplot.rcParams["axes.labelsize"] = 20
    # pyplot.rcParams["pgf.rcfonts"] = False
    # pyplot.rcParams["font.family"] = "serif"
    # pyplot.figure(figsize=[20, 10])
    pyplot.ylim(0, 1.1 * max(max(col) for col in throughputs))
    seaborn.set_context("paper")
    # seaborn.set_context("talk")
    dash_styles = [
        "", (4, 1.5), (1, 1), (3, 1, 1.5, 1), (5, 1, 1, 1), (5, 1, 2, 1, 2, 1),
        (2, 2, 3, 1.5), (1, 2.5, 3, 1.2)
    ]
    plot = seaborn.lineplot(
        data=dataframe,
        sort=False,
        dashes=dash_styles,
    )
    plot.set(ylabel="Throughput (MB/s)\n", xscale="log")
    pyplot.legend(loc="best", framealpha=1)
    # pyplot.legend(loc="lower right", framealpha=1)
    plot.set(xticks=ticks)
    plot.set_xticklabels(tick_names, rotation=270)
    # pyplot.savefig(target.with_suffix(".pgf"), bbox_inches="tight")
    pyplot.show()


if __name__ == "__main__":
    main()
