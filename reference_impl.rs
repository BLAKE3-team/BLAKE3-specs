//! This reference implementation is written to be short and simple. It is not
//! optimized for performance, and it does not use any parallelism. The
//! throughput of this implementation is on par with OpenSSL SHA-256.

const OUT_LEN: usize = 32;
const KEY_LEN: usize = 32;
const BLOCK_LEN: usize = 64;
const CHUNK_LEN: usize = 2048;
const ROUNDS: usize = 7;

// The domain separation flags distinguish different node types and also the
// keyed and key derivation modes. Flags are combined with exclusive-or.
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

// Note that this conversion is little-endian.
fn words_from_bytes(bytes: &[u8], words: &mut [u32]) {
    for (b, w) in bytes.chunks_exact(4).zip(words.iter_mut()) {
        *w = u32::from_le_bytes(core::convert::TryInto::try_into(b).unwrap());
    }
}

// Note that this conversion is little-endian.
fn bytes_from_words(words: &[u32], bytes: &mut [u8]) {
    for (w, b) in words.iter().zip(bytes.chunks_exact_mut(4)) {
        b.copy_from_slice(&w.to_le_bytes());
    }
}

// The mixing function, also called G, mixes either a column or a diagonal.
fn mix(
    state: &mut [u32; 16],
    a: usize,
    b: usize,
    c: usize,
    d: usize,
    m1: u32,
    m2: u32,
) {
    state[a] = state[a].wrapping_add(state[b]).wrapping_add(m1);
    state[d] = (state[d] ^ state[a]).rotate_right(16);
    state[c] = state[c].wrapping_add(state[d]);
    state[b] = (state[b] ^ state[c]).rotate_right(12);
    state[a] = state[a].wrapping_add(state[b]).wrapping_add(m2);
    state[d] = (state[d] ^ state[a]).rotate_right(8);
    state[c] = state[c].wrapping_add(state[d]);
    state[b] = (state[b] ^ state[c]).rotate_right(7);
}

// The round function mixes all the columns, and then all the diagonals.
fn round(state: &mut [u32; 16], msg: &[u32; 16], schedule: &[usize; 16]) {
    // Mix the columns.
    mix(state, 0, 4, 8, 12, msg[schedule[0]], msg[schedule[1]]);
    mix(state, 1, 5, 9, 13, msg[schedule[2]], msg[schedule[3]]);
    mix(state, 2, 6, 10, 14, msg[schedule[4]], msg[schedule[5]]);
    mix(state, 3, 7, 11, 15, msg[schedule[6]], msg[schedule[7]]);
    // Mix the diagonals.
    mix(state, 0, 5, 10, 15, msg[schedule[8]], msg[schedule[9]]);
    mix(state, 1, 6, 11, 12, msg[schedule[10]], msg[schedule[11]]);
    mix(state, 2, 7, 8, 13, msg[schedule[12]], msg[schedule[13]]);
    mix(state, 3, 4, 9, 14, msg[schedule[14]], msg[schedule[15]]);
}

// The compression function initializes a 16-word state and then performs 7
// rounds of compression.
fn compress_inner(
    chaining_value: &[u32; 8],
    block_words: &[u32; 16],
    block_len: usize,
    offset: u64,
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
        IV[6] ^ block_len as u32,
        IV[7] ^ flags,
    ];
    for r in 0..ROUNDS {
        round(&mut state, &block_words, &MSG_SCHEDULE[r]);
    }
    state
}

// The standard compression function updates a chaining value in place.
fn compress(
    chaining_value: &mut [u32; 8],
    block_words: &[u32; 16],
    block_len: usize,
    offset: u64,
    flags: u32,
) {
    let state =
        compress_inner(chaining_value, block_words, block_len, offset, flags);
    for i in 0..8 {
        chaining_value[i] = state[i] ^ state[i + 8];
    }
}

// The extended compression function writes up to 64 bytes of output. Note that
// the first 32 bytes are the same as the new chaining value returned by the
// standard compression function.
fn compress_extended(
    chaining_value: &[u32; 8],
    block_words: &[u32; 16],
    block_len: usize,
    offset: u64,
    flags: u32,
    output: &mut [u8],
) {
    let mut state =
        compress_inner(chaining_value, block_words, block_len, offset, flags);
    for i in 0..8 {
        state[i] ^= state[i + 8];
        state[i + 8] ^= chaining_value[i];
    }
    bytes_from_words(&state, output);
}

// An Output represents either the final compression step of a chunk, or the
// single compression step a parent node. It can produce either a 32-byte
// chaining value, or any number of extended output bytes.
struct Output {
    prev_chaining_value: [u32; 8],
    block_words: [u32; 16],
    block_len: usize,
    offset: u64,
    flags: u32,
}

impl Output {
    fn chaining_value(&self) -> [u32; 8] {
        let mut chaining_value = self.prev_chaining_value;
        compress(
            &mut chaining_value,
            &self.block_words,
            self.block_len,
            self.offset,
            self.flags,
        );
        chaining_value
    }

    fn extend_output(&self, output: &mut [u8]) {
        let mut offset = self.offset;
        for out_slice in output.chunks_mut(2 * OUT_LEN) {
            compress_extended(
                &self.prev_chaining_value,
                &self.block_words,
                self.block_len,
                offset,
                self.flags,
                out_slice,
            );
            offset += 2 * OUT_LEN as u64;
        }
    }
}

// The incremental hash state of the current chunk.
struct ChunkState {
    chaining_value: [u32; 8],
    offset: u64,
    total_len: usize,
    block: [u8; BLOCK_LEN],
    block_len: usize,
    flags: u32,
}

impl ChunkState {
    fn new(key: &[u32; 8], offset: u64, flags: u32) -> Self {
        Self {
            chaining_value: *key,
            offset,
            total_len: 0,
            block: [0; BLOCK_LEN],
            block_len: 0,
            flags,
        }
    }

    // The CHUNK_START domain flag, if this is the first block of the chunk.
    fn start_flag(&self) -> u32 {
        if self.total_len <= BLOCK_LEN {
            CHUNK_START
        } else {
            0
        }
    }

    // Add input bytes to this chunk.
    fn update(&mut self, mut input: &[u8]) {
        while !input.is_empty() {
            // If the buffer is full, and there's more input coming, compress
            // it and clear it.
            if self.block_len == BLOCK_LEN {
                let mut block_words = [0; 16];
                words_from_bytes(&self.block, &mut block_words);
                let block_flags = self.start_flag() | self.flags;
                compress(
                    &mut self.chaining_value,
                    &block_words,
                    BLOCK_LEN,
                    self.offset,
                    block_flags,
                );
                self.block = [0; BLOCK_LEN];
                self.block_len = 0;
            }

            // Buffer as many input bytes as possible.
            let want = BLOCK_LEN - self.block_len;
            let take = core::cmp::min(want, input.len());
            self.block[self.block_len as usize..][..take]
                .copy_from_slice(&input[..take]);
            self.block_len += take;
            self.total_len += take;
            input = &input[take..];
        }
    }

    // Construct the Output object for this chunk. That object will perform the
    // final compression.
    fn finalize(&self, is_root: bool) -> Output {
        let mut block_words = [0; 16];
        words_from_bytes(&self.block, &mut block_words);
        let root_flag = if is_root { ROOT } else { 0 };
        let block_flags =
            self.flags | self.start_flag() | CHUNK_END | root_flag;
        Output {
            prev_chaining_value: self.chaining_value,
            block_words,
            block_len: self.block_len,
            offset: self.offset,
            flags: block_flags,
        }
    }
}

// Construct a parent Output from its left and right child chaining values.
fn hash_parent(
    left_child: &[u32; 8],
    right_child: &[u32; 8],
    key: &[u32; 8],
    is_root: bool,
    flags: u32,
) -> Output {
    let root_flag = if is_root { ROOT } else { 0 };
    let mut block_words = [0; 16];
    block_words[..8].copy_from_slice(left_child);
    block_words[8..].copy_from_slice(right_child);
    Output {
        prev_chaining_value: *key,
        block_words,
        block_len: BLOCK_LEN,
        // Note that the offset parameter is always zero for parent nodes.
        offset: 0,
        flags: PARENT | root_flag | flags,
    }
}

/// An incremental BLAKE3 hash state, which can accept any number of writes.
/// The maximum input length is 2^64-1 bytes.
pub struct Hasher {
    // Space for 53 subtree chaining values: 2^53 * CHUNK_LEN = 2^64
    subtree_stack: [[u32; 8]; 53],
    num_subtrees: usize,
    chunk_state: ChunkState,
    key: [u32; 8],
}

impl Hasher {
    fn new_internal(key: &[u32; 8], flags: u32) -> Self {
        Self {
            subtree_stack: [[0; 8]; 53],
            num_subtrees: 0,
            chunk_state: ChunkState::new(key, 0, flags),
            key: *key,
        }
    }

    /// Construct a new Hasher for the default, unkeyed hash function.
    pub fn new() -> Self {
        Self::new_internal(&IV, 0)
    }

    /// Construct a new Hasher for the keyed hash function.
    pub fn new_keyed(key: &[u8; KEY_LEN]) -> Self {
        let mut key_words = [0; 8];
        words_from_bytes(key, &mut key_words);
        Self::new_internal(&key_words, KEYED_HASH)
    }

    /// Construct a new Hasher for the key derivation function. The input
    /// supplied to update() should then be a globally unique,
    /// application-specific context string.
    pub fn new_derive_key(key: &[u8; KEY_LEN]) -> Self {
        let mut key_words = [0; 8];
        words_from_bytes(key, &mut key_words);
        Self::new_internal(&key_words, DERIVE_KEY)
    }

    // Take two subtree hashes off the end of the stack, compress them into a
    // parent hash, and put that hash back on the stack.
    fn merge_two_subtrees(&mut self) {
        let left_child = &self.subtree_stack[self.num_subtrees as usize - 2];
        let right_child = &self.subtree_stack[self.num_subtrees as usize - 1];
        let parent_hash = hash_parent(
            left_child,
            right_child,
            &self.key,
            false,
            self.chunk_state.flags,
        )
        .chaining_value();
        self.subtree_stack[self.num_subtrees as usize - 2] = parent_hash;
        self.num_subtrees -= 1;
    }

    // Add a finalized chunk chaining value to the subtree stack.
    fn push_chunk_chaining_value(&mut self, cv: &[u32; 8], total_bytes: u64) {
        self.subtree_stack[self.num_subtrees as usize] = *cv;
        self.num_subtrees += 1;
        // After pushing the new chunk hash onto the subtree stack, hash as
        // many parent nodes as we can. We know there is at least one more
        // chunk to come, so these are non-root parent hashes. The number of 1
        // bits in the total number of chunks so far is the same as the number
        // of subtrees that should remain in the stack.
        let total_chunks = total_bytes / CHUNK_LEN as u64;
        while self.num_subtrees > total_chunks.count_ones() as usize {
            self.merge_two_subtrees();
        }
    }

    /// Add input to the hash state. This can be called any number of times.
    pub fn update(&mut self, mut input: &[u8]) {
        while !input.is_empty() {
            // If the current chunk is full, and there's more input coming,
            // finalize it and push its hash into the subtree stack.
            if self.chunk_state.total_len == CHUNK_LEN {
                let chunk_cv =
                    self.chunk_state.finalize(false).chaining_value();
                let new_chunk_offset =
                    self.chunk_state.offset + CHUNK_LEN as u64;
                self.push_chunk_chaining_value(&chunk_cv, new_chunk_offset);
                self.chunk_state = ChunkState::new(
                    &self.key,
                    new_chunk_offset,
                    self.chunk_state.flags,
                );
            }

            // Add as many bytes as possible to the current chunk.
            let want = CHUNK_LEN - self.chunk_state.total_len;
            let take = core::cmp::min(want, input.len());
            self.chunk_state.update(&input[..take]);
            input = &input[take..];
        }
    }

    // Construct the root Output object, with the ROOT flag set. If there is
    // only one chunk of input, this it the Output of that chunk. Otherwise it
    // is the output of the root parent node.
    fn finalize_inner(&self) -> Output {
        // If the subtree stack is empty, then the current chunk is the root.
        if self.num_subtrees == 0 {
            return self.chunk_state.finalize(true);
        }

        // Otherwise, finish the current chunk, and then merge all the subtrees
        // along the right edge of the tree. Unlike update(), these subtrees
        // may not be complete.
        let mut right_child = self.chunk_state.finalize(false).chaining_value();
        let mut remaining = self.num_subtrees;
        loop {
            let left_child = &self.subtree_stack[remaining - 1];
            let is_root = remaining == 1;
            let output = hash_parent(
                left_child,
                &right_child,
                &self.key,
                is_root,
                self.chunk_state.flags,
            );
            if is_root {
                return output;
            }
            right_child = output.chaining_value();
            remaining -= 1;
        }
    }

    /// Finalize the hash and return a 32-byte output. Note that this method
    /// does not mutate the Hasher, and calling it twice gives the same result.
    pub fn finalize(&self) -> [u8; OUT_LEN] {
        let cv = self.finalize_inner().chaining_value();
        let mut bytes = [0; OUT_LEN];
        bytes_from_words(&cv, &mut bytes);
        bytes
    }

    /// Finalize the hash and write any number of output bytes. Note that this
    /// method does not mutate the Hasher, and calling it twice gives the same
    /// result.
    pub fn finalize_extended(&self, output: &mut [u8]) {
        self.finalize_inner().extend_output(output);
    }
}
