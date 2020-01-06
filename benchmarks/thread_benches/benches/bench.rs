use criterion::{criterion_group, criterion_main, Criterion, ParameterizedBenchmark, Throughput};
use rand::prelude::*;

const MIN_LEN: usize = 1 << 15;
const MAX_LEN: usize = 1 << 30;

criterion_group!(cg, benchmarks);
criterion_main!(cg);

fn benchmarks(c: &mut Criterion) {
    let mut params = Vec::new();
    let mut len = MIN_LEN;
    while len < MAX_LEN {
        params.push(len);
        len *= 2;
    }
    params.push(MAX_LEN);

    let b = ParameterizedBenchmark::new(
        "threads_01",
        |b, param| {
            let mut input = RandomInput::new(*param);
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(1)
                .build()
                .unwrap();
            pool.install(|| {
                b.iter(|| blake3::hash(input.get()));
            });
        },
        params,
    );
    let b = b
        .with_function("threads_02", |b, param| {
            let mut input = RandomInput::new(*param);
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(2)
                .build()
                .unwrap();
            pool.install(|| {
                b.iter(|| blake3::hash(input.get()));
            });
        })
        .with_function("threads_04", |b, param| {
            let mut input = RandomInput::new(*param);
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(4)
                .build()
                .unwrap();
            pool.install(|| {
                b.iter(|| blake3::hash(input.get()));
            });
        })
        .with_function("threads_08", |b, param| {
            let mut input = RandomInput::new(*param);
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(8)
                .build()
                .unwrap();
            pool.install(|| {
                b.iter(|| blake3::hash(input.get()));
            });
        })
        .with_function("threads_16", |b, param| {
            let mut input = RandomInput::new(*param);
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(16)
                .build()
                .unwrap();
            pool.install(|| {
                b.iter(|| blake3::hash(input.get()));
            });
        })
        .with_function("threads_32", |b, param| {
            let mut input = RandomInput::new(*param);
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(32)
                .build()
                .unwrap();
            pool.install(|| {
                b.iter(|| blake3::hash(input.get()));
            });
        })
        // 48 is the number of physical cores on the AWS c5.metal instance
        // which I current use to run this benchmark.
        .with_function("threads_48", |b, param| {
            let mut input = RandomInput::new(*param);
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(48)
                .build()
                .unwrap();
            pool.install(|| {
                b.iter(|| blake3::hash(input.get()));
            });
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
