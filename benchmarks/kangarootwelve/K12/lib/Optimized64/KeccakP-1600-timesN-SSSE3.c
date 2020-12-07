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
#include <tmmintrin.h>
#include "KeccakP-1600-SnP.h"
#include "align.h"

#define KeccakP1600times2_SSSE3_unrolling 2

#define SSSE3alignment 16

#define ANDnu128(a, b)      _mm_andnot_si128(a, b)
#define CONST128(a)         _mm_load_si128((const __m128i *)&(a))
#define LOAD128(a)          _mm_load_si128((const __m128i *)&(a))
#define LOAD6464(a, b)      _mm_set_epi64x(a, b)
#define CONST128_64(a)      _mm_set1_epi64x(a)
#define ROL64in128(a, o)    _mm_or_si128(_mm_slli_epi64(a, o), _mm_srli_epi64(a, 64-(o)))
#define ROL64in128_8(a)     _mm_shuffle_epi8(a, CONST128(rho8))
#define ROL64in128_56(a)    _mm_shuffle_epi8(a, CONST128(rho56))
static const uint64_t rho8[2] = {0x0605040302010007, 0x0E0D0C0B0A09080F};
static const uint64_t rho56[2] = {0x0007060504030201, 0x080F0E0D0C0B0A09};
#define STORE128(a, b)      _mm_store_si128((__m128i *)&(a), b)
#define STORE128u(a, b)     _mm_storeu_si128((__m128i *)&(a), b)
#define XOR128(a, b)        _mm_xor_si128(a, b)
#define XOReq128(a, b)      a = _mm_xor_si128(a, b)
#define UNPACKL( a, b )     _mm_unpacklo_epi64((a), (b))
#define UNPACKH( a, b )     _mm_unpackhi_epi64((a), (b))
#define ZERO()              _mm_setzero_si128()

static ALIGN(SSSE3alignment) const uint64_t KeccakP1600RoundConstants[24] = {
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
    __m128i Aba, Abe, Abi, Abo, Abu; \
    __m128i Aga, Age, Agi, Ago, Agu; \
    __m128i Aka, Ake, Aki, Ako, Aku; \
    __m128i Ama, Ame, Ami, Amo, Amu; \
    __m128i Asa, Ase, Asi, Aso, Asu; \
    __m128i Bba, Bbe, Bbi, Bbo, Bbu; \
    __m128i Bga, Bge, Bgi, Bgo, Bgu; \
    __m128i Bka, Bke, Bki, Bko, Bku; \
    __m128i Bma, Bme, Bmi, Bmo, Bmu; \
    __m128i Bsa, Bse, Bsi, Bso, Bsu; \
    __m128i Ca, Ce, Ci, Co, Cu; \
    __m128i Da, De, Di, Do, Du; \
    __m128i Eba, Ebe, Ebi, Ebo, Ebu; \
    __m128i Ega, Ege, Egi, Ego, Egu; \
    __m128i Eka, Eke, Eki, Eko, Eku; \
    __m128i Ema, Eme, Emi, Emo, Emu; \
    __m128i Esa, Ese, Esi, Eso, Esu; \

#define prepareTheta \
    Ca = XOR128(Aba, XOR128(Aga, XOR128(Aka, XOR128(Ama, Asa)))); \
    Ce = XOR128(Abe, XOR128(Age, XOR128(Ake, XOR128(Ame, Ase)))); \
    Ci = XOR128(Abi, XOR128(Agi, XOR128(Aki, XOR128(Ami, Asi)))); \
    Co = XOR128(Abo, XOR128(Ago, XOR128(Ako, XOR128(Amo, Aso)))); \
    Cu = XOR128(Abu, XOR128(Agu, XOR128(Aku, XOR128(Amu, Asu)))); \

/* --- Theta Rho Pi Chi Iota Prepare-theta */
/* --- 64-bit lanes mapped to 64-bit words */
#define thetaRhoPiChiIotaPrepareTheta(i, A, E) \
    Da = XOR128(Cu, ROL64in128(Ce, 1)); \
    De = XOR128(Ca, ROL64in128(Ci, 1)); \
    Di = XOR128(Ce, ROL64in128(Co, 1)); \
    Do = XOR128(Ci, ROL64in128(Cu, 1)); \
    Du = XOR128(Co, ROL64in128(Ca, 1)); \
\
    XOReq128(A##ba, Da); \
    Bba = A##ba; \
    XOReq128(A##ge, De); \
    Bbe = ROL64in128(A##ge, 44); \
    XOReq128(A##ki, Di); \
    Bbi = ROL64in128(A##ki, 43); \
    E##ba = XOR128(Bba, ANDnu128(Bbe, Bbi)); \
    XOReq128(E##ba, CONST128_64(KeccakP1600RoundConstants[i])); \
    Ca = E##ba; \
    XOReq128(A##mo, Do); \
    Bbo = ROL64in128(A##mo, 21); \
    E##be = XOR128(Bbe, ANDnu128(Bbi, Bbo)); \
    Ce = E##be; \
    XOReq128(A##su, Du); \
    Bbu = ROL64in128(A##su, 14); \
    E##bi = XOR128(Bbi, ANDnu128(Bbo, Bbu)); \
    Ci = E##bi; \
    E##bo = XOR128(Bbo, ANDnu128(Bbu, Bba)); \
    Co = E##bo; \
    E##bu = XOR128(Bbu, ANDnu128(Bba, Bbe)); \
    Cu = E##bu; \
\
    XOReq128(A##bo, Do); \
    Bga = ROL64in128(A##bo, 28); \
    XOReq128(A##gu, Du); \
    Bge = ROL64in128(A##gu, 20); \
    XOReq128(A##ka, Da); \
    Bgi = ROL64in128(A##ka, 3); \
    E##ga = XOR128(Bga, ANDnu128(Bge, Bgi)); \
    XOReq128(Ca, E##ga); \
    XOReq128(A##me, De); \
    Bgo = ROL64in128(A##me, 45); \
    E##ge = XOR128(Bge, ANDnu128(Bgi, Bgo)); \
    XOReq128(Ce, E##ge); \
    XOReq128(A##si, Di); \
    Bgu = ROL64in128(A##si, 61); \
    E##gi = XOR128(Bgi, ANDnu128(Bgo, Bgu)); \
    XOReq128(Ci, E##gi); \
    E##go = XOR128(Bgo, ANDnu128(Bgu, Bga)); \
    XOReq128(Co, E##go); \
    E##gu = XOR128(Bgu, ANDnu128(Bga, Bge)); \
    XOReq128(Cu, E##gu); \
\
    XOReq128(A##be, De); \
    Bka = ROL64in128(A##be, 1); \
    XOReq128(A##gi, Di); \
    Bke = ROL64in128(A##gi, 6); \
    XOReq128(A##ko, Do); \
    Bki = ROL64in128(A##ko, 25); \
    E##ka = XOR128(Bka, ANDnu128(Bke, Bki)); \
    XOReq128(Ca, E##ka); \
    XOReq128(A##mu, Du); \
    Bko = ROL64in128_8(A##mu); \
    E##ke = XOR128(Bke, ANDnu128(Bki, Bko)); \
    XOReq128(Ce, E##ke); \
    XOReq128(A##sa, Da); \
    Bku = ROL64in128(A##sa, 18); \
    E##ki = XOR128(Bki, ANDnu128(Bko, Bku)); \
    XOReq128(Ci, E##ki); \
    E##ko = XOR128(Bko, ANDnu128(Bku, Bka)); \
    XOReq128(Co, E##ko); \
    E##ku = XOR128(Bku, ANDnu128(Bka, Bke)); \
    XOReq128(Cu, E##ku); \
\
    XOReq128(A##bu, Du); \
    Bma = ROL64in128(A##bu, 27); \
    XOReq128(A##ga, Da); \
    Bme = ROL64in128(A##ga, 36); \
    XOReq128(A##ke, De); \
    Bmi = ROL64in128(A##ke, 10); \
    E##ma = XOR128(Bma, ANDnu128(Bme, Bmi)); \
    XOReq128(Ca, E##ma); \
    XOReq128(A##mi, Di); \
    Bmo = ROL64in128(A##mi, 15); \
    E##me = XOR128(Bme, ANDnu128(Bmi, Bmo)); \
    XOReq128(Ce, E##me); \
    XOReq128(A##so, Do); \
    Bmu = ROL64in128_56(A##so); \
    E##mi = XOR128(Bmi, ANDnu128(Bmo, Bmu)); \
    XOReq128(Ci, E##mi); \
    E##mo = XOR128(Bmo, ANDnu128(Bmu, Bma)); \
    XOReq128(Co, E##mo); \
    E##mu = XOR128(Bmu, ANDnu128(Bma, Bme)); \
    XOReq128(Cu, E##mu); \
\
    XOReq128(A##bi, Di); \
    Bsa = ROL64in128(A##bi, 62); \
    XOReq128(A##go, Do); \
    Bse = ROL64in128(A##go, 55); \
    XOReq128(A##ku, Du); \
    Bsi = ROL64in128(A##ku, 39); \
    E##sa = XOR128(Bsa, ANDnu128(Bse, Bsi)); \
    XOReq128(Ca, E##sa); \
    XOReq128(A##ma, Da); \
    Bso = ROL64in128(A##ma, 41); \
    E##se = XOR128(Bse, ANDnu128(Bsi, Bso)); \
    XOReq128(Ce, E##se); \
    XOReq128(A##se, De); \
    Bsu = ROL64in128(A##se, 2); \
    E##si = XOR128(Bsi, ANDnu128(Bso, Bsu)); \
    XOReq128(Ci, E##si); \
    E##so = XOR128(Bso, ANDnu128(Bsu, Bsa)); \
    XOReq128(Co, E##so); \
    E##su = XOR128(Bsu, ANDnu128(Bsa, Bse)); \
    XOReq128(Cu, E##su); \
\

/* --- Theta Rho Pi Chi Iota */
/* --- 64-bit lanes mapped to 64-bit words */
#define thetaRhoPiChiIota(i, A, E) \
    Da = XOR128(Cu, ROL64in128(Ce, 1)); \
    De = XOR128(Ca, ROL64in128(Ci, 1)); \
    Di = XOR128(Ce, ROL64in128(Co, 1)); \
    Do = XOR128(Ci, ROL64in128(Cu, 1)); \
    Du = XOR128(Co, ROL64in128(Ca, 1)); \
\
    XOReq128(A##ba, Da); \
    Bba = A##ba; \
    XOReq128(A##ge, De); \
    Bbe = ROL64in128(A##ge, 44); \
    XOReq128(A##ki, Di); \
    Bbi = ROL64in128(A##ki, 43); \
    E##ba = XOR128(Bba, ANDnu128(Bbe, Bbi)); \
    XOReq128(E##ba, CONST128_64(KeccakP1600RoundConstants[i])); \
    XOReq128(A##mo, Do); \
    Bbo = ROL64in128(A##mo, 21); \
    E##be = XOR128(Bbe, ANDnu128(Bbi, Bbo)); \
    XOReq128(A##su, Du); \
    Bbu = ROL64in128(A##su, 14); \
    E##bi = XOR128(Bbi, ANDnu128(Bbo, Bbu)); \
    E##bo = XOR128(Bbo, ANDnu128(Bbu, Bba)); \
    E##bu = XOR128(Bbu, ANDnu128(Bba, Bbe)); \
\
    XOReq128(A##bo, Do); \
    Bga = ROL64in128(A##bo, 28); \
    XOReq128(A##gu, Du); \
    Bge = ROL64in128(A##gu, 20); \
    XOReq128(A##ka, Da); \
    Bgi = ROL64in128(A##ka, 3); \
    E##ga = XOR128(Bga, ANDnu128(Bge, Bgi)); \
    XOReq128(A##me, De); \
    Bgo = ROL64in128(A##me, 45); \
    E##ge = XOR128(Bge, ANDnu128(Bgi, Bgo)); \
    XOReq128(A##si, Di); \
    Bgu = ROL64in128(A##si, 61); \
    E##gi = XOR128(Bgi, ANDnu128(Bgo, Bgu)); \
    E##go = XOR128(Bgo, ANDnu128(Bgu, Bga)); \
    E##gu = XOR128(Bgu, ANDnu128(Bga, Bge)); \
\
    XOReq128(A##be, De); \
    Bka = ROL64in128(A##be, 1); \
    XOReq128(A##gi, Di); \
    Bke = ROL64in128(A##gi, 6); \
    XOReq128(A##ko, Do); \
    Bki = ROL64in128(A##ko, 25); \
    E##ka = XOR128(Bka, ANDnu128(Bke, Bki)); \
    XOReq128(A##mu, Du); \
    Bko = ROL64in128_8(A##mu); \
    E##ke = XOR128(Bke, ANDnu128(Bki, Bko)); \
    XOReq128(A##sa, Da); \
    Bku = ROL64in128(A##sa, 18); \
    E##ki = XOR128(Bki, ANDnu128(Bko, Bku)); \
    E##ko = XOR128(Bko, ANDnu128(Bku, Bka)); \
    E##ku = XOR128(Bku, ANDnu128(Bka, Bke)); \
\
    XOReq128(A##bu, Du); \
    Bma = ROL64in128(A##bu, 27); \
    XOReq128(A##ga, Da); \
    Bme = ROL64in128(A##ga, 36); \
    XOReq128(A##ke, De); \
    Bmi = ROL64in128(A##ke, 10); \
    E##ma = XOR128(Bma, ANDnu128(Bme, Bmi)); \
    XOReq128(A##mi, Di); \
    Bmo = ROL64in128(A##mi, 15); \
    E##me = XOR128(Bme, ANDnu128(Bmi, Bmo)); \
    XOReq128(A##so, Do); \
    Bmu = ROL64in128_56(A##so); \
    E##mi = XOR128(Bmi, ANDnu128(Bmo, Bmu)); \
    E##mo = XOR128(Bmo, ANDnu128(Bmu, Bma)); \
    E##mu = XOR128(Bmu, ANDnu128(Bma, Bme)); \
\
    XOReq128(A##bi, Di); \
    Bsa = ROL64in128(A##bi, 62); \
    XOReq128(A##go, Do); \
    Bse = ROL64in128(A##go, 55); \
    XOReq128(A##ku, Du); \
    Bsi = ROL64in128(A##ku, 39); \
    E##sa = XOR128(Bsa, ANDnu128(Bse, Bsi)); \
    XOReq128(A##ma, Da); \
    Bso = ROL64in128(A##ma, 41); \
    E##se = XOR128(Bse, ANDnu128(Bsi, Bso)); \
    XOReq128(A##se, De); \
    Bsu = ROL64in128(A##se, 2); \
    E##si = XOR128(Bsi, ANDnu128(Bso, Bsu)); \
    E##so = XOR128(Bso, ANDnu128(Bsu, Bsa)); \
    E##su = XOR128(Bsu, ANDnu128(Bsa, Bse)); \
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

#define XORdata16(X, data0, data1) \
    XOReq128(X##ba, LOAD6464((data1)[ 0], (data0)[ 0])); \
    XOReq128(X##be, LOAD6464((data1)[ 1], (data0)[ 1])); \
    XOReq128(X##bi, LOAD6464((data1)[ 2], (data0)[ 2])); \
    XOReq128(X##bo, LOAD6464((data1)[ 3], (data0)[ 3])); \
    XOReq128(X##bu, LOAD6464((data1)[ 4], (data0)[ 4])); \
    XOReq128(X##ga, LOAD6464((data1)[ 5], (data0)[ 5])); \
    XOReq128(X##ge, LOAD6464((data1)[ 6], (data0)[ 6])); \
    XOReq128(X##gi, LOAD6464((data1)[ 7], (data0)[ 7])); \
    XOReq128(X##go, LOAD6464((data1)[ 8], (data0)[ 8])); \
    XOReq128(X##gu, LOAD6464((data1)[ 9], (data0)[ 9])); \
    XOReq128(X##ka, LOAD6464((data1)[10], (data0)[10])); \
    XOReq128(X##ke, LOAD6464((data1)[11], (data0)[11])); \
    XOReq128(X##ki, LOAD6464((data1)[12], (data0)[12])); \
    XOReq128(X##ko, LOAD6464((data1)[13], (data0)[13])); \
    XOReq128(X##ku, LOAD6464((data1)[14], (data0)[14])); \
    XOReq128(X##ma, LOAD6464((data1)[15], (data0)[15])); \

#define XORdata21(X, data0, data1) \
    XORdata16(X, data0, data1) \
    XOReq128(X##me, LOAD6464((data1)[16], (data0)[16])); \
    XOReq128(X##mi, LOAD6464((data1)[17], (data0)[17])); \
    XOReq128(X##mo, LOAD6464((data1)[18], (data0)[18])); \
    XOReq128(X##mu, LOAD6464((data1)[19], (data0)[19])); \
    XOReq128(X##sa, LOAD6464((data1)[20], (data0)[20])); \

#if ((defined(KeccakP1600times2_SSSE3_fullUnrolling)) || (KeccakP1600times2_SSSE3_unrolling == 12))
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
    thetaRhoPiChiIota(23, E, A) \

#elif (KeccakP1600times2_SSSE3_unrolling == 6)
#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=6) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+5, E, A) \
    } \

#elif (KeccakP1600times2_SSSE3_unrolling == 4)
#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=4) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
    } \

#elif (KeccakP1600times2_SSSE3_unrolling == 2)
#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \

#else
#error "KeccakP1600times2_SSSE3_unrolling is not correctly specified!"
#endif

#define chunkSize 8192
#define rateInBytes (21*8)

void KangarooTwelve_SSSE3_Process2Leaves(const unsigned char *input, unsigned char *output)
{
    declareABCDE
    #ifndef KeccakP1600times2_SSSE3_fullUnrolling
    unsigned int i;
    #endif
    unsigned int j;

    initializeState(A);

    for(j = 0; j < (chunkSize - rateInBytes); j += rateInBytes) {
        XORdata21(A, (const uint64_t *)input, (const uint64_t *)(input+chunkSize));
        rounds12
        input += rateInBytes;
    }

    XORdata16(A, (const uint64_t *)input, (const uint64_t *)(input+chunkSize));
    XOReq128(Ame, _mm_set1_epi64x(0x0BULL));
    XOReq128(Asa, _mm_set1_epi64x(0x8000000000000000ULL));
    rounds12

    STORE128u( *(__m128i*)&(output[ 0]), UNPACKL( Aba, Abe ) );
    STORE128u( *(__m128i*)&(output[16]), UNPACKL( Abi, Abo ) );
    STORE128u( *(__m128i*)&(output[32]), UNPACKH( Aba, Abe ) );
    STORE128u( *(__m128i*)&(output[48]), UNPACKH( Abi, Abo ) );
}
