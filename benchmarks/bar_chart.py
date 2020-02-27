#! /usr/bin/env python3

from matplotlib import pyplot
from pathlib import Path
import pandas
import seaborn

THIS_FILE = Path(__file__)

# Benchmark measurements were taken from `cargo bench` run in this directory
# and hardcoded here. The machine used was an AWS c5.metal instance, with an
# Intel Cascade Lake-SP 8275CL processor, running Arch Linux. MD5, SHA-1,
# SHA-2, and SHA-3 are all provided by OpenSSL. BLAKE2b and BLAKE2s are
# provided by the `blake2b_simd` and `blake2s_simd` crates. BLAKE3 is provided
# by the `blake3` crate.
#
# The benchmark suite includes a large set of different input lengths (the
# entire data set for Figure 3 in the BLAKE3 paper), which takes a long time to
# run. To run just the 16 KiB benchmarks shown in this chart, set the env var
# `BLAKE3_BENCH_16KIB=1`.
#
# Note that while the benchmark machine supports AVX-512, the `blake2b_simd`
# and `blake2s_simd` crates do not currently include explicit AVX-512
# rotations. To get the compiler to include those rotations, I set the env var
# `RUSTFLAGS="-C target-cpu=native"`, for the BLAKE2 benchmarks only.
#
# Although the benchmark suite includes other parallel algorithms (BLAKE2bp,
# BLAKE2sp, KangarooTwelve), their figures are omitted here. That's both
# because they are uncommon -- for example, not supported in OpenSSL -- and
# also because the 16 KiB input length used here would be unfair to them.

BARS = [
    ("BLAKE3", 6866),
    ("BLAKE2b", 1312),
    ("SHA-1", 1027),
    ("BLAKE2s", 876),
    ("MD5", 740),
    ("SHA-512", 720),
    ("SHA-256", 484),
    ("SHA3-256", 394),
]


# adapted from https://stackoverflow.com/a/56780852/823869
def show_values_on_bars(axes):
    left_padding = 100
    bottom_padding = 0.25
    for p in axes.patches:
        _x = p.get_x() + p.get_width() + float(left_padding)
        _y = p.get_y() + p.get_height() - float(bottom_padding)
        value = int(p.get_width())
        axes.text(_x, _y, value, ha="left")


def main():
    names = [x[0] for x in BARS]
    values = [x[1] for x in BARS]
    dataframe = pandas.DataFrame({"names": names, "values": values})
    seaborn.set()
    seaborn.set_style("ticks")
    # pyplot.rcParams["axes.labelsize"] = 20
    # pyplot.rcParams["pgf.rcfonts"] = False
    # pyplot.rcParams["font.family"] = "serif"
    # pyplot.figure(figsize=[20, 10])
    # seaborn.set_context("paper")
    # seaborn.set_context("talk")
    plot = seaborn.barplot(
        data=dataframe,
        y="names",
        x="values",
        color="red",
        edgecolor="black",
        # linewidth=3,
    )
    show_values_on_bars(plot.axes)
    plot.set_title("Performance on AWS c5.metal, 16 KiB input, 1 thread")
    plot.set(xlabel="Speed (MiB/s)")
    plot.set(ylabel=None)
    pyplot.xlim(0, 8000)
    # pyplot.savefig(THIS_FILE.with_suffix(".svg"), bbox_inches="tight")
    # pyplot.savefig(THIS_FILE.with_suffix(".png"), bbox_inches="tight")
    pyplot.show()


if __name__ == "__main__":
    main()
