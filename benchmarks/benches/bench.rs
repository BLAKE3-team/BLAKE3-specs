use criterion::{criterion_group, criterion_main, Criterion, ParameterizedBenchmark, Throughput};
use rand::prelude::*;

const MIN_LEN: usize = 64;
const MAX_LEN: usize = 1 << 20;

criterion_group!(cg, benchmarks);
criterion_main!(cg);

fn benchmarks(c: &mut Criterion) {
    let mut params = Vec::new();
    let mut len = MIN_LEN;
    while len < MAX_LEN {
        // Add points in the middle, so we measure the sawtooth curve.
        params.push(len);
        params.push(len * 5 / 4);
        params.push(len * 6 / 4);
        params.push(len * 7 / 4);
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
