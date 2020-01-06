#! /usr/bin/env python3

import json
from pathlib import Path
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
    throughputs = []
    sizes = []
    for hash_name, hash_name_pretty in BENCH_NAMES:
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
            seconds = point / 1e9
            bps_throughput = size / seconds
            gibps_throughput = bps_throughput / (2 ** 30)
            if len(sizes) == size_i:
                # sizes.append(size_pretty)
                sizes.append(size)
            if len(throughputs) == size_i:
                throughputs.append([])
            throughputs[size_i].append(gibps_throughput)

        print('% {}'.format(hash_name))
        print('\\addplot coordinates {')
        print('\n'.join([ '({}, {})'.format(x, y[BENCH_NAMES.index((hash_name, hash_name_pretty))]) for x, y in zip(sizes, throughputs) ]))
        print('};')
        # break


if __name__ == "__main__":
    main()
