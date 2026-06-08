#pragma once

#ifdef __AVX512F__
#define SIMD_ALIGN alignas(64)
#elifdef __AVX2__
#define SIMD_ALIGN alignas(32)
#else
#define SIMD_ALIGN alignas(16)
#endif

namespace NNUE
{
    int32_t forward(const Network& __restrict network, int16_t* __restrict stm, int16_t* __restrict nstm, uint8_t bucket);
}