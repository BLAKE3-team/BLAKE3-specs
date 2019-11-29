#! /usr/bin/env python3

import json
#from matplotlib import pyplot
from pathlib import Path
#import pandas
#import seaborn
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


def main():
    target = Path(sys.argv[1])
    names = []
    throughputs = []
    sizes = []
    for hash_name, hash_name_pretty in HASH_NAMES:
        names.append(hash_name_pretty)
        hash_dir = target / "bench_group" / hash_name
        for size_i, (size, size_pretty) in enumerate(SIZES):
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
            mbps_throughput = (size / (point * 1e-9)) / 2**20
            if len(sizes) == size_i:
                # sizes.append(size_pretty)
                sizes.append(size)
            if len(throughputs) == size_i:
                throughputs.append([])
            throughputs[size_i].append(mbps_throughput)

        print('% {}'.format(hash_name))
        print('\\addplot coordinates {')
        print('\n'.join([ '({}, {})'.format(x, y[HASH_NAMES.index((hash_name, hash_name_pretty))]) for x, y in zip(sizes, throughputs) ]))
        print('};')
        # break


if __name__ == "__main__":
    main()
