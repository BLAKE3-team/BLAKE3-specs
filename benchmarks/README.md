Run `cargo bench` to execute these benchmarks yourself.

To cross-compile benchmarks for ARM Cortex-M0, follow these steps:

```
git clone https://github.com/raspberrypi/tools /tmp/rpi_tools
export CC=/tmp/rpi_tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
export RUSTFLAGS="-C linker=$CC"
cargo build --target arm-unknown-linux-gnueabihf --release --benches
```

The executable is in `target/arm-unknown-linux-gnueabihf/release` with a
name like `bench-4c1193f4bc51158b` (the numbers vary). Run that binary
with the `--bench` flag. Benchmark results will be written to
`./target/criterion/bench_group`.

Note that the `blake2b_simd` crate used here sets the
`uninline_portable` flag, which improves BLAKE2b performance on ARM
Cortex-M0. If you run benchmarks on larger ARM processors or other
non-x86, you might want to disable that feature.
