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
#include <immintrin.h>
#include "KeccakP-1600-SnP.h"
#include "align.h"

#define AVX2alignment 32

#define ANDnu256(a, b)          _mm256_andnot_si256(a, b)
#define CONST256(a)             _mm256_load_si256((const __m256i *)&(a))
#define CONST256_64(a)          _mm256_set1_epi64x(a)
#define LOAD256(a)              _mm256_load_si256((const __m256i *)&(a))
#define LOAD4_64(a, b, c, d)    _mm256_set_epi64x((uint64_t)(a), (uint64_t)(b), (uint64_t)(c), (uint64_t)(d))
#define ROL64in256(d, a, o)     d = _mm256_or_si256(_mm256_slli_epi64(a, o), _mm256_srli_epi64(a, 64-(o)))
#define ROL64in256_8(d, a)      d = _mm256_shuffle_epi8(a, CONST256(rho8))
#define ROL64in256_56(d, a)     d = _mm256_shuffle_epi8(a, CONST256(rho56))
static const uint64_t rho8[4] = {0x0605040302010007, 0x0E0D0C0B0A09080F, 0x1615141312111017, 0x1E1D1C1B1A19181F};
static const uint64_t rho56[4] = {0x0007060504030201, 0x080F0E0D0C0B0A09, 0x1017161514131211, 0x181F1E1D1C1B1A19};
#define STORE256(a, b)          _mm256_store_si256((__m256i *)&(a), b)
#define STORE256u(a, b)         _mm256_storeu_si256((__m256i *)&(a), b)
#define XOR256(a, b)            _mm256_xor_si256(a, b)
#define XOReq256(a, b)          a = _mm256_xor_si256(a, b)
#define UNPACKL( a, b )         _mm256_unpacklo_epi64((a), (b))
#define UNPACKH( a, b )         _mm256_unpackhi_epi64((a), (b))
#define PERM128( a, b, c )      _mm256_permute2f128_si256(a, b, c)
#define SHUFFLE64( a, b, c )    _mm256_castpd_si256(_mm256_shuffle_pd(_mm256_castsi256_pd(a), _mm256_castsi256_pd(b), c))
#define ZERO()                  _mm256_setzero_si256()

static ALIGN(AVX2alignment) const uint64_t KeccakP1600RoundConstants[24] = {
    0x0000000000000001ULL,
    0x0000000000008082ULL,
    0x800000000000808aULL,
    0x8000000080008000ULL,
    0x000000000000808bULL,
    0x0000000080000001ULL,
    0x8000000080008081ULL,
    0x8000000000008009ULL,
    0x000000000000008aULL,
    0x0000000000000088ULL,
    0x0000000080008009ULL,
    0x000000008000000aULL,
    0x000000008000808bULL,
    0x800000000000008bULL,
    0x8000000000008089ULL,
    0x8000000000008003ULL,
    0x8000000000008002ULL,
    0x8000000000000080ULL,
    0x000000000000800aULL,
    0x800000008000000aULL,
    0x8000000080008081ULL,
    0x8000000000008080ULL,
    0x0000000080000001ULL,
    0x8000000080008008ULL};

#define declareABCDE \
    __m256i Aba, Abe, Abi, Abo, Abu; \
    __m256i Aga, Age, Agi, Ago, Agu; \
    __m256i Aka, Ake, Aki, Ako, Aku; \
    __m256i Ama, Ame, Ami, Amo, Amu; \
    __m256i Asa, Ase, Asi, Aso, Asu; \
    __m256i Bba, Bbe, Bbi, Bbo, Bbu; \
    __m256i Bga, Bge, Bgi, Bgo, Bgu; \
    __m256i Bka, Bke, Bki, Bko, Bku; \
    __m256i Bma, Bme, Bmi, Bmo, Bmu; \
    __m256i Bsa, Bse, Bsi, Bso, Bsu; \
    __m256i Ca, Ce, Ci, Co, Cu; \
    __m256i Ca1, Ce1, Ci1, Co1, Cu1; \
    __m256i Da, De, Di, Do, Du; \
    __m256i Eba, Ebe, Ebi, Ebo, Ebu; \
    __m256i Ega, Ege, Egi, Ego, Egu; \
    __m256i Eka, Eke, Eki, Eko, Eku; \
    __m256i Ema, Eme, Emi, Emo, Emu; \
    __m256i Esa, Ese, Esi, Eso, Esu; \

#define prepareTheta \
    Ca = XOR256(Aba, XOR256(Aga, XOR256(Aka, XOR256(Ama, Asa)))); \
    Ce = XOR256(Abe, XOR256(Age, XOR256(Ake, XOR256(Ame, Ase)))); \
    Ci = XOR256(Abi, XOR256(Agi, XOR256(Aki, XOR256(Ami, Asi)))); \
    Co = XOR256(Abo, XOR256(Ago, XOR256(Ako, XOR256(Amo, Aso)))); \
    Cu = XOR256(Abu, XOR256(Agu, XOR256(Aku, XOR256(Amu, Asu)))); \

/* --- Theta Rho Pi Chi Iota Prepare-theta */
/* --- 64-bit lanes mapped to 64-bit words */
#define thetaRhoPiChiIotaPrepareTheta(i, A, E) \
    ROL64in256(Ce1, Ce, 1); \
    Da = XOR256(Cu, Ce1); \
    ROL64in256(Ci1, Ci, 1); \
    De = XOR256(Ca, Ci1); \
    ROL64in256(Co1, Co, 1); \
    Di = XOR256(Ce, Co1); \
    ROL64in256(Cu1, Cu, 1); \
    Do = XOR256(Ci, Cu1); \
    ROL64in256(Ca1, Ca, 1); \
    Du = XOR256(Co, Ca1); \
\
    XOReq256(A##ba, Da); \
    Bba = A##ba; \
    XOReq256(A##ge, De); \
    ROL64in256(Bbe, A##ge, 44); \
    XOReq256(A##ki, Di); \
    ROL64in256(Bbi, A##ki, 43); \
    E##ba = XOR256(Bba, ANDnu256(Bbe, Bbi)); \
    XOReq256(E##ba, CONST256_64(KeccakP1600RoundConstants[i])); \
    Ca = E##ba; \
    XOReq256(A##mo, Do); \
    ROL64in256(Bbo, A##mo, 21); \
    E##be = XOR256(Bbe, ANDnu256(Bbi, Bbo)); \
    Ce = E##be; \
    XOReq256(A##su, Du); \
    ROL64in256(Bbu, A##su, 14); \
    E##bi = XOR256(Bbi, ANDnu256(Bbo, Bbu)); \
    Ci = E##bi; \
    E##bo = XOR256(Bbo, ANDnu256(Bbu, Bba)); \
    Co = E##bo; \
    E##bu = XOR256(Bbu, ANDnu256(Bba, Bbe)); \
    Cu = E##bu; \
\
    XOReq256(A##bo, Do); \
    ROL64in256(Bga, A##bo, 28); \
    XOReq256(A##gu, Du); \
    ROL64in256(Bge, A##gu, 20); \
    XOReq256(A##ka, Da); \
    ROL64in256(Bgi, A##ka, 3); \
    E##ga = XOR256(Bga, ANDnu256(Bge, Bgi)); \
    XOReq256(Ca, E##ga); \
    XOReq256(A##me, De); \
    ROL64in256(Bgo, A##me, 45); \
    E##ge = XOR256(Bge, ANDnu256(Bgi, Bgo)); \
    XOReq256(Ce, E##ge); \
    XOReq256(A##si, Di); \
    ROL64in256(Bgu, A##si, 61); \
    E##gi = XOR256(Bgi, ANDnu256(Bgo, Bgu)); \
    XOReq256(Ci, E##gi); \
    E##go = XOR256(Bgo, ANDnu256(Bgu, Bga)); \
    XOReq256(Co, E##go); \
    E##gu = XOR256(Bgu, ANDnu256(Bga, Bge)); \
    XOReq256(Cu, E##gu); \
\
    XOReq256(A##be, De); \
    ROL64in256(Bka, A##be, 1); \
    XOReq256(A##gi, Di); \
    ROL64in256(Bke, A##gi, 6); \
    XOReq256(A##ko, Do); \
    ROL64in256(Bki, A##ko, 25); \
    E##ka = XOR256(Bka, ANDnu256(Bke, Bki)); \
    XOReq256(Ca, E##ka); \
    XOReq256(A##mu, Du); \
    ROL64in256_8(Bko, A##mu); \
    E##ke = XOR256(Bke, ANDnu256(Bki, Bko)); \
    XOReq256(Ce, E##ke); \
    XOReq256(A##sa, Da); \
    ROL64in256(Bku, A##sa, 18); \
    E##ki = XOR256(Bki, ANDnu256(Bko, Bku)); \
    XOReq256(Ci, E##ki); \
    E##ko = XOR256(Bko, ANDnu256(Bku, Bka)); \
    XOReq256(Co, E##ko); \
    E##ku = XOR256(Bku, ANDnu256(Bka, Bke)); \
    XOReq256(Cu, E##ku); \
\
    XOReq256(A##bu, Du); \
    ROL64in256(Bma, A##bu, 27); \
    XOReq256(A##ga, Da); \
    ROL64in256(Bme, A##ga, 36); \
    XOReq256(A##ke, De); \
    ROL64in256(Bmi, A##ke, 10); \
    E##ma = XOR256(Bma, ANDnu256(Bme, Bmi)); \
    XOReq256(Ca, E##ma); \
    XOReq256(A##mi, Di); \
    ROL64in256(Bmo, A##mi, 15); \
    E##me = XOR256(Bme, ANDnu256(Bmi, Bmo)); \
    XOReq256(Ce, E##me); \
    XOReq256(A##so, Do); \
    ROL64in256_56(Bmu, A##so); \
    E##mi = XOR256(Bmi, ANDnu256(Bmo, Bmu)); \
    XOReq256(Ci, E##mi); \
    E##mo = XOR256(Bmo, ANDnu256(Bmu, Bma)); \
    XOReq256(Co, E##mo); \
    E##mu = XOR256(Bmu, ANDnu256(Bma, Bme)); \
    XOReq256(Cu, E##mu); \
\
    XOReq256(A##bi, Di); \
    ROL64in256(Bsa, A##bi, 62); \
    XOReq256(A##go, Do); \
    ROL64in256(Bse, A##go, 55); \
    XOReq256(A##ku, Du); \
    ROL64in256(Bsi, A##ku, 39); \
    E##sa = XOR256(Bsa, ANDnu256(Bse, Bsi)); \
    XOReq256(Ca, E##sa); \
    XOReq256(A##ma, Da); \
    ROL64in256(Bso, A##ma, 41); \
    E##se = XOR256(Bse, ANDnu256(Bsi, Bso)); \
    XOReq256(Ce, E##se); \
    XOReq256(A##se, De); \
    ROL64in256(Bsu, A##se, 2); \
    E##si = XOR256(Bsi, ANDnu256(Bso, Bsu)); \
    XOReq256(Ci, E##si); \
    E##so = XOR256(Bso, ANDnu256(Bsu, Bsa)); \
    XOReq256(Co, E##so); \
    E##su = XOR256(Bsu, ANDnu256(Bsa, Bse)); \
    XOReq256(Cu, E##su); \
\

/* --- Theta Rho Pi Chi Iota */
/* --- 64-bit lanes mapped to 64-bit words */
#define thetaRhoPiChiIota(i, A, E) \
    ROL64in256(Ce1, Ce, 1); \
    Da = XOR256(Cu, Ce1); \
    ROL64in256(Ci1, Ci, 1); \
    De = XOR256(Ca, Ci1); \
    ROL64in256(Co1, Co, 1); \
    Di = XOR256(Ce, Co1); \
    ROL64in256(Cu1, Cu, 1); \
    Do = XOR256(Ci, Cu1); \
    ROL64in256(Ca1, Ca, 1); \
    Du = XOR256(Co, Ca1); \
\
    XOReq256(A##ba, Da); \
    Bba = A##ba; \
    XOReq256(A##ge, De); \
    ROL64in256(Bbe, A##ge, 44); \
    XOReq256(A##ki, Di); \
    ROL64in256(Bbi, A##ki, 43); \
    E##ba = XOR256(Bba, ANDnu256(Bbe, Bbi)); \
    XOReq256(E##ba, CONST256_64(KeccakP1600RoundConstants[i])); \
    XOReq256(A##mo, Do); \
    ROL64in256(Bbo, A##mo, 21); \
    E##be = XOR256(Bbe, ANDnu256(Bbi, Bbo)); \
    XOReq256(A##su, Du); \
    ROL64in256(Bbu, A##su, 14); \
    E##bi = XOR256(Bbi, ANDnu256(Bbo, Bbu)); \
    E##bo = XOR256(Bbo, ANDnu256(Bbu, Bba)); \
    E##bu = XOR256(Bbu, ANDnu256(Bba, Bbe)); \
\
    XOReq256(A##bo, Do); \
    ROL64in256(Bga, A##bo, 28); \
    XOReq256(A##gu, Du); \
    ROL64in256(Bge, A##gu, 20); \
    XOReq256(A##ka, Da); \
    ROL64in256(Bgi, A##ka, 3); \
    E##ga = XOR256(Bga, ANDnu256(Bge, Bgi)); \
    XOReq256(A##me, De); \
    ROL64in256(Bgo, A##me, 45); \
    E##ge = XOR256(Bge, ANDnu256(Bgi, Bgo)); \
    XOReq256(A##si, Di); \
    ROL64in256(Bgu, A##si, 61); \
    E##gi = XOR256(Bgi, ANDnu256(Bgo, Bgu)); \
    E##go = XOR256(Bgo, ANDnu256(Bgu, Bga)); \
    E##gu = XOR256(Bgu, ANDnu256(Bga, Bge)); \
\
    XOReq256(A##be, De); \
    ROL64in256(Bka, A##be, 1); \
    XOReq256(A##gi, Di); \
    ROL64in256(Bke, A##gi, 6); \
    XOReq256(A##ko, Do); \
    ROL64in256(Bki, A##ko, 25); \
    E##ka = XOR256(Bka, ANDnu256(Bke, Bki)); \
    XOReq256(A##mu, Du); \
    ROL64in256_8(Bko, A##mu); \
    E##ke = XOR256(Bke, ANDnu256(Bki, Bko)); \
    XOReq256(A##sa, Da); \
    ROL64in256(Bku, A##sa, 18); \
    E##ki = XOR256(Bki, ANDnu256(Bko, Bku)); \
    E##ko = XOR256(Bko, ANDnu256(Bku, Bka)); \
    E##ku = XOR256(Bku, ANDnu256(Bka, Bke)); \
\
    XOReq256(A##bu, Du); \
    ROL64in256(Bma, A##bu, 27); \
    XOReq256(A##ga, Da); \
    ROL64in256(Bme, A##ga, 36); \
    XOReq256(A##ke, De); \
    ROL64in256(Bmi, A##ke, 10); \
    E##ma = XOR256(Bma, ANDnu256(Bme, Bmi)); \
    XOReq256(A##mi, Di); \
    ROL64in256(Bmo, A##mi, 15); \
    E##me = XOR256(Bme, ANDnu256(Bmi, Bmo)); \
    XOReq256(A##so, Do); \
    ROL64in256_56(Bmu, A##so); \
    E##mi = XOR256(Bmi, ANDnu256(Bmo, Bmu)); \
    E##mo = XOR256(Bmo, ANDnu256(Bmu, Bma)); \
    E##mu = XOR256(Bmu, ANDnu256(Bma, Bme)); \
\
    XOReq256(A##bi, Di); \
    ROL64in256(Bsa, A##bi, 62); \
    XOReq256(A##go, Do); \
    ROL64in256(Bse, A##go, 55); \
    XOReq256(A##ku, Du); \
    ROL64in256(Bsi, A##ku, 39); \
    E##sa = XOR256(Bsa, ANDnu256(Bse, Bsi)); \
    XOReq256(A##ma, Da); \
    ROL64in256(Bso, A##ma, 41); \
    E##se = XOR256(Bse, ANDnu256(Bsi, Bso)); \
    XOReq256(A##se, De); \
    ROL64in256(Bsu, A##se, 2); \
    E##si = XOR256(Bsi, ANDnu256(Bso, Bsu)); \
    E##so = XOR256(Bso, ANDnu256(Bsu, Bsa)); \
    E##su = XOR256(Bsu, ANDnu256(Bsa, Bse)); \
\

#define initializeState(X) \
    X##ba = ZERO(); \
    X##be = ZERO(); \
    X##bi = ZERO(); \
    X##bo = ZERO(); \
    X##bu = ZERO(); \
    X##ga = ZERO(); \
    X##ge = ZERO(); \
    X##gi = ZERO(); \
    X##go = ZERO(); \
    X##gu = ZERO(); \
    X##ka = ZERO(); \
    X##ke = ZERO(); \
    X##ki = ZERO(); \
    X##ko = ZERO(); \
    X##ku = ZERO(); \
    X##ma = ZERO(); \
    X##me = ZERO(); \
    X##mi = ZERO(); \
    X##mo = ZERO(); \
    X##mu = ZERO(); \
    X##sa = ZERO(); \
    X##se = ZERO(); \
    X##si = ZERO(); \
    X##so = ZERO(); \
    X##su = ZERO(); \

#define XORdata16(X, data0, data1, data2, data3) \
    XOReq256(X##ba, LOAD4_64((data3)[ 0], (data2)[ 0], (data1)[ 0], (data0)[ 0])); \
    XOReq256(X##be, LOAD4_64((data3)[ 1], (data2)[ 1], (data1)[ 1], (data0)[ 1])); \
    XOReq256(X##bi, LOAD4_64((data3)[ 2], (data2)[ 2], (data1)[ 2], (data0)[ 2])); \
    XOReq256(X##bo, LOAD4_64((data3)[ 3], (data2)[ 3], (data1)[ 3], (data0)[ 3])); \
    XOReq256(X##bu, LOAD4_64((data3)[ 4], (data2)[ 4], (data1)[ 4], (data0)[ 4])); \
    XOReq256(X##ga, LOAD4_64((data3)[ 5], (data2)[ 5], (data1)[ 5], (data0)[ 5])); \
    XOReq256(X##ge, LOAD4_64((data3)[ 6], (data2)[ 6], (data1)[ 6], (data0)[ 6])); \
    XOReq256(X##gi, LOAD4_64((data3)[ 7], (data2)[ 7], (data1)[ 7], (data0)[ 7])); \
    XOReq256(X##go, LOAD4_64((data3)[ 8], (data2)[ 8], (data1)[ 8], (data0)[ 8])); \
    XOReq256(X##gu, LOAD4_64((data3)[ 9], (data2)[ 9], (data1)[ 9], (data0)[ 9])); \
    XOReq256(X##ka, LOAD4_64((data3)[10], (data2)[10], (data1)[10], (data0)[10])); \
    XOReq256(X##ke, LOAD4_64((data3)[11], (data2)[11], (data1)[11], (data0)[11])); \
    XOReq256(X##ki, LOAD4_64((data3)[12], (data2)[12], (data1)[12], (data0)[12])); \
    XOReq256(X##ko, LOAD4_64((data3)[13], (data2)[13], (data1)[13], (data0)[13])); \
    XOReq256(X##ku, LOAD4_64((data3)[14], (data2)[14], (data1)[14], (data0)[14])); \
    XOReq256(X##ma, LOAD4_64((data3)[15], (data2)[15], (data1)[15], (data0)[15])); \

#define XORdata21(X, data0, data1, data2, data3) \
    XORdata16(X, data0, data1, data2, data3) \
    XOReq256(X##me, LOAD4_64((data3)[16], (data2)[16], (data1)[16], (data0)[16])); \
    XOReq256(X##mi, LOAD4_64((data3)[17], (data2)[17], (data1)[17], (data0)[17])); \
    XOReq256(X##mo, LOAD4_64((data3)[18], (data2)[18], (data1)[18], (data0)[18])); \
    XOReq256(X##mu, LOAD4_64((data3)[19], (data2)[19], (data1)[19], (data0)[19])); \
    XOReq256(X##sa, LOAD4_64((data3)[20], (data2)[20], (data1)[20], (data0)[20])); \

#define rounds12 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(12, A, E) \
    thetaRhoPiChiIotaPrepareTheta(13, E, A) \
    thetaRhoPiChiIotaPrepareTheta(14, A, E) \
    thetaRhoPiChiIotaPrepareTheta(15, E, A) \
    thetaRhoPiChiIotaPrepareTheta(16, A, E) \
    thetaRhoPiChiIotaPrepareTheta(17, E, A) \
    thetaRhoPiChiIotaPrepareTheta(18, A, E) \
    thetaRhoPiChiIotaPrepareTheta(19, E, A) \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A)

#define chunkSize 8192
#define rateInBytes (21*8)

void KangarooTwelve_AVX2_Process4Leaves(const unsigned char *input, unsigned char *output)
{
    declareABCDE
    unsigned int j;

    initializeState(A);

    for(j = 0; j < (chunkSize - rateInBytes); j += rateInBytes) {
        XORdata21(A, (const uint64_t *)input, (const uint64_t *)(input+chunkSize), (const uint64_t *)(input+2*chunkSize), (const uint64_t *)(input+3*chunkSize));
        rounds12
        input += rateInBytes;
    }

    XORdata16(A, (const uint64_t *)input, (const uint64_t *)(input+chunkSize), (const uint64_t *)(input+2*chunkSize), (const uint64_t *)(input+3*chunkSize));
    XOReq256(Ame, CONST256_64(0x0BULL));
    XOReq256(Asa, CONST256_64(0x8000000000000000ULL));
    rounds12

    {
        __m256i lanesL01, lanesL23, lanesH01, lanesH23;

        lanesL01 = UNPACKL( Aba, Abe );
        lanesH01 = UNPACKH( Aba, Abe );
        lanesL23 = UNPACKL( Abi, Abo );
        lanesH23 = UNPACKH( Abi, Abo );
        STORE256u( output[ 0], PERM128( lanesL01, lanesL23, 0x20 ) );
        STORE256u( output[32], PERM128( lanesH01, lanesH23, 0x20 ) );
        STORE256u( output[64], PERM128( lanesL01, lanesL23, 0x31 ) );
        STORE256u( output[96], PERM128( lanesH01, lanesH23, 0x31 ) );
    }
}
