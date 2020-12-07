/*
K12 based on the eXtended Keccak Code Package (XKCP)
https://github.com/XKCP/XKCP

The Keccak-p permutations, designed by Guido Bertoni, Joan Daemen, Michaël Peeters and Gilles Van Assche.

Implementation by Gilles Van Assche and Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to the Keccak Team website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/

---

Please refer to the XKCP for more details.
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "brg_endian.h"
#include "KeccakP-1600-SnP.h"

#ifdef KeccakP1600_disableParallelism
#undef KeccakP1600_enable_simd_options
#else

// Forward declaration
void KangarooTwelve_SetProcessorCapabilities();
#ifdef KeccakP1600_enable_simd_options
int K12_SSSE3_requested_disabled = 0;
int K12_AVX2_requested_disabled = 0;
int K12_AVX512_requested_disabled = 0;
#endif  // KeccakP1600_enable_simd_options
int K12_enableSSSE3 = 0;
int K12_enableAVX2 = 0;
int K12_enableAVX512 = 0;

/* ---------------------------------------------------------------- */

void KangarooTwelve_SSSE3_Process2Leaves(const unsigned char *input, unsigned char *output);
void KangarooTwelve_AVX512_Process2Leaves(const unsigned char *input, unsigned char *output);

int KeccakP1600times2_IsAvailable()
{
    int result = 0;
    result |= K12_enableAVX512;
    result |= K12_enableSSSE3;
    return result;
}

const char * KeccakP1600times2_GetImplementation()
{
    if (K12_enableAVX512) {
        return "AVX-512 implementation";
    } else if (K12_enableSSSE3) {
        return "SSSE3 implementation";
    } else {
        return "";
    }
}

void KangarooTwelve_Process2Leaves(const unsigned char *input, unsigned char *output)
{
    if (K12_enableAVX512) {
        KangarooTwelve_AVX512_Process2Leaves(input, output);
    } else if (K12_enableSSSE3) {
        KangarooTwelve_SSSE3_Process2Leaves(input, output);
    }
}


void KangarooTwelve_AVX2_Process4Leaves(const unsigned char *input, unsigned char *output);
void KangarooTwelve_AVX512_Process4Leaves(const unsigned char *input, unsigned char *output);

int KeccakP1600times4_IsAvailable()
{
    int result = 0;
    result |= K12_enableAVX512;
    result |= K12_enableAVX2;
    return result;
}

const char * KeccakP1600times4_GetImplementation()
{
    if (K12_enableAVX512) {
        return "AVX-512 implementation";
    } else if (K12_enableAVX2) {
        return "AVX2 implementation";
    } else {
        return "";
    }
}

void KangarooTwelve_Process4Leaves(const unsigned char *input, unsigned char *output)
{
    if (K12_enableAVX512) {
        KangarooTwelve_AVX512_Process4Leaves(input, output);
    } else if (K12_enableAVX2) {
        KangarooTwelve_AVX2_Process4Leaves(input, output);
    }
}


void KangarooTwelve_AVX512_Process8Leaves(const unsigned char *input, unsigned char *output);

int KeccakP1600times8_IsAvailable()
{
    int result = 0;
    result |= K12_enableAVX512;
    return result;
}

const char * KeccakP1600times8_GetImplementation()
{
    if (K12_enableAVX512) {
        return "AVX-512 implementation";
    } else {
        return "";
    }
}

void KangarooTwelve_Process8Leaves(const unsigned char *input, unsigned char *output)
{
    if (K12_enableAVX512)
        KangarooTwelve_AVX512_Process8Leaves(input, output);
}

#endif  // KeccakP1600_disableParallelism

const char * KeccakP1600_GetImplementation()
{
    if (K12_enableAVX512)
        return "AVX-512 implementation";
    else
#ifndef KeccakP1600_noAssembly
    if (K12_enableAVX2)
        return "AVX2 implementation";
    else
#endif
        return "generic 64-bit implementation";
}

void KeccakP1600_Initialize(void *state)
{
    KangarooTwelve_SetProcessorCapabilities();
    if (K12_enableAVX512)
        KeccakP1600_AVX512_Initialize(state);
    else
#ifndef KeccakP1600_noAssembly
    if (K12_enableAVX2)
        KeccakP1600_AVX2_Initialize(state);
    else
#endif
        KeccakP1600_opt64_Initialize(state);
}

void KeccakP1600_AddByte(void *state, unsigned char data, unsigned int offset)
{
    if (K12_enableAVX512)
        ((unsigned char*)(state))[offset] ^= data;
    else
#ifndef KeccakP1600_noAssembly
    if (K12_enableAVX2)
        KeccakP1600_AVX2_AddByte(state, data, offset);
    else
#endif
        KeccakP1600_opt64_AddByte(state, data, offset);
}

void KeccakP1600_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    if (K12_enableAVX512)
        KeccakP1600_AVX512_AddBytes(state, data, offset, length);
    else
#ifndef KeccakP1600_noAssembly
    if (K12_enableAVX2)
        KeccakP1600_AVX2_AddBytes(state, data, offset, length);
    else
#endif
        KeccakP1600_opt64_AddBytes(state, data, offset, length);
}

void KeccakP1600_Permute_12rounds(void *state)
{
    if (K12_enableAVX512)
        KeccakP1600_AVX512_Permute_12rounds(state);
    else
#ifndef KeccakP1600_noAssembly
    if (K12_enableAVX2)
        KeccakP1600_AVX2_Permute_12rounds(state);
    else
#endif
        KeccakP1600_opt64_Permute_12rounds(state);
}

void KeccakP1600_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length)
{
    if (K12_enableAVX512)
        KeccakP1600_AVX512_ExtractBytes(state, data, offset, length);
    else
#ifndef KeccakP1600_noAssembly
    if (K12_enableAVX2)
        KeccakP1600_AVX2_ExtractBytes(state, data, offset, length);
    else
#endif
        KeccakP1600_opt64_ExtractBytes(state, data, offset, length);
}

size_t KeccakP1600_12rounds_FastLoop_Absorb(void *state, unsigned int laneCount, const unsigned char *data, size_t dataByteLen)
{
    if (K12_enableAVX512)
        return KeccakP1600_AVX512_12rounds_FastLoop_Absorb(state, laneCount, data, dataByteLen);
    else
#ifndef KeccakP1600_noAssembly
    if (K12_enableAVX2)
        return KeccakP1600_AVX2_12rounds_FastLoop_Absorb(state, laneCount, data, dataByteLen);
    else
#endif
        return KeccakP1600_opt64_12rounds_FastLoop_Absorb(state, laneCount, data, dataByteLen);
}

/* ---------------------------------------------------------------- */

/* Processor capability detection code by Samuel Neves and Jack O'Connor, see
 * https://github.com/BLAKE3-team/BLAKE3/blob/master/c/blake3_dispatch.c
 */

#if defined(__x86_64__) || defined(_M_X64)
#define IS_X86
#define IS_X86_64
#endif

#if defined(__i386__) || defined(_M_IX86)
#define IS_X86
#define IS_X86_32
#endif

#if defined(IS_X86)
static uint64_t xgetbv() {
#if defined(_MSC_VER)
  return _xgetbv(0);
#else
  uint32_t eax = 0, edx = 0;
  __asm__ __volatile__("xgetbv\n" : "=a"(eax), "=d"(edx) : "c"(0));
  return ((uint64_t)edx << 32) | eax;
#endif
}

static void cpuid(uint32_t out[4], uint32_t id) {
#if defined(_MSC_VER)
  __cpuid((int *)out, id);
#elif defined(__i386__) || defined(_M_IX86)
  __asm__ __volatile__("movl %%ebx, %1\n"
                       "cpuid\n"
                       "xchgl %1, %%ebx\n"
                       : "=a"(out[0]), "=r"(out[1]), "=c"(out[2]), "=d"(out[3])
                       : "a"(id));
#else
  __asm__ __volatile__("cpuid\n"
                       : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3])
                       : "a"(id));
#endif
}

static void cpuidex(uint32_t out[4], uint32_t id, uint32_t sid) {
#if defined(_MSC_VER)
  __cpuidex((int *)out, id, sid);
#elif defined(__i386__) || defined(_M_IX86)
  __asm__ __volatile__("movl %%ebx, %1\n"
                       "cpuid\n"
                       "xchgl %1, %%ebx\n"
                       : "=a"(out[0]), "=r"(out[1]), "=c"(out[2]), "=d"(out[3])
                       : "a"(id), "c"(sid));
#else
  __asm__ __volatile__("cpuid\n"
                       : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3])
                       : "a"(id), "c"(sid));
#endif
}

#endif

enum cpu_feature {
  SSE2 = 1 << 0,
  SSSE3 = 1 << 1,
  SSE41 = 1 << 2,
  AVX = 1 << 3,
  AVX2 = 1 << 4,
  AVX512F = 1 << 5,
  AVX512VL = 1 << 6,
  /* ... */
  UNDEFINED = 1 << 30
};

static enum cpu_feature g_cpu_features = UNDEFINED;

static enum cpu_feature
    get_cpu_features(void) {

  if (g_cpu_features != UNDEFINED) {
    return g_cpu_features;
  } else {
#if defined(IS_X86)
    uint32_t regs[4] = {0};
    uint32_t *eax = &regs[0], *ebx = &regs[1], *ecx = &regs[2], *edx = &regs[3];
    (void)edx;
    enum cpu_feature features = 0;
    cpuid(regs, 0);
    const int max_id = *eax;
    cpuid(regs, 1);
#if defined(__amd64__) || defined(_M_X64)
    features |= SSE2;
#else
    if (*edx & (1UL << 26))
      features |= SSE2;
#endif
    if (*ecx & (1UL << 0))
      features |= SSSE3;
    if (*ecx & (1UL << 19))
      features |= SSE41;

    if (*ecx & (1UL << 27)) { // OSXSAVE
      const uint64_t mask = xgetbv();
      if ((mask & 6) == 6) { // SSE and AVX states
        if (*ecx & (1UL << 28))
          features |= AVX;
        if (max_id >= 7) {
          cpuidex(regs, 7, 0);
          if (*ebx & (1UL << 5))
            features |= AVX2;
          if ((mask & 224) == 224) { // Opmask, ZMM_Hi256, Hi16_Zmm
            if (*ebx & (1UL << 31))
              features |= AVX512VL;
            if (*ebx & (1UL << 16))
              features |= AVX512F;
          }
        }
      }
    }
    g_cpu_features = features;
    return features;
#else
    /* How to detect NEON? */
    return 0;
#endif
  }
}

void KangarooTwelve_SetProcessorCapabilities()
{
    enum cpu_feature features = get_cpu_features();
    K12_enableSSSE3 = (features & SSSE3);
    K12_enableAVX2 = (features & AVX2);
    K12_enableAVX512 = (features & AVX512F) && (features & AVX512VL);
#ifdef KeccakP1600_enable_simd_options
    K12_enableSSSE3 = K12_enableSSSE3 && !K12_SSSE3_requested_disabled;
    K12_enableAVX2 = K12_enableAVX2 && !K12_AVX2_requested_disabled;
    K12_enableAVX512 = K12_enableAVX512 && !K12_AVX512_requested_disabled;
#endif  // KeccakP1600_enable_simd_options
}

#ifdef KeccakP1600_enable_simd_options
int KangarooTwelve_DisableSSSE3(void) {
    KangarooTwelve_SetProcessorCapabilities();
    K12_SSSE3_requested_disabled = 1;
    if (K12_enableSSSE3) {
        KangarooTwelve_SetProcessorCapabilities();
        return 1;  // SSSE3 was disabled on this call.
    } else {
        return 0;  // Nothing changed.
    }
}

int KangarooTwelve_DisableAVX2(void) {
    KangarooTwelve_SetProcessorCapabilities();
    K12_AVX2_requested_disabled = 1;
    if (K12_enableAVX2) {
        KangarooTwelve_SetProcessorCapabilities();
        return 1;  // AVX2 was disabled on this call.
    } else {
        return 0;  // Nothing changed.
    }
}

int KangarooTwelve_DisableAVX512(void) {
    KangarooTwelve_SetProcessorCapabilities();
    K12_AVX512_requested_disabled = 1;
    if (K12_enableAVX512) {
        KangarooTwelve_SetProcessorCapabilities();
        return 1;  // AVX512 was disabled on this call.
    } else {
        return 0;  // Nothing changed.
    }
}

void KangarooTwelve_EnableAllCpuFeatures(void) {
    K12_SSSE3_requested_disabled = 0;
    K12_AVX2_requested_disabled = 0;
    K12_AVX512_requested_disabled = 0;
    KangarooTwelve_SetProcessorCapabilities();
}
#endif  // KeccakP1600_enable_simd_options
