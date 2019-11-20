const OUT_LEN: usize = 32;
const KEY_LEN: usize = 32;
const BLOCK_LEN: usize = 64;
const CHUNK_LEN: usize = 2048;
const ROUNDS: usize = 7;

const CHUNK_START: u32 = 1 << 0;
const CHUNK_END: u32 = 1 << 1;
const PARENT: u32 = 1 << 2;
const ROOT: u32 = 1 << 3;
const KEYED_HASH: u32 = 1 << 4;
const DERIVE_KEY: u32 = 1 << 5;

const IV: [u32; 8] = [
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C,
    0x1F83D9AB, 0x5BE0CD19,
];

const MSG_SCHEDULE: [[usize; 16]; ROUNDS] = [
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
    [14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3],
    [11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4],
    [7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8],
    [9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13],
    [2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9],
    [12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11],
];

fn words_from_litte_endian_bytes(bytes: &[u8], words: &mut [u32]) {
    for (b, w) in bytes.chunks_exact(4).zip(words.iter_mut()) {
        *w = u32::from_le_bytes(core::convert::TryInto::try_into(b).unwrap());
    }
}

fn little_endian_bytes_from_words(words: &[u32], bytes: &mut [u8]) {
    for (w, b) in words.iter().zip(bytes.chunks_exact_mut(4)) {
        b.copy_from_slice(&w.to_le_bytes());
    }
}

// The mixing function, G, which mixes either a column or a diagonal.
fn g(
    state: &mut [u32; 16],
    a: usize,
    b: usize,
    c: usize,
    d: usize,
    mx: u32,
    my: u32,
) {
    state[a] = state[a].wrapping_add(state[b]).wrapping_add(mx);
    state[d] = (state[d] ^ state[a]).rotate_right(16);
    state[c] = state[c].wrapping_add(state[d]);
    state[b] = (state[b] ^ state[c]).rotate_right(12);
    state[a] = state[a].wrapping_add(state[b]).wrapping_add(my);
    state[d] = (state[d] ^ state[a]).rotate_right(8);
    state[c] = state[c].wrapping_add(state[d]);
    state[b] = (state[b] ^ state[c]).rotate_right(7);
}

fn round(state: &mut [u32; 16], m: &[u32; 16], schedule: &[usize; 16]) {
    // Mix the columns.
    g(state, 0, 4, 8, 12, m[schedule[0]], m[schedule[1]]);
    g(state, 1, 5, 9, 13, m[schedule[2]], m[schedule[3]]);
    g(state, 2, 6, 10, 14, m[schedule[4]], m[schedule[5]]);
    g(state, 3, 7, 11, 15, m[schedule[6]], m[schedule[7]]);
    // Mix the diagonals.
    g(state, 0, 5, 10, 15, m[schedule[8]], m[schedule[9]]);
    g(state, 1, 6, 11, 12, m[schedule[10]], m[schedule[11]]);
    g(state, 2, 7, 8, 13, m[schedule[12]], m[schedule[13]]);
    g(state, 3, 4, 9, 14, m[schedule[14]], m[schedule[15]]);
}

fn compress_inner(
    chaining_value: &[u32; 8],
    block_words: &[u32; 16],
    offset: u64,
    block_len: u32,
    flags: u32,
) -> [u32; 16] {
    let mut state = [
        chaining_value[0],
        chaining_value[1],
        chaining_value[2],
        chaining_value[3],
        chaining_value[4],
        chaining_value[5],
        chaining_value[6],
        chaining_value[7],
        IV[0],
        IV[1],
        IV[2],
        IV[3],
        IV[4] ^ (offset as u32),
        IV[5] ^ ((offset >> 32) as u32),
        IV[6] ^ block_len,
        IV[7] ^ flags,
    ];
    for r in 0..ROUNDS {
        round(&mut state, &block_words, &MSG_SCHEDULE[r]);
    }
    state
}

// The standard compression function updates an 8-word chaining value in place.
fn compress(
    chaining_value: &mut [u32; 8],
    block_words: &[u32; 16],
    offset: u64,
    block_len: u32,
    flags: u32,
) {
    let state =
        compress_inner(chaining_value, block_words, offset, block_len, flags);
    for i in 0..8 {
        chaining_value[i] = state[i] ^ state[i + 8];
    }
}

// The extended compression function returns a new 16-word extended output.
// Note that the first 8 words of output are the same as with compress().
// Implementations that do not support extendable output can omit this.
fn compress_extended(
    chaining_value: &[u32; 8],
    block_words: &[u32; 16],
    offset: u64,
    block_len: u32,
    flags: u32,
) -> [u32; 16] {
    let mut state =
        compress_inner(chaining_value, block_words, offset, block_len, flags);
    for i in 0..8 {
        state[i] ^= state[i + 8];
        state[i + 8] ^= chaining_value[i];
    }
    state
}

// The output of a chunk or parent node, which could be an 8-word chaining
// value or any number of root output bytes.
struct Output {
    input_chaining_value: [u32; 8],
    block_words: [u32; 16],
    offset: u64,
    block_len: u32,
    flags: u32,
}

impl Output {
    fn chaining_value(&self) -> [u32; 8] {
        let mut cv = self.input_chaining_value;
        compress(
            &mut cv,
            &self.block_words,
            self.offset,
            self.block_len,
            self.flags,
        );
        cv
    }

    fn root_output(&self, out_slice: &mut [u8]) {
        let mut offset = self.offset;
        for out_block in out_slice.chunks_mut(2 * OUT_LEN) {
            let words = compress_extended(
                &self.input_chaining_value,
                &self.block_words,
                offset,
                self.block_len,
                self.flags | ROOT,
            );
            little_endian_bytes_from_words(&words, out_block);
            offset += 2 * OUT_LEN as u64;
        }
    }
}

struct ChunkState {
    chaining_value: [u32; 8],
    offset: u64,
    block: [u8; BLOCK_LEN],
    block_len: u8,
    blocks_compressed: u8,
    flags: u32,
}

impl ChunkState {
    fn new(key: &[u32; 8], offset: u64, flags: u32) -> Self {
        Self {
            chaining_value: *key,
            offset,
            block: [0; BLOCK_LEN],
            block_len: 0,
            blocks_compressed: 0,
            flags,
        }
    }

    fn len(&self) -> usize {
        BLOCK_LEN * self.blocks_compressed as usize + self.block_len as usize
    }

    fn start_flag(&self) -> u32 {
        if self.blocks_compressed == 0 {
            CHUNK_START
        } else {
            0
        }
    }

    fn update(&mut self, mut input: &[u8]) {
        while !input.is_empty() {
            if self.block_len as usize == BLOCK_LEN {
                let mut block_words = [0; 16];
                words_from_litte_endian_bytes(&self.block, &mut block_words);
                let block_flags = self.start_flag() | self.flags;
                compress(
                    &mut self.chaining_value,
                    &block_words,
                    self.offset,
                    BLOCK_LEN as u32,
                    block_flags as u32,
                );
                self.blocks_compressed += 1;
                self.block = [0; BLOCK_LEN];
                self.block_len = 0;
            }

            let want = BLOCK_LEN - self.block_len as usize;
            let take = core::cmp::min(want, input.len());
            self.block[self.block_len as usize..][..take]
                .copy_from_slice(&input[..take]);
            self.block_len += take as u8;
            input = &input[take..];
        }
    }

    fn output(&self) -> Output {
        let mut block_words = [0; 16];
        words_from_litte_endian_bytes(&self.block, &mut block_words);
        let block_flags = self.flags | self.start_flag() | CHUNK_END;
        Output {
            input_chaining_value: self.chaining_value,
            block_words,
            block_len: self.block_len as u32,
            offset: self.offset,
            flags: block_flags,
        }
    }
}

fn parent_output(
    left_child_cv: &[u32; 8],
    right_child_cv: &[u32; 8],
    key: &[u32; 8],
    flags: u32,
) -> Output {
    let mut block_words = [0; 16];
    block_words[..8].copy_from_slice(left_child_cv);
    block_words[8..].copy_from_slice(right_child_cv);
    Output {
        input_chaining_value: *key,
        block_words,
        offset: 0,                   // Always 0 for parent nodes.
        block_len: BLOCK_LEN as u32, // Always BLOCK_LEN (64) for parent nodes.
        flags: PARENT | flags,
    }
}

/// An incremental hasher that can accept any number of writes.
pub struct Hasher {
    chunk_state: ChunkState,
    key: [u32; 8],
    subtree_stack: [[u32; 8]; 53], // Space for 53 subtree chaining values:
    num_subtrees: u8,              // 2^53 * CHUNK_LEN = 2^64
}

impl Hasher {
    fn new_internal(key: &[u32; 8], flags: u32) -> Self {
        Self {
            chunk_state: ChunkState::new(key, 0, flags),
            key: *key,
            subtree_stack: [[0; 8]; 53],
            num_subtrees: 0,
        }
    }

    /// Construct a new `Hasher` for the default **hash** mode.
    pub fn new() -> Self {
        Self::new_internal(&IV, 0)
    }

    /// Construct a new `Hasher` for the **keyed_hash** mode.
    pub fn new_keyed(key: &[u8; KEY_LEN]) -> Self {
        let mut key_words = [0; 8];
        words_from_litte_endian_bytes(key, &mut key_words);
        Self::new_internal(&key_words, KEYED_HASH)
    }

    /// Construct a new `Hasher` for the **derive_key** mode.
    pub fn new_derive_key(key: &[u8; KEY_LEN]) -> Self {
        let mut key_words = [0; 8];
        words_from_litte_endian_bytes(key, &mut key_words);
        Self::new_internal(&key_words, DERIVE_KEY)
    }

    // Pop the top two subtree chaining values off the stack, compress them
    // into a parent CV, and put that new CV on top of the stack.
    fn merge_two_subtrees(&mut self) {
        let left_child = &self.subtree_stack[self.num_subtrees as usize - 2];
        let right_child = &self.subtree_stack[self.num_subtrees as usize - 1];
        let parent_hash = parent_output(
            left_child,
            right_child,
            &self.key,
            self.chunk_state.flags,
        )
        .chaining_value();
        self.subtree_stack[self.num_subtrees as usize - 2] = parent_hash;
        self.num_subtrees -= 1;
    }

    fn push_chunk_chaining_value(&mut self, cv: &[u32; 8], total_bytes: u64) {
        self.subtree_stack[self.num_subtrees as usize] = *cv;
        self.num_subtrees += 1;
        // After pushing the new chunk chaining value onto the stack, assemble
        // and compress as many parent nodes as we can. The number of 1 bits in
        // the total number of chunks so far is the same as the number of CVs
        // that should remain in the stack after this is done.
        let total_chunks = total_bytes / CHUNK_LEN as u64;
        while self.num_subtrees as usize > total_chunks.count_ones() as usize {
            self.merge_two_subtrees();
        }
    }

    /// Add input to the hash state. This can be called any number of times.
    pub fn update(&mut self, mut input: &[u8]) {
        while !input.is_empty() {
            if self.chunk_state.len() == CHUNK_LEN {
                let chunk_cv = self.chunk_state.output().chaining_value();
                let new_chunk_offset =
                    self.chunk_state.offset + CHUNK_LEN as u64;
                self.push_chunk_chaining_value(&chunk_cv, new_chunk_offset);
                self.chunk_state = ChunkState::new(
                    &self.key,
                    new_chunk_offset,
                    self.chunk_state.flags,
                );
            }

            let want = CHUNK_LEN - self.chunk_state.len();
            let take = core::cmp::min(want, input.len());
            self.chunk_state.update(&input[..take]);
            input = &input[take..];
        }
    }

    /// Finalize the hash and return the default 32-byte output.
    pub fn finalize(&self) -> [u8; OUT_LEN] {
        let mut bytes = [0; OUT_LEN];
        self.finalize_extended(&mut bytes);
        bytes
    }

    /// Finalize the hash and write any number of output bytes.
    pub fn finalize_extended(&self, out_slice: &mut [u8]) {
        // If the subtree stack is empty, then the current chunk is the root.
        if self.num_subtrees == 0 {
            self.chunk_state.output().root_output(out_slice);
            return;
        }

        // Otherwise, finalize the current chunk, and then merge all the
        // subtrees along the right edge of the tree.
        let mut right_child = self.chunk_state.output().chaining_value();
        let mut subtrees_remaining = self.num_subtrees as usize;
        loop {
            let left_child = &self.subtree_stack[subtrees_remaining - 1];
            let output = parent_output(
                left_child,
                &right_child,
                &self.key,
                self.chunk_state.flags,
            );
            if subtrees_remaining == 1 {
                output.root_output(out_slice);
                return;
            }
            right_child = output.chaining_value();
            subtrees_remaining -= 1;
        }
    }
}
