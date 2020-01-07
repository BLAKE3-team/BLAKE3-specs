#! /usr/bin/env python3

from matplotlib import pyplot
from pathlib import Path
import numpy
import pandas
import seaborn

THIS_FILE = Path(__file__)

# Measurements were taken with a 16 KiB input on the c5.metal machine, with all
# cores disabled but one, and TurboBoost left on (3.9 GHz). Best of 3 runs.
# BLAKE2b and BLAKE2s (but not BLAKE3) were built with RUSTFLAGS="-C
# target-cpu=native", to get AVX-512 rotations. Other parallel algorithms
# (BLAKE2bp, BLAKE2sp, KangarooTwelve) are omitted here, both because they are
# uncommon, and because the 16 KiB length is cherry-picked for BLAKE3 and would
# be unfair to them.

BARS = [
    ("BLAKE3", 6164),
    ("BLAKE2b", 1312),
    ("SHA1", 1027),
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
    plot.set_title("Hash function speed for a 16 KiB input")
    plot.set(xlabel="Speed (MiB/s)")
    plot.set(ylabel=None)
    pyplot.xlim(0, 7000)
    # pyplot.savefig(THIS_FILE.with_suffix(".svg"), bbox_inches="tight")
    pyplot.show()


if __name__ == "__main__":
    main()
