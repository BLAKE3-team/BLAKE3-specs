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

#ifndef _KeccakP_1600_SnP_h_
#define _KeccakP_1600_SnP_h_

/* Keccak-p[1600] */

#define KeccakP1600_stateSizeInBytes    200
#define KeccakP1600_stateAlignment      64
#define KeccakP1600_12rounds_FastLoop_supported

const char * KeccakP1600_GetImplementation();
void KeccakP1600_Initialize(void *state);
void KeccakP1600_AddByte(void *state, unsigned char data, unsigned int offset);
void KeccakP1600_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length);
void KeccakP1600_Permute_12rounds(void *state);
void KeccakP1600_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length);
size_t KeccakP1600_12rounds_FastLoop_Absorb(void *state, unsigned int laneCount, const unsigned char *data, size_t dataByteLen);

void KeccakP1600_AVX512_Initialize(void *state);
void KeccakP1600_AVX512_AddByte(void *state, unsigned char data, unsigned int offset);
void KeccakP1600_AVX512_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length);
void KeccakP1600_AVX512_Permute_12rounds(void *state);
void KeccakP1600_AVX512_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length);
size_t KeccakP1600_AVX512_12rounds_FastLoop_Absorb(void *state, unsigned int laneCount, const unsigned char *data, size_t dataByteLen);

void KeccakP1600_AVX2_Initialize(void *state);
void KeccakP1600_AVX2_AddByte(void *state, unsigned char data, unsigned int offset);
void KeccakP1600_AVX2_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length);
void KeccakP1600_AVX2_Permute_12rounds(void *state);
void KeccakP1600_AVX2_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length);
size_t KeccakP1600_AVX2_12rounds_FastLoop_Absorb(void *state, unsigned int laneCount, const unsigned char *data, size_t dataByteLen);

void KeccakP1600_opt64_Initialize(void *state);
void KeccakP1600_opt64_AddByte(void *state, unsigned char data, unsigned int offset);
void KeccakP1600_opt64_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length);
void KeccakP1600_opt64_Permute_12rounds(void *state);
void KeccakP1600_opt64_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length);
size_t KeccakP1600_opt64_12rounds_FastLoop_Absorb(void *state, unsigned int laneCount, const unsigned char *data, size_t dataByteLen);

/* Keccak-p[1600]×2 */

int KeccakP1600times2_IsAvailable();
const char * KeccakP1600times2_GetImplementation();

/* Keccak-p[1600]×4 */

int KeccakP1600times4_IsAvailable();
const char * KeccakP1600times4_GetImplementation();

/* Keccak-p[1600]×8 */

int KeccakP1600times8_IsAvailable();
const char * KeccakP1600times8_GetImplementation();

#endif
