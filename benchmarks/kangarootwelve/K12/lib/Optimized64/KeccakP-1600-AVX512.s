# Copyright (c) 2006-2017, CRYPTOGAMS by <appro@openssl.org>
# Copyright (c) 2018 Ronny Van Keer
# All rights reserved.
#
# The source code in this file is licensed under the CRYPTOGAMS license.
# For further details see http://www.openssl.org/~appro/cryptogams/.
#
# Notes:
# The code for the permutation (__KeccakF1600) was generated with
# Andy Polyakov's keccak1600-avx512.pl from the CRYPTOGAMS project
# (https://github.com/dot-asm/cryptogams/blob/master/x86_64/keccak1600-avx512.pl).
# The rest of the code was written by Ronny Van Keer.
# Adaptations for macOS by Stéphane Léon.

.text

# -----------------------------------------------------------------------------
#
# void KeccakP1600_AVX512_Initialize(void *state);
#
.ifdef macOS
.globl  _KeccakP1600_AVX512_Initialize
.balign  32
_KeccakP1600_AVX512_Initialize:
.else
.globl  KeccakP1600_AVX512_Initialize
.type   KeccakP1600_AVX512_Initialize,@function
.align  32
KeccakP1600_AVX512_Initialize:
.endif
    vpxorq      %zmm0,%zmm0,%zmm0
    vmovdqa64   %zmm0,0*64(%rdi)
    vmovdqa64   %zmm0,1*64(%rdi)
    vmovdqa64   %zmm0,2*64(%rdi)
    movq        $0,3*64(%rdi)
    ret
.ifdef macOS
.else
.size   KeccakP1600_AVX512_Initialize,.-KeccakP1600_AVX512_Initialize
.endif

# -----------------------------------------------------------------------------
#
# void KeccakP1600_AVX512_AddByte(void *state, unsigned char data, unsigned int offset);
#                                %rdi                 %rsi               %rdx
#!!
#.globl  KeccakP1600_AVX512_AddByte
#.type   KeccakP1600_AVX512_AddByte,@function
#.align  32
#KeccakP1600_AVX512_AddByte:
#    mov         %rdx, %rax
#    and         $7, %rax
#    and         $0xFFFFFFF8, %edx
#    mov         mapState(%rdx), %rdx
#    add         %rdx, %rdi
#    add         %rax, %rdi
#    xorb        %sil, (%rdi)
#    ret
#.size   KeccakP1600_AVX512_AddByte,.-KeccakP1600_AVX512_AddByte

# -----------------------------------------------------------------------------
#
# void KeccakP1600_AVX512_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length);
#                                %rdi                         %rsi               %rdx                 %rcx
#
.ifdef macOS
.globl  _KeccakP1600_AVX512_AddBytes
.balign 32
_KeccakP1600_AVX512_AddBytes:
.else
.globl  KeccakP1600_AVX512_AddBytes
.type   KeccakP1600_AVX512_AddBytes,@function
.align  32
KeccakP1600_AVX512_AddBytes:
.endif
    cmp         $0, %rcx
    jz          KeccakP1600_AVX512_AddBytes_Exit
    add            %rdx, %rdi                                # state += offset
    and         $7, %rdx
    jz          KeccakP1600_AVX512_AddBytes_LaneAlignedCheck
    mov         $8, %r9                                 # r9 is (max) length of incomplete lane
    sub         %rdx, %r9
    cmp         %rcx, %r9
    cmovae      %rcx, %r9
    sub         %r9, %rcx                               # length -= length of incomplete lane
KeccakP1600_AVX512_AddBytes_NotAlignedLoop:
    mov         (%rsi), %r8b
    inc         %rsi
    xorb        %r8b, (%rdi)
    inc         %rdi
    dec         %r9
    jnz         KeccakP1600_AVX512_AddBytes_NotAlignedLoop
    jmp         KeccakP1600_AVX512_AddBytes_LaneAlignedCheck
KeccakP1600_AVX512_AddBytes_LaneAlignedLoop:
    mov         (%rsi), %r8
    add         $8, %rsi
    xor         %r8, (%rdi)
    add         $8, %rdi
KeccakP1600_AVX512_AddBytes_LaneAlignedCheck:
    sub         $8, %rcx
    jnc         KeccakP1600_AVX512_AddBytes_LaneAlignedLoop
KeccakP1600_AVX512_AddBytes_LastIncompleteLane:
    add         $8, %rcx
    jz          KeccakP1600_AVX512_AddBytes_Exit
KeccakP1600_AVX512_AddBytes_LastIncompleteLaneLoop:
    mov         (%rsi), %r8b
    inc         %rsi
    xor         %r8b, (%rdi)
    inc         %rdi
    dec         %rcx
    jnz         KeccakP1600_AVX512_AddBytes_LastIncompleteLaneLoop
KeccakP1600_AVX512_AddBytes_Exit:
    ret
.ifdef macOS
.else
.size   KeccakP1600_AVX512_AddBytes,.-KeccakP1600_AVX512_AddBytes
.endif

# -----------------------------------------------------------------------------
#
# void KeccakP1600_AVX512_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length);
#                                           %rdi                  %rsi               %rdx                 %rcx
#
.ifdef macOS
.globl  _KeccakP1600_AVX512_ExtractBytes
.balign  32
_KeccakP1600_AVX512_ExtractBytes:
.else
.globl  KeccakP1600_AVX512_ExtractBytes
.type   KeccakP1600_AVX512_ExtractBytes,@function
.align  32
KeccakP1600_AVX512_ExtractBytes:
.endif
    cmp         $0, %rcx
    jz          KeccakP1600_AVX512_ExtractBytes_Exit
    add         %rdx, %rdi                              # state += offset
    and         $7, %rdx
    jz          KeccakP1600_AVX512_ExtractBytes_LaneAlignedCheck
    mov         $8, %rax                                # rax is (max) length of incomplete lane
    sub         %rdx, %rax
    cmp         %rcx, %rax
    cmovae      %rcx, %rax
    sub         %rax, %rcx                              # length -= length of incomplete lane
KeccakP1600_AVX512_ExtractBytes_NotAlignedLoop:
    mov         (%rdi), %r8b
    inc         %rdi
    mov         %r8b, (%rsi)
    inc         %rsi
    dec         %rax
    jnz         KeccakP1600_AVX512_ExtractBytes_NotAlignedLoop
    jmp         KeccakP1600_AVX512_ExtractBytes_LaneAlignedCheck
KeccakP1600_AVX512_ExtractBytes_LaneAlignedLoop:
    mov         (%rdi), %r8
    add         $8, %rdi
    mov         %r8, (%rsi)
    add         $8, %rsi
KeccakP1600_AVX512_ExtractBytes_LaneAlignedCheck:
    sub         $8, %rcx
    jnc         KeccakP1600_AVX512_ExtractBytes_LaneAlignedLoop
KeccakP1600_AVX512_ExtractBytes_LastIncompleteLane:
    add         $8, %rcx
    jz          KeccakP1600_AVX512_ExtractBytes_Exit
    mov         (%rdi), %r8
KeccakP1600_AVX512_ExtractBytes_LastIncompleteLaneLoop:
    mov         %r8b, (%rsi)
    shr         $8, %r8
    inc         %rsi
    dec         %rcx
    jnz         KeccakP1600_AVX512_ExtractBytes_LastIncompleteLaneLoop
KeccakP1600_AVX512_ExtractBytes_Exit:
    ret
.ifdef macOS
.else
.size   KeccakP1600_AVX512_ExtractBytes,.-KeccakP1600_AVX512_ExtractBytes
.endif

# -----------------------------------------------------------------------------
#
# internal    
#
.text
.ifdef macOS
.balign  32
.else
.type    __KeccakF1600,@function
.align    32
.endif
__KeccakF1600:
.Loop_avx512:
    ######################################### Theta, even round
    vmovdqa64    %zmm0,%zmm5        # put aside original A00
    vpternlogq    $0x96,%zmm2,%zmm1,%zmm0    # and use it as "C00"
    vpternlogq    $0x96,%zmm4,%zmm3,%zmm0
    vprolq        $1,%zmm0,%zmm6
    vpermq        %zmm0,%zmm13,%zmm0
    vpermq        %zmm6,%zmm16,%zmm6
    vpternlogq    $0x96,%zmm0,%zmm6,%zmm5    # T[0] is original A00
    vpternlogq    $0x96,%zmm0,%zmm6,%zmm1
    vpternlogq    $0x96,%zmm0,%zmm6,%zmm2
    vpternlogq    $0x96,%zmm0,%zmm6,%zmm3
    vpternlogq    $0x96,%zmm0,%zmm6,%zmm4
    ######################################### Rho
    vprolvq        %zmm22,%zmm5,%zmm0    # T[0] is original A00
    vprolvq        %zmm23,%zmm1,%zmm1
    vprolvq        %zmm24,%zmm2,%zmm2
    vprolvq        %zmm25,%zmm3,%zmm3
    vprolvq        %zmm26,%zmm4,%zmm4
    ######################################### Pi
    vpermq        %zmm0,%zmm17,%zmm0
    vpermq        %zmm1,%zmm18,%zmm1
    vpermq        %zmm2,%zmm19,%zmm2
    vpermq        %zmm3,%zmm20,%zmm3
    vpermq        %zmm4,%zmm21,%zmm4
    ######################################### Chi
    vmovdqa64    %zmm0,%zmm5
    vmovdqa64    %zmm1,%zmm6
    vpternlogq    $0xD2,%zmm2,%zmm1,%zmm0
    vpternlogq    $0xD2,%zmm3,%zmm2,%zmm1
    vpternlogq    $0xD2,%zmm4,%zmm3,%zmm2
    vpternlogq    $0xD2,%zmm5,%zmm4,%zmm3
    vpternlogq    $0xD2,%zmm6,%zmm5,%zmm4
    ######################################### Iota
    vpxorq        (%r10),%zmm0,%zmm0{%k1}
    lea        16(%r10),%r10
    ######################################### Harmonize rounds
    vpblendmq    %zmm2,%zmm1,%zmm6{%k2}
    vpblendmq    %zmm3,%zmm2,%zmm7{%k2}
    vpblendmq    %zmm4,%zmm3,%zmm8{%k2}
     vpblendmq    %zmm1,%zmm0,%zmm5{%k2}
    vpblendmq    %zmm0,%zmm4,%zmm9{%k2}
    vpblendmq    %zmm3,%zmm6,%zmm6{%k3}
    vpblendmq    %zmm4,%zmm7,%zmm7{%k3}
     vpblendmq    %zmm2,%zmm5,%zmm5{%k3}
    vpblendmq    %zmm0,%zmm8,%zmm8{%k3}
    vpblendmq    %zmm1,%zmm9,%zmm9{%k3}
    vpblendmq    %zmm4,%zmm6,%zmm6{%k4}
     vpblendmq    %zmm3,%zmm5,%zmm5{%k4}
    vpblendmq    %zmm0,%zmm7,%zmm7{%k4}
    vpblendmq    %zmm1,%zmm8,%zmm8{%k4}
    vpblendmq    %zmm2,%zmm9,%zmm9{%k4}
    vpblendmq    %zmm4,%zmm5,%zmm5{%k5}
    vpblendmq    %zmm0,%zmm6,%zmm6{%k5}
    vpblendmq    %zmm1,%zmm7,%zmm7{%k5}
    vpblendmq    %zmm2,%zmm8,%zmm8{%k5}
    vpblendmq    %zmm3,%zmm9,%zmm9{%k5}
    #vpermq        %zmm5,%zmm33,%zmm0    # doesn't actually change order
    vpermq        %zmm6,%zmm13,%zmm1
    vpermq        %zmm7,%zmm14,%zmm2
    vpermq        %zmm8,%zmm15,%zmm3
    vpermq        %zmm9,%zmm16,%zmm4
    ######################################### Theta, odd round
    vmovdqa64    %zmm5,%zmm0        # real A00
    vpternlogq    $0x96,%zmm2,%zmm1,%zmm5    # C00 is %zmm5's alias
    vpternlogq    $0x96,%zmm4,%zmm3,%zmm5
    vprolq        $1,%zmm5,%zmm6
    vpermq        %zmm5,%zmm13,%zmm5
    vpermq        %zmm6,%zmm16,%zmm6
    vpternlogq    $0x96,%zmm5,%zmm6,%zmm0
    vpternlogq    $0x96,%zmm5,%zmm6,%zmm3
    vpternlogq    $0x96,%zmm5,%zmm6,%zmm1
    vpternlogq    $0x96,%zmm5,%zmm6,%zmm4
    vpternlogq    $0x96,%zmm5,%zmm6,%zmm2
    ######################################### Rho
    vprolvq        %zmm27,%zmm0,%zmm0
    vprolvq        %zmm30,%zmm3,%zmm6
    vprolvq        %zmm28,%zmm1,%zmm7
    vprolvq        %zmm31,%zmm4,%zmm8
    vprolvq        %zmm29,%zmm2,%zmm9
     vpermq        %zmm0,%zmm16,%zmm10
     vpermq        %zmm0,%zmm15,%zmm11
    ######################################### Iota
    vpxorq        -8(%r10),%zmm0,%zmm0{%k1}
    ######################################### Pi
    vpermq        %zmm6,%zmm14,%zmm1
    vpermq        %zmm7,%zmm16,%zmm2
    vpermq        %zmm8,%zmm13,%zmm3
    vpermq        %zmm9,%zmm15,%zmm4
    ######################################### Chi
    vpternlogq    $0xD2,%zmm11,%zmm10,%zmm0
    vpermq        %zmm6,%zmm13,%zmm12
    #vpermq        %zmm6,%zmm33,%zmm6
    vpternlogq    $0xD2,%zmm6,%zmm12,%zmm1
    vpermq        %zmm7,%zmm15,%zmm5
    vpermq        %zmm7,%zmm14,%zmm7
    vpternlogq    $0xD2,%zmm7,%zmm5,%zmm2
    #vpermq        %zmm8,%zmm33,%zmm8
    vpermq        %zmm8,%zmm16,%zmm6
    vpternlogq    $0xD2,%zmm6,%zmm8,%zmm3
    vpermq        %zmm9,%zmm14,%zmm5
    vpermq        %zmm9,%zmm13,%zmm9
    vpternlogq    $0xD2,%zmm9,%zmm5,%zmm4
    dec        %eax
    jnz        .Loop_avx512
    ret
.ifdef macOS
.else
.size    __KeccakF1600,.-__KeccakF1600
.endif

# -----------------------------------------------------------------------------
#
# void KeccakP1600_AVX512_Permute_12rounds(void *state);
#                                        %rdi
#
.ifdef macOS
.globl  _KeccakP1600_AVX512_Permute_12rounds
.balign  32
_KeccakP1600_AVX512_Permute_12rounds:
.else
.globl  KeccakP1600_AVX512_Permute_12rounds
.type   KeccakP1600_AVX512_Permute_12rounds,@function
.align  32
KeccakP1600_AVX512_Permute_12rounds:
.endif
    lea         96(%rdi),%rdi
    lea         theta_perm(%rip),%r8
    kxnorw      %k6,%k6,%k6
    kshiftrw    $15,%k6,%k1
    kshiftrw    $11,%k6,%k6
    kshiftlw    $1,%k1,%k2
    kshiftlw    $2,%k1,%k3
    kshiftlw    $3,%k1,%k4
    kshiftlw    $4,%k1,%k5
    #vmovdqa64   64*0(%r8),%zmm33
    vmovdqa64   64*1(%r8),%zmm13
    vmovdqa64   64*2(%r8),%zmm14
    vmovdqa64   64*3(%r8),%zmm15
    vmovdqa64   64*4(%r8),%zmm16
    vmovdqa64   64*5(%r8),%zmm27
    vmovdqa64   64*6(%r8),%zmm28
    vmovdqa64   64*7(%r8),%zmm29
    vmovdqa64   64*8(%r8),%zmm30
    vmovdqa64   64*9(%r8),%zmm31
    vmovdqa64   64*10(%r8),%zmm22
    vmovdqa64   64*11(%r8),%zmm23
    vmovdqa64   64*12(%r8),%zmm24
    vmovdqa64   64*13(%r8),%zmm25
    vmovdqa64   64*14(%r8),%zmm26
    vmovdqa64   64*15(%r8),%zmm17
    vmovdqa64   64*16(%r8),%zmm18
    vmovdqa64   64*17(%r8),%zmm19
    vmovdqa64   64*18(%r8),%zmm20
    vmovdqa64   64*19(%r8),%zmm21
    vmovdqu64   40*0-96(%rdi),%zmm0{%k6}{z}
#    vpxorq      %zmm5,%zmm5,%zmm5
    vmovdqu64   40*1-96(%rdi),%zmm1{%k6}{z}
    vmovdqu64   40*2-96(%rdi),%zmm2{%k6}{z}
    vmovdqu64   40*3-96(%rdi),%zmm3{%k6}{z}
    vmovdqu64   40*4-96(%rdi),%zmm4{%k6}{z}
    lea         iotas+12*8(%rip), %r10
    mov         $12/2, %eax
    call        __KeccakF1600
    vmovdqu64   %zmm0,40*0-96(%rdi){%k6}
    vmovdqu64   %zmm1,40*1-96(%rdi){%k6}
    vmovdqu64   %zmm2,40*2-96(%rdi){%k6}
    vmovdqu64   %zmm3,40*3-96(%rdi){%k6}
    vmovdqu64   %zmm4,40*4-96(%rdi){%k6}
    vzeroupper
    ret
.ifdef macOS
.else
.size   KeccakP1600_AVX512_Permute_12rounds,.-KeccakP1600_AVX512_Permute_12rounds
.endif

# -----------------------------------------------------------------------------
#
# size_t KeccakP1600_AVX512_12rounds_FastLoop_Absorb(void *state, unsigned int laneCount, const unsigned char *data, size_t dataByteLen);
#                                          %rdi                %rsi                            %rdx         %rcx
#
.ifdef macOS
.globl  _KeccakP1600_AVX512_12rounds_FastLoop_Absorb
.balign  32
_KeccakP1600_AVX512_12rounds_FastLoop_Absorb:
.else
.globl  KeccakP1600_AVX512_12rounds_FastLoop_Absorb
.type   KeccakP1600_AVX512_12rounds_FastLoop_Absorb,@function
.align  32
KeccakP1600_AVX512_12rounds_FastLoop_Absorb:
.endif
    push            %rbx
    push            %r10
    shr             $3, %rcx                # rcx = data length in lanes
    mov             %rdx, %rbx              # rbx = initial data pointer
    cmp             %rsi, %rcx
    jb              KeccakP1600_AVX512_FastLoop_Absorb_Exit
    lea             96(%rdi),%rdi
    lea             theta_perm(%rip),%r8
    kxnorw          %k6,%k6,%k6
    kshiftrw        $15,%k6,%k1
    kshiftrw        $11,%k6,%k6
    kshiftlw        $1,%k1,%k2
    kshiftlw        $2,%k1,%k3
    kshiftlw        $3,%k1,%k4
    kshiftlw        $4,%k1,%k5
    vmovdqa64       64*1(%r8),%zmm13
    vmovdqa64       64*2(%r8),%zmm14
    vmovdqa64       64*3(%r8),%zmm15
    vmovdqa64       64*4(%r8),%zmm16
    vmovdqa64       64*5(%r8),%zmm27
    vmovdqa64       64*6(%r8),%zmm28
    vmovdqa64       64*7(%r8),%zmm29
    vmovdqa64       64*8(%r8),%zmm30
    vmovdqa64       64*9(%r8),%zmm31
    vmovdqa64       64*10(%r8),%zmm22
    vmovdqa64       64*11(%r8),%zmm23
    vmovdqa64       64*12(%r8),%zmm24
    vmovdqa64       64*13(%r8),%zmm25
    vmovdqa64       64*14(%r8),%zmm26
    vmovdqa64       64*15(%r8),%zmm17
    vmovdqa64       64*16(%r8),%zmm18
    vmovdqa64       64*17(%r8),%zmm19
    vmovdqa64       64*18(%r8),%zmm20
    vmovdqa64       64*19(%r8),%zmm21
    vmovdqu64       40*0-96(%rdi),%zmm0{%k6}{z}
    vmovdqu64       40*1-96(%rdi),%zmm1{%k6}{z}
    vmovdqu64       40*2-96(%rdi),%zmm2{%k6}{z}
    vmovdqu64       40*3-96(%rdi),%zmm3{%k6}{z}
    vmovdqu64       40*4-96(%rdi),%zmm4{%k6}{z}
    cmp             $21, %rsi    
    jnz             KeccakP1600_AVX512_FastLoop_Absorb_Not21Lanes
    sub             $21, %rcx
KeccakP1600_AVX512_FastLoop_Absorb_Loop21Lanes:
    vmovdqu64       8*0(%rdx),%zmm5{%k6}{z}
    vmovdqu64       8*5(%rdx),%zmm6{%k6}{z}
    vmovdqu64       8*10(%rdx),%zmm7{%k6}{z}
    vmovdqu64       8*15(%rdx),%zmm8{%k6}{z}
    vmovdqu64       8*20(%rdx),%zmm9{%k1}{z}
    vpxorq          %zmm5,%zmm0,%zmm0
    vpxorq          %zmm6,%zmm1,%zmm1
    vpxorq          %zmm7,%zmm2,%zmm2
    vpxorq          %zmm8,%zmm3,%zmm3
    vpxorq          %zmm9,%zmm4,%zmm4
    add             $21*8, %rdx
    lea             iotas+12*8(%rip), %r10
    mov             $12/2, %eax
    call            __KeccakF1600
    sub             $21, %rcx
    jnc             KeccakP1600_AVX512_FastLoop_Absorb_Loop21Lanes
KeccakP1600_AVX512_FastLoop_Absorb_SaveAndExit:
    vmovdqu64       %zmm0,40*0-96(%rdi){%k6}
    vmovdqu64       %zmm1,40*1-96(%rdi){%k6}
    vmovdqu64       %zmm2,40*2-96(%rdi){%k6}
    vmovdqu64       %zmm3,40*3-96(%rdi){%k6}
    vmovdqu64       %zmm4,40*4-96(%rdi){%k6}
KeccakP1600_AVX512_FastLoop_Absorb_Exit:
    vzeroupper
    mov             %rdx, %rax               # return number of bytes processed
    sub             %rbx, %rax
    pop             %r10
    pop             %rbx
    ret
KeccakP1600_AVX512_FastLoop_Absorb_Not21Lanes:
    cmp             $17, %rsi    
    jnz             KeccakP1600_AVX512_FastLoop_Absorb_Not17Lanes
    sub             $17, %rcx
KeccakP1600_AVX512_FastLoop_Absorb_Loop17Lanes:
    vmovdqu64       8*0(%rdx),%zmm5{%k6}{z}
    vmovdqu64       8*5(%rdx),%zmm6{%k6}{z}
    vmovdqu64       8*10(%rdx),%zmm7{%k6}{z}
    vmovdqu64       8*15(%rdx),%zmm8{%k1}{z}
    vmovdqu64       8*15(%rdx),%zmm8{%k2}
    vpxorq          %zmm5,%zmm0,%zmm0
    vpxorq          %zmm6,%zmm1,%zmm1
    vpxorq          %zmm7,%zmm2,%zmm2
    vpxorq          %zmm8,%zmm3,%zmm3
    add             $17*8, %rdx
    lea             iotas+12*8(%rip), %r10
    mov             $12/2, %eax
    call            __KeccakF1600
    sub             $17, %rcx
    jnc             KeccakP1600_AVX512_FastLoop_Absorb_Loop17Lanes
    jmp             KeccakP1600_AVX512_FastLoop_Absorb_SaveAndExit
KeccakP1600_AVX512_FastLoop_Absorb_Not17Lanes:
    lea             -96(%rdi), %rdi
KeccakP1600_AVX512_FastLoop_Absorb_LanesLoop:
    mov             %rsi, %rax
    mov             %rdi, %r10
KeccakP1600_AVX512_FastLoop_Absorb_LanesAddLoop:
    mov             (%rdx), %r8
    add             $8, %rdx
    xor             %r8, (%r10)
    add             $8, %r10
    sub             $1, %rax
    jnz             KeccakP1600_AVX512_FastLoop_Absorb_LanesAddLoop
    sub             %rsi, %rcx
    push            %rdi
    push            %rsi
    push            %rdx
    push            %rcx
.ifdef macOS
    call            _KeccakP1600_AVX512_Permute_12rounds
.else
    call            KeccakP1600_AVX512_Permute_12rounds@PLT
.endif
    pop             %rcx
    pop             %rdx
    pop             %rsi
    pop             %rdi
    cmp             %rsi, %rcx
    jae             KeccakP1600_AVX512_FastLoop_Absorb_LanesLoop
    jmp             KeccakP1600_AVX512_FastLoop_Absorb_Exit
.ifdef macOS
.balign  64
.else
.size   KeccakP1600_AVX512_12rounds_FastLoop_Absorb,.-KeccakP1600_AVX512_12rounds_FastLoop_Absorb

.align    64
.endif
theta_perm:
    .quad    0, 1, 2, 3, 4, 5, 6, 7        # [not used]
    .quad    4, 0, 1, 2, 3, 5, 6, 7
    .quad    3, 4, 0, 1, 2, 5, 6, 7
    .quad    2, 3, 4, 0, 1, 5, 6, 7
    .quad    1, 2, 3, 4, 0, 5, 6, 7
rhotates1:
    .quad    0,  44, 43, 21, 14, 0, 0, 0    # [0][0] [1][1] [2][2] [3][3] [4][4]
    .quad    18, 1,  6,  25, 8,  0, 0, 0    # [4][0] [0][1] [1][2] [2][3] [3][4]
    .quad    41, 2,  62, 55, 39, 0, 0, 0    # [3][0] [4][1] [0][2] [1][3] [2][4]
    .quad    3,  45, 61, 28, 20, 0, 0, 0    # [2][0] [3][1] [4][2] [0][3] [1][4]
    .quad    36, 10, 15, 56, 27, 0, 0, 0    # [1][0] [2][1] [3][2] [4][3] [0][4]
rhotates0:
    .quad     0,  1, 62, 28, 27, 0, 0, 0
    .quad    36, 44,  6, 55, 20, 0, 0, 0
    .quad     3, 10, 43, 25, 39, 0, 0, 0
    .quad    41, 45, 15, 21,  8, 0, 0, 0
    .quad    18,  2, 61, 56, 14, 0, 0, 0
pi0_perm:
    .quad    0, 3, 1, 4, 2, 5, 6, 7
    .quad    1, 4, 2, 0, 3, 5, 6, 7
    .quad    2, 0, 3, 1, 4, 5, 6, 7
    .quad    3, 1, 4, 2, 0, 5, 6, 7
    .quad    4, 2, 0, 3, 1, 5, 6, 7
iotas:
    .quad    0x0000000000000001
    .quad    0x0000000000008082
    .quad    0x800000000000808a
    .quad    0x8000000080008000
    .quad    0x000000000000808b
    .quad    0x0000000080000001
    .quad    0x8000000080008081
    .quad    0x8000000000008009
    .quad    0x000000000000008a
    .quad    0x0000000000000088
    .quad    0x0000000080008009
    .quad    0x000000008000000a
    .quad    0x000000008000808b
    .quad    0x800000000000008b
    .quad    0x8000000000008089
    .quad    0x8000000000008003
    .quad    0x8000000000008002
    .quad    0x8000000000000080
    .quad    0x000000000000800a
    .quad    0x800000008000000a
    .quad    0x8000000080008081
    .quad    0x8000000000008080
    .quad    0x0000000080000001
    .quad    0x8000000080008008
iotas_end:
.asciz    "Keccak-1600 for AVX-512F, CRYPTOGAMS by <appro@openssl.org>"
