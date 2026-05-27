#pragma once

#ifdef __AVX512F__
    #include "avx512.hpp"
#elifdef __AVX2__
    #include "avx2.hpp"
#else
    #include "scalar.hpp"
#endif