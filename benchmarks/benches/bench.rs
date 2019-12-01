use criterion::{criterion_group, criterion_main, Criterion, ParameterizedBenchmark, Throughput, measurement::{Measurement, ValueFormatter}};
use rand::prelude::*;

const MIN_LEN: usize = 64;
const MAX_LEN: usize = 1 << 20;

/* copy-pasted from https://docs.rs/crate/criterion-cycles-per-byte/0.1.1/source/src/lib.rs */

/// `CyclesPerByte` measures clock cycles using the x86 or x86_64 `rdtsc` instruction. `cpb` is
/// the preferrerd measurement for cryptographic algorithms.
pub struct CyclesPerByte;

// WARN: does not check for the cpu feature; but we'd panic anyway so...
fn rdtsc() -> u64 {
    #[cfg(target_arch = "x86_64")]
    unsafe {
        // core::arch::x86_64::_rdtsc()
        let mut discard: u32 = 0;
        core::arch::x86_64::__rdtscp(&mut discard)
    }

    #[cfg(target_arch = "x86")]
    unsafe {
        let mut discard: u32 = 0;
        core::arch::x86::__rdtscp(&mut discard)
        // core::arch::x86::_rdtsc()
    }
}

impl Measurement for CyclesPerByte {
    type Intermediate = u64;
    type Value = u64;

    fn start(&self) -> Self::Intermediate {
        rdtsc()
    }

    fn end(&self, i: Self::Intermediate) -> Self::Value {
        rdtsc() - i
    }

    fn add(&self, v1: &Self::Value, v2: &Self::Value) -> Self::Value {
        v1 + v2
    }

    fn zero(&self) -> Self::Value {
        0
    }

    fn to_f64(&self, value: &Self::Value) -> f64 {
        *value as f64
    }

    fn formatter(&self) -> &dyn ValueFormatter {
        &CyclesPerByteFormatter
    }
}

struct CyclesPerByteFormatter;

impl ValueFormatter for CyclesPerByteFormatter {
    fn format_value(&self, value: f64) -> String {
        format!("{:.4} cycles", value)
    }

    fn format_throughput(&self, throughput: &Throughput, value: f64) -> String {
        match throughput {
            Throughput::Bytes(b) => format!("{:.4} cpb", value / *b as f64),
            Throughput::Elements(b) => format!("{:.4} cycles/{}", value, b),
        }
    }

    fn scale_values(&self, _typical_value: f64, _values: &mut [f64]) -> &'static str {
        "cycles"
    }

    fn scale_throughputs(
        &self,
        _typical_value: f64,
        throughput: &Throughput,
        values: &mut [f64],
    ) -> &'static str {
        match throughput {
            Throughput::Bytes(n) => {
                for val in values {
                    *val /= *n as f64;
                }
                "cpb"
            }
            Throughput::Elements(n) => {
                for val in values {
                    *val /= *n as f64;
                }
                "c/e"
            }
        }
    }

    fn scale_for_machines(&self, _values: &mut [f64]) -> &'static str {
        "cycles"
    }
}

/* /copy-pasted from https://docs.rs/crate/criterion-cycles-per-byte/0.1.1/source/src/lib.rs */

criterion_group!(
    name = cg; 
    config = Criterion::default().with_measurement(CyclesPerByte); 
    targets = benchmarks
);
criterion_main!(cg);

fn benchmarks(c: &mut Criterion<CyclesPerByte>) {
    let mut params = Vec::new();
    let mut len = MIN_LEN;
    while len <= MAX_LEN {
        params.push(len);
        len *= 2;
    }

    let b = ParameterizedBenchmark::new(
        "blake3",
        |b, param| {
            let mut input = RandomInput::new(*param);
            b.iter(|| blake3::hash(input.get()));
        },
        params,
    );
    // This C implementation is generally slower than the Rust one for short
    // inputs, because it does more copying and less inlining. But Rust doesn't
    // yet support AVX-512 or NEON.
    #[cfg(feature = "c_detect")]
    let b = b.with_function("blake3_c", |b, param| {
        let mut input = RandomInput::new(*param);
        b.iter(|| {
            let mut hasher = blake3::c::Hasher::new(&[0; 32], 0);
            hasher.update(input.get());
            hasher.finalize()
        })
    });
    let b = b
        .with_function("blake2b", |b, param| {
            let mut input = RandomInput::new(*param);
            b.iter(|| blake2b_simd::blake2b(input.get()));
        })
        .with_function("blake2s", |b, param| {
            let mut input = RandomInput::new(*param);
            b.iter(|| blake2s_simd::blake2s(input.get()));
        })
        .with_function("blake2bp", |b, param| {
            let mut input = RandomInput::new(*param);
            b.iter(|| blake2b_simd::blake2bp::blake2bp(input.get()));
        })
        .with_function("blake2sp", |b, param| {
            let mut input = RandomInput::new(*param);
            b.iter(|| blake2s_simd::blake2sp::blake2sp(input.get()));
        })
        .with_function("sha256", |b, param| {
            let mut input = RandomInput::new(*param);
            b.iter(|| openssl::hash::hash(openssl::hash::MessageDigest::sha256(), input.get()));
        })
        .with_function("sha512", |b, param| {
            let mut input = RandomInput::new(*param);
            b.iter(|| openssl::hash::hash(openssl::hash::MessageDigest::sha512(), input.get()));
        })
        .with_function("sha3-256", |b, param| {
            let mut input = RandomInput::new(*param);
            b.iter(|| openssl::hash::hash(openssl::hash::MessageDigest::sha3_256(), input.get()));
        })
        .throughput(|len| Throughput::Bytes(*len as u64));
    c.bench("bench_group", b);
}

// This struct randomizes two things:
// 1. The actual bytes of input.
// 2. The page offset the input starts at.
pub struct RandomInput {
    buf: Vec<u8>,
    len: usize,
    offsets: Vec<usize>,
    offset_index: usize,
}

impl RandomInput {
    pub fn new(len: usize) -> Self {
        let page_size: usize = page_size::get();
        let mut buf = vec![0u8; len + page_size];
        let mut rng = rand::thread_rng();
        rng.fill_bytes(&mut buf);
        let mut offsets: Vec<usize> = (0..page_size).collect();
        offsets.shuffle(&mut rng);
        Self {
            buf,
            len,
            offsets,
            offset_index: 0,
        }
    }

    pub fn get(&mut self) -> &[u8] {
        let offset = self.offsets[self.offset_index];
        self.offset_index += 1;
        if self.offset_index >= self.offsets.len() {
            self.offset_index = 0;
        }
        &self.buf[offset..][..self.len]
    }
}
