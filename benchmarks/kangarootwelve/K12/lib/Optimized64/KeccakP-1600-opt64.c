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
#include <KeccakP-1600-SnP.h>

#define KeccakP1600_opt64_implementation_config "all rounds unrolled"
#define KeccakP1600_opt64_fullUnrolling
/* Or */
/*
#define KeccakP1600_opt64_implementation_config "6 rounds unrolled"
#define KeccakP1600_opt64_unrolling 6
*/
/* Or */
/*
#define KeccakP1600_opt64_implementation_config "lane complementing, 6 rounds unrolled"
#define KeccakP1600_opt64_unrolling 6
#define KeccakP1600_opt64_useLaneComplementing
*/
/* Or */
/*
#define KeccakP1600_opt64_implementation_config "lane complementing, all rounds unrolled"
#define KeccakP1600_opt64_fullUnrolling
#define KeccakP1600_opt64_useLaneComplementing
*/
/* Or */
/*
#define KeccakP1600_opt64_implementation_config "lane complementing, all rounds unrolled, using SHLD for rotations"
#define KeccakP1600_opt64_fullUnrolling
#define KeccakP1600_opt64_useLaneComplementing
#define KeccakP1600_opt64_useSHLD
*/

#if defined(KeccakP1600_opt64_useLaneComplementing)
#define UseBebigokimisa
#endif

#if defined(_MSC_VER)
#define ROL64(a, offset) _rotl64(a, offset)
#elif defined(KeccakP1600_opt64_useSHLD)
    #define ROL64(x,N) ({ \
    register uint64_t __out; \
    register uint64_t __in = x; \
    __asm__ ("shld %2,%0,%0" : "=r"(__out) : "0"(__in), "i"(N)); \
    __out; \
    })
#else
#define ROL64(a, offset) ((((uint64_t)a) << offset) ^ (((uint64_t)a) >> (64-offset)))
#endif

#ifdef KeccakP1600_opt64_fullUnrolling
#define FullUnrolling
#else
#define Unrolling KeccakP1600_opt64_unrolling
#endif

static const uint64_t KeccakF1600RoundConstants[24] = {
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
    0x8000000080008008ULL };

/* ---------------------------------------------------------------- */

void KeccakP1600_opt64_Initialize(void *state)
{
    memset(state, 0, 200);
#ifdef KeccakP1600_opt64_useLaneComplementing
    ((uint64_t*)state)[ 1] = ~(uint64_t)0;
    ((uint64_t*)state)[ 2] = ~(uint64_t)0;
    ((uint64_t*)state)[ 8] = ~(uint64_t)0;
    ((uint64_t*)state)[12] = ~(uint64_t)0;
    ((uint64_t*)state)[17] = ~(uint64_t)0;
    ((uint64_t*)state)[20] = ~(uint64_t)0;
#endif
}

/* ---------------------------------------------------------------- */

void KeccakP1600_opt64_AddBytesInLane(void *state, unsigned int lanePosition, const unsigned char *data, unsigned int offset, unsigned int length)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    uint64_t lane;
    if (length == 0)
        return;
    if (length == 1)
        lane = data[0];
    else {
        lane = 0;
        memcpy(&lane, data, length);
    }
    lane <<= offset*8;
#else
    uint64_t lane = 0;
    unsigned int i;
    for(i=0; i<length; i++)
        lane |= ((uint64_t)data[i]) << ((i+offset)*8);
#endif
    ((uint64_t*)state)[lanePosition] ^= lane;
}

/* ---------------------------------------------------------------- */

static void KeccakP1600_opt64_AddLanes(void *state, const unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    unsigned int i = 0;
#ifdef NO_MISALIGNED_ACCESSES
    /* If either pointer is misaligned, fall back to byte-wise xor. */
    if (((((uintptr_t)state) & 7) != 0) || ((((uintptr_t)data) & 7) != 0)) {
      for (i = 0; i < laneCount * 8; i++) {
        ((unsigned char*)state)[i] ^= data[i];
      }
    }
    else
#endif
    {
      /* Otherwise... */
      for( ; (i+8)<=laneCount; i+=8) {
          ((uint64_t*)state)[i+0] ^= ((uint64_t*)data)[i+0];
          ((uint64_t*)state)[i+1] ^= ((uint64_t*)data)[i+1];
          ((uint64_t*)state)[i+2] ^= ((uint64_t*)data)[i+2];
          ((uint64_t*)state)[i+3] ^= ((uint64_t*)data)[i+3];
          ((uint64_t*)state)[i+4] ^= ((uint64_t*)data)[i+4];
          ((uint64_t*)state)[i+5] ^= ((uint64_t*)data)[i+5];
          ((uint64_t*)state)[i+6] ^= ((uint64_t*)data)[i+6];
          ((uint64_t*)state)[i+7] ^= ((uint64_t*)data)[i+7];
      }
      for( ; (i+4)<=laneCount; i+=4) {
          ((uint64_t*)state)[i+0] ^= ((uint64_t*)data)[i+0];
          ((uint64_t*)state)[i+1] ^= ((uint64_t*)data)[i+1];
          ((uint64_t*)state)[i+2] ^= ((uint64_t*)data)[i+2];
          ((uint64_t*)state)[i+3] ^= ((uint64_t*)data)[i+3];
      }
      for( ; (i+2)<=laneCount; i+=2) {
          ((uint64_t*)state)[i+0] ^= ((uint64_t*)data)[i+0];
          ((uint64_t*)state)[i+1] ^= ((uint64_t*)data)[i+1];
      }
      if (i<laneCount) {
          ((uint64_t*)state)[i+0] ^= ((uint64_t*)data)[i+0];
      }
    }
#else
    unsigned int i;
    const uint8_t *curData = data;
    for(i=0; i<laneCount; i++, curData+=8) {
        uint64_t lane = (uint64_t)curData[0]
            | ((uint64_t)curData[1] <<  8)
            | ((uint64_t)curData[2] << 16)
            | ((uint64_t)curData[3] << 24)
            | ((uint64_t)curData[4] << 32)
            | ((uint64_t)curData[5] << 40)
            | ((uint64_t)curData[6] << 48)
            | ((uint64_t)curData[7] << 56);
        ((uint64_t*)state)[i] ^= lane;
    }
#endif
}

/* ---------------------------------------------------------------- */

void KeccakP1600_opt64_AddByte(void *state, unsigned char byte, unsigned int offset)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    ((unsigned char*)(state))[offset] ^= byte;
#else
    uint64_t lane = byte;
    lane <<= (offset%8)*8;
    ((uint64_t*)state)[offset/8] ^= lane;
#endif
}

/* ---------------------------------------------------------------- */

#define SnP_AddBytes(state, data, offset, length, SnP_AddLanes, SnP_AddBytesInLane, SnP_laneLengthInBytes) \
    { \
        if ((offset) == 0) { \
            SnP_AddLanes(state, data, (length)/SnP_laneLengthInBytes); \
            SnP_AddBytesInLane(state, \
                (length)/SnP_laneLengthInBytes, \
                (data)+((length)/SnP_laneLengthInBytes)*SnP_laneLengthInBytes, \
                0, \
                (length)%SnP_laneLengthInBytes); \
        } \
        else { \
            unsigned int _sizeLeft = (length); \
            unsigned int _lanePosition = (offset)/SnP_laneLengthInBytes; \
            unsigned int _offsetInLane = (offset)%SnP_laneLengthInBytes; \
            const unsigned char *_curData = (data); \
            while(_sizeLeft > 0) { \
                unsigned int _bytesInLane = SnP_laneLengthInBytes - _offsetInLane; \
                if (_bytesInLane > _sizeLeft) \
                    _bytesInLane = _sizeLeft; \
                SnP_AddBytesInLane(state, _lanePosition, _curData, _offsetInLane, _bytesInLane); \
                _sizeLeft -= _bytesInLane; \
                _lanePosition++; \
                _offsetInLane = 0; \
                _curData += _bytesInLane; \
            } \
        } \
    }

void KeccakP1600_opt64_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    SnP_AddBytes(state, data, offset, length, KeccakP1600_opt64_AddLanes, KeccakP1600_opt64_AddBytesInLane, 8);
}

/* ---------------------------------------------------------------- */

#define declareABCDE \
    uint64_t Aba, Abe, Abi, Abo, Abu; \
    uint64_t Aga, Age, Agi, Ago, Agu; \
    uint64_t Aka, Ake, Aki, Ako, Aku; \
    uint64_t Ama, Ame, Ami, Amo, Amu; \
    uint64_t Asa, Ase, Asi, Aso, Asu; \
    uint64_t Bba, Bbe, Bbi, Bbo, Bbu; \
    uint64_t Bga, Bge, Bgi, Bgo, Bgu; \
    uint64_t Bka, Bke, Bki, Bko, Bku; \
    uint64_t Bma, Bme, Bmi, Bmo, Bmu; \
    uint64_t Bsa, Bse, Bsi, Bso, Bsu; \
    uint64_t Ca, Ce, Ci, Co, Cu; \
    uint64_t Da, De, Di, Do, Du; \
    uint64_t Eba, Ebe, Ebi, Ebo, Ebu; \
    uint64_t Ega, Ege, Egi, Ego, Egu; \
    uint64_t Eka, Eke, Eki, Eko, Eku; \
    uint64_t Ema, Eme, Emi, Emo, Emu; \
    uint64_t Esa, Ese, Esi, Eso, Esu; \

#define prepareTheta \
    Ca = Aba^Aga^Aka^Ama^Asa; \
    Ce = Abe^Age^Ake^Ame^Ase; \
    Ci = Abi^Agi^Aki^Ami^Asi; \
    Co = Abo^Ago^Ako^Amo^Aso; \
    Cu = Abu^Agu^Aku^Amu^Asu; \

#ifdef UseBebigokimisa
/* --- Code for round, with prepare-theta (lane complementing pattern 'bebigokimisa') */
/* --- 64-bit lanes mapped to 64-bit words */
#define thetaRhoPiChiIotaPrepareTheta(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^(  Bbe |  Bbi ); \
    E##ba ^= KeccakF1600RoundConstants[i]; \
    Ca = E##ba; \
    E##be =   Bbe ^((~Bbi)|  Bbo ); \
    Ce = E##be; \
    E##bi =   Bbi ^(  Bbo &  Bbu ); \
    Ci = E##bi; \
    E##bo =   Bbo ^(  Bbu |  Bba ); \
    Co = E##bo; \
    E##bu =   Bbu ^(  Bba &  Bbe ); \
    Cu = E##bu; \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^(  Bge |  Bgi ); \
    Ca ^= E##ga; \
    E##ge =   Bge ^(  Bgi &  Bgo ); \
    Ce ^= E##ge; \
    E##gi =   Bgi ^(  Bgo |(~Bgu)); \
    Ci ^= E##gi; \
    E##go =   Bgo ^(  Bgu |  Bga ); \
    Co ^= E##go; \
    E##gu =   Bgu ^(  Bga &  Bge ); \
    Cu ^= E##gu; \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^(  Bke |  Bki ); \
    Ca ^= E##ka; \
    E##ke =   Bke ^(  Bki &  Bko ); \
    Ce ^= E##ke; \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    Ci ^= E##ki; \
    E##ko = (~Bko)^(  Bku |  Bka ); \
    Co ^= E##ko; \
    E##ku =   Bku ^(  Bka &  Bke ); \
    Cu ^= E##ku; \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^(  Bme &  Bmi ); \
    Ca ^= E##ma; \
    E##me =   Bme ^(  Bmi |  Bmo ); \
    Ce ^= E##me; \
    E##mi =   Bmi ^((~Bmo)|  Bmu ); \
    Ci ^= E##mi; \
    E##mo = (~Bmo)^(  Bmu &  Bma ); \
    Co ^= E##mo; \
    E##mu =   Bmu ^(  Bma |  Bme ); \
    Cu ^= E##mu; \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    Ca ^= E##sa; \
    E##se = (~Bse)^(  Bsi |  Bso ); \
    Ce ^= E##se; \
    E##si =   Bsi ^(  Bso &  Bsu ); \
    Ci ^= E##si; \
    E##so =   Bso ^(  Bsu |  Bsa ); \
    Co ^= E##so; \
    E##su =   Bsu ^(  Bsa &  Bse ); \
    Cu ^= E##su; \
\

/* --- Code for round (lane complementing pattern 'bebigokimisa') */
/* --- 64-bit lanes mapped to 64-bit words */
#define thetaRhoPiChiIota(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^(  Bbe |  Bbi ); \
    E##ba ^= KeccakF1600RoundConstants[i]; \
    E##be =   Bbe ^((~Bbi)|  Bbo ); \
    E##bi =   Bbi ^(  Bbo &  Bbu ); \
    E##bo =   Bbo ^(  Bbu |  Bba ); \
    E##bu =   Bbu ^(  Bba &  Bbe ); \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^(  Bge |  Bgi ); \
    E##ge =   Bge ^(  Bgi &  Bgo ); \
    E##gi =   Bgi ^(  Bgo |(~Bgu)); \
    E##go =   Bgo ^(  Bgu |  Bga ); \
    E##gu =   Bgu ^(  Bga &  Bge ); \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^(  Bke |  Bki ); \
    E##ke =   Bke ^(  Bki &  Bko ); \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    E##ko = (~Bko)^(  Bku |  Bka ); \
    E##ku =   Bku ^(  Bka &  Bke ); \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^(  Bme &  Bmi ); \
    E##me =   Bme ^(  Bmi |  Bmo ); \
    E##mi =   Bmi ^((~Bmo)|  Bmu ); \
    E##mo = (~Bmo)^(  Bmu &  Bma ); \
    E##mu =   Bmu ^(  Bma |  Bme ); \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    E##se = (~Bse)^(  Bsi |  Bso ); \
    E##si =   Bsi ^(  Bso &  Bsu ); \
    E##so =   Bso ^(  Bsu |  Bsa ); \
    E##su =   Bsu ^(  Bsa &  Bse ); \
\

#else /* UseBebigokimisa */
/* --- Code for round, with prepare-theta */
/* --- 64-bit lanes mapped to 64-bit words */
#define thetaRhoPiChiIotaPrepareTheta(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^((~Bbe)&  Bbi ); \
    E##ba ^= KeccakF1600RoundConstants[i]; \
    Ca = E##ba; \
    E##be =   Bbe ^((~Bbi)&  Bbo ); \
    Ce = E##be; \
    E##bi =   Bbi ^((~Bbo)&  Bbu ); \
    Ci = E##bi; \
    E##bo =   Bbo ^((~Bbu)&  Bba ); \
    Co = E##bo; \
    E##bu =   Bbu ^((~Bba)&  Bbe ); \
    Cu = E##bu; \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^((~Bge)&  Bgi ); \
    Ca ^= E##ga; \
    E##ge =   Bge ^((~Bgi)&  Bgo ); \
    Ce ^= E##ge; \
    E##gi =   Bgi ^((~Bgo)&  Bgu ); \
    Ci ^= E##gi; \
    E##go =   Bgo ^((~Bgu)&  Bga ); \
    Co ^= E##go; \
    E##gu =   Bgu ^((~Bga)&  Bge ); \
    Cu ^= E##gu; \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^((~Bke)&  Bki ); \
    Ca ^= E##ka; \
    E##ke =   Bke ^((~Bki)&  Bko ); \
    Ce ^= E##ke; \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    Ci ^= E##ki; \
    E##ko =   Bko ^((~Bku)&  Bka ); \
    Co ^= E##ko; \
    E##ku =   Bku ^((~Bka)&  Bke ); \
    Cu ^= E##ku; \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^((~Bme)&  Bmi ); \
    Ca ^= E##ma; \
    E##me =   Bme ^((~Bmi)&  Bmo ); \
    Ce ^= E##me; \
    E##mi =   Bmi ^((~Bmo)&  Bmu ); \
    Ci ^= E##mi; \
    E##mo =   Bmo ^((~Bmu)&  Bma ); \
    Co ^= E##mo; \
    E##mu =   Bmu ^((~Bma)&  Bme ); \
    Cu ^= E##mu; \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    Ca ^= E##sa; \
    E##se =   Bse ^((~Bsi)&  Bso ); \
    Ce ^= E##se; \
    E##si =   Bsi ^((~Bso)&  Bsu ); \
    Ci ^= E##si; \
    E##so =   Bso ^((~Bsu)&  Bsa ); \
    Co ^= E##so; \
    E##su =   Bsu ^((~Bsa)&  Bse ); \
    Cu ^= E##su; \
\

/* --- Code for round */
/* --- 64-bit lanes mapped to 64-bit words */
#define thetaRhoPiChiIota(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^((~Bbe)&  Bbi ); \
    E##ba ^= KeccakF1600RoundConstants[i]; \
    E##be =   Bbe ^((~Bbi)&  Bbo ); \
    E##bi =   Bbi ^((~Bbo)&  Bbu ); \
    E##bo =   Bbo ^((~Bbu)&  Bba ); \
    E##bu =   Bbu ^((~Bba)&  Bbe ); \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^((~Bge)&  Bgi ); \
    E##ge =   Bge ^((~Bgi)&  Bgo ); \
    E##gi =   Bgi ^((~Bgo)&  Bgu ); \
    E##go =   Bgo ^((~Bgu)&  Bga ); \
    E##gu =   Bgu ^((~Bga)&  Bge ); \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^((~Bke)&  Bki ); \
    E##ke =   Bke ^((~Bki)&  Bko ); \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    E##ko =   Bko ^((~Bku)&  Bka ); \
    E##ku =   Bku ^((~Bka)&  Bke ); \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^((~Bme)&  Bmi ); \
    E##me =   Bme ^((~Bmi)&  Bmo ); \
    E##mi =   Bmi ^((~Bmo)&  Bmu ); \
    E##mo =   Bmo ^((~Bmu)&  Bma ); \
    E##mu =   Bmu ^((~Bma)&  Bme ); \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    E##se =   Bse ^((~Bsi)&  Bso ); \
    E##si =   Bsi ^((~Bso)&  Bsu ); \
    E##so =   Bso ^((~Bsu)&  Bsa ); \
    E##su =   Bsu ^((~Bsa)&  Bse ); \
\

#endif /* UseBebigokimisa */

#define copyFromState(X, state) \
    X##ba = state[ 0]; \
    X##be = state[ 1]; \
    X##bi = state[ 2]; \
    X##bo = state[ 3]; \
    X##bu = state[ 4]; \
    X##ga = state[ 5]; \
    X##ge = state[ 6]; \
    X##gi = state[ 7]; \
    X##go = state[ 8]; \
    X##gu = state[ 9]; \
    X##ka = state[10]; \
    X##ke = state[11]; \
    X##ki = state[12]; \
    X##ko = state[13]; \
    X##ku = state[14]; \
    X##ma = state[15]; \
    X##me = state[16]; \
    X##mi = state[17]; \
    X##mo = state[18]; \
    X##mu = state[19]; \
    X##sa = state[20]; \
    X##se = state[21]; \
    X##si = state[22]; \
    X##so = state[23]; \
    X##su = state[24]; \

#define copyToState(state, X) \
    state[ 0] = X##ba; \
    state[ 1] = X##be; \
    state[ 2] = X##bi; \
    state[ 3] = X##bo; \
    state[ 4] = X##bu; \
    state[ 5] = X##ga; \
    state[ 6] = X##ge; \
    state[ 7] = X##gi; \
    state[ 8] = X##go; \
    state[ 9] = X##gu; \
    state[10] = X##ka; \
    state[11] = X##ke; \
    state[12] = X##ki; \
    state[13] = X##ko; \
    state[14] = X##ku; \
    state[15] = X##ma; \
    state[16] = X##me; \
    state[17] = X##mi; \
    state[18] = X##mo; \
    state[19] = X##mu; \
    state[20] = X##sa; \
    state[21] = X##se; \
    state[22] = X##si; \
    state[23] = X##so; \
    state[24] = X##su; \

#define copyStateVariables(X, Y) \
    X##ba = Y##ba; \
    X##be = Y##be; \
    X##bi = Y##bi; \
    X##bo = Y##bo; \
    X##bu = Y##bu; \
    X##ga = Y##ga; \
    X##ge = Y##ge; \
    X##gi = Y##gi; \
    X##go = Y##go; \
    X##gu = Y##gu; \
    X##ka = Y##ka; \
    X##ke = Y##ke; \
    X##ki = Y##ki; \
    X##ko = Y##ko; \
    X##ku = Y##ku; \
    X##ma = Y##ma; \
    X##me = Y##me; \
    X##mi = Y##mi; \
    X##mo = Y##mo; \
    X##mu = Y##mu; \
    X##sa = Y##sa; \
    X##se = Y##se; \
    X##si = Y##si; \
    X##so = Y##so; \
    X##su = Y##su; \

#if ((defined(FullUnrolling)) || (Unrolling == 12))
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

#elif (Unrolling == 6)
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

#elif (Unrolling == 4)
#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=4) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
    } \

#elif (Unrolling == 3)
#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=3) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        copyStateVariables(A, E) \
    } \

#elif (Unrolling == 2)
#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \

#elif (Unrolling == 1)
#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i++) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        copyStateVariables(A, E) \
    } \

#else
#error "Unrolling is not correctly specified!"
#endif

void KeccakP1600_opt64_Permute_12rounds(void *state)
{
    declareABCDE
    #ifndef KeccakP1600_opt64_fullUnrolling
    unsigned int i;
    #endif
    uint64_t *stateAsLanes = (uint64_t*)state;

    copyFromState(A, stateAsLanes)
    rounds12
    copyToState(stateAsLanes, A)
}

/* ---------------------------------------------------------------- */

void KeccakP1600_opt64_ExtractBytesInLane(const void *state, unsigned int lanePosition, unsigned char *data, unsigned int offset, unsigned int length)
{
    uint64_t lane = ((uint64_t*)state)[lanePosition];
#ifdef KeccakP1600_opt64_useLaneComplementing
    if ((lanePosition == 1) || (lanePosition == 2) || (lanePosition == 8) || (lanePosition == 12) || (lanePosition == 17) || (lanePosition == 20))
        lane = ~lane;
#endif
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    {
        uint64_t lane1[1];
        lane1[0] = lane;
        memcpy(data, (uint8_t*)lane1+offset, length);
    }
#else
    unsigned int i;
    lane >>= offset*8;
    for(i=0; i<length; i++) {
        data[i] = lane & 0xFF;
        lane >>= 8;
    }
#endif
}

/* ---------------------------------------------------------------- */

#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
static void fromWordToBytes(uint8_t *bytes, const uint64_t word)
{
    unsigned int i;

    for(i=0; i<(64/8); i++)
        bytes[i] = (word >> (8*i)) & 0xFF;
}
#endif

void KeccakP1600_opt64_ExtractLanes(const void *state, unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    memcpy(data, state, laneCount*8);
#else
    unsigned int i;

    for(i=0; i<laneCount; i++)
        fromWordToBytes(data+(i*8), ((const uint64_t*)state)[i]);
#endif
#ifdef KeccakP1600_opt64_useLaneComplementing
    if (laneCount > 1) {
        ((uint64_t*)data)[ 1] = ~((uint64_t*)data)[ 1];
        if (laneCount > 2) {
            ((uint64_t*)data)[ 2] = ~((uint64_t*)data)[ 2];
            if (laneCount > 8) {
                ((uint64_t*)data)[ 8] = ~((uint64_t*)data)[ 8];
                if (laneCount > 12) {
                    ((uint64_t*)data)[12] = ~((uint64_t*)data)[12];
                    if (laneCount > 17) {
                        ((uint64_t*)data)[17] = ~((uint64_t*)data)[17];
                        if (laneCount > 20) {
                            ((uint64_t*)data)[20] = ~((uint64_t*)data)[20];
                        }
                    }
                }
            }
        }
    }
#endif
}

/* ---------------------------------------------------------------- */

#define SnP_ExtractBytes(state, data, offset, length, SnP_ExtractLanes, SnP_ExtractBytesInLane, SnP_laneLengthInBytes) \
    { \
        if ((offset) == 0) { \
            SnP_ExtractLanes(state, data, (length)/SnP_laneLengthInBytes); \
            SnP_ExtractBytesInLane(state, \
                (length)/SnP_laneLengthInBytes, \
                (data)+((length)/SnP_laneLengthInBytes)*SnP_laneLengthInBytes, \
                0, \
                (length)%SnP_laneLengthInBytes); \
        } \
        else { \
            unsigned int _sizeLeft = (length); \
            unsigned int _lanePosition = (offset)/SnP_laneLengthInBytes; \
            unsigned int _offsetInLane = (offset)%SnP_laneLengthInBytes; \
            unsigned char *_curData = (data); \
            while(_sizeLeft > 0) { \
                unsigned int _bytesInLane = SnP_laneLengthInBytes - _offsetInLane; \
                if (_bytesInLane > _sizeLeft) \
                    _bytesInLane = _sizeLeft; \
                SnP_ExtractBytesInLane(state, _lanePosition, _curData, _offsetInLane, _bytesInLane); \
                _sizeLeft -= _bytesInLane; \
                _lanePosition++; \
                _offsetInLane = 0; \
                _curData += _bytesInLane; \
            } \
        } \
    }

void KeccakP1600_opt64_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length)
{
    SnP_ExtractBytes(state, data, offset, length, KeccakP1600_opt64_ExtractLanes, KeccakP1600_opt64_ExtractBytesInLane, 8);
}

/* ---------------------------------------------------------------- */

#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
#define HTOLE64(x) (x)
#else
#define HTOLE64(x) (\
  ((x & 0xff00000000000000ull) >> 56) | \
  ((x & 0x00ff000000000000ull) >> 40) | \
  ((x & 0x0000ff0000000000ull) >> 24) | \
  ((x & 0x000000ff00000000ull) >> 8)  | \
  ((x & 0x00000000ff000000ull) << 8)  | \
  ((x & 0x0000000000ff0000ull) << 24) | \
  ((x & 0x000000000000ff00ull) << 40) | \
  ((x & 0x00000000000000ffull) << 56))
#endif

#define addInput(X, input, laneCount) \
    if (laneCount == 21) { \
        X##ba ^= HTOLE64(input[ 0]); \
        X##be ^= HTOLE64(input[ 1]); \
        X##bi ^= HTOLE64(input[ 2]); \
        X##bo ^= HTOLE64(input[ 3]); \
        X##bu ^= HTOLE64(input[ 4]); \
        X##ga ^= HTOLE64(input[ 5]); \
        X##ge ^= HTOLE64(input[ 6]); \
        X##gi ^= HTOLE64(input[ 7]); \
        X##go ^= HTOLE64(input[ 8]); \
        X##gu ^= HTOLE64(input[ 9]); \
        X##ka ^= HTOLE64(input[10]); \
        X##ke ^= HTOLE64(input[11]); \
        X##ki ^= HTOLE64(input[12]); \
        X##ko ^= HTOLE64(input[13]); \
        X##ku ^= HTOLE64(input[14]); \
        X##ma ^= HTOLE64(input[15]); \
        X##me ^= HTOLE64(input[16]); \
        X##mi ^= HTOLE64(input[17]); \
        X##mo ^= HTOLE64(input[18]); \
        X##mu ^= HTOLE64(input[19]); \
        X##sa ^= HTOLE64(input[20]); \
    } \

#include <assert.h>

size_t KeccakP1600_opt64_12rounds_FastLoop_Absorb(void *state, unsigned int laneCount, const unsigned char *data, size_t dataByteLen)
{
    size_t originalDataByteLen = dataByteLen;
    declareABCDE
    #ifndef KeccakP1600_opt64_fullUnrolling
    unsigned int i;
    #endif
    uint64_t *stateAsLanes = (uint64_t*)state;
    uint64_t *inDataAsLanes = (uint64_t*)data;

    assert(laneCount == 21);

    #define laneCount 21
    copyFromState(A, stateAsLanes)
    while(dataByteLen >= laneCount*8) {
        addInput(A, inDataAsLanes, laneCount)
        rounds12
        inDataAsLanes += laneCount;
        dataByteLen -= laneCount*8;
    }
    #undef laneCount
    copyToState(stateAsLanes, A)
    return originalDataByteLen - dataByteLen;
}
