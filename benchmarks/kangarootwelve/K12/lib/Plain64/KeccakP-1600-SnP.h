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
#define KeccakP1600_stateAlignment      8
#define KeccakP1600_12rounds_FastLoop_supported
#define KeccakP1600_disableParallelism

const char * KeccakP1600_GetImplementation();
void KeccakP1600_Initialize(void *state);
void KeccakP1600_AddByte(void *state, unsigned char data, unsigned int offset);
void KeccakP1600_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length);
void KeccakP1600_Permute_12rounds(void *state);
void KeccakP1600_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length);
size_t KeccakP1600_12rounds_FastLoop_Absorb(void *state, unsigned int laneCount, const unsigned char *data, size_t dataByteLen);

// Instead of defining proxy functions which do nothing, simply rename the
// symbols of the opt64 implementation where they are used.
#define KeccakP1600_opt64_Initialize KeccakP1600_Initialize
#define KeccakP1600_opt64_AddByte KeccakP1600_AddByte
#define KeccakP1600_opt64_AddBytes KeccakP1600_AddBytes
#define KeccakP1600_opt64_Permute_12rounds KeccakP1600_Permute_12rounds
#define KeccakP1600_opt64_ExtractBytes KeccakP1600_ExtractBytes
#define KeccakP1600_opt64_12rounds_FastLoop_Absorb KeccakP1600_12rounds_FastLoop_Absorb

#endif
