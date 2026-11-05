#ifndef _FAST_MOD_H
#define _FAST_MOD_H

#include <cstdint>
#include "ap_int.h"

// Exact constant modulo for 32-bit hashes. Vivado HLS 2018.3 often maps
// "x % const" to a large divider; reciprocal multiply plus one correction is
// much cheaper for the fixed FunnelSketch table sizes.
static inline ap_uint<5> fast_mod_25(uint32_t x) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    // 2^5 == 32 == 7 (mod 25), and 7^4 == 1 (mod 25).
    // Folding 5-bit chunks avoids the former 68-bit reciprocal multiply and
    // 64-bit subtract path that fed Middle A1 BRAM write enables.
    ap_uint<5> d0 = static_cast<ap_uint<5> >(x & 31U);
    ap_uint<5> d1 = static_cast<ap_uint<5> >((x >> 5) & 31U);
    ap_uint<5> d2 = static_cast<ap_uint<5> >((x >> 10) & 31U);
    ap_uint<5> d3 = static_cast<ap_uint<5> >((x >> 15) & 31U);
    ap_uint<5> d4 = static_cast<ap_uint<5> >((x >> 20) & 31U);
    ap_uint<5> d5 = static_cast<ap_uint<5> >((x >> 25) & 31U);
    ap_uint<2> d6 = static_cast<ap_uint<2> >(x >> 30);

    ap_uint<11> d1x7  = (static_cast<ap_uint<11> >(d1) << 2)
                      + (static_cast<ap_uint<11> >(d1) << 1)
                      +  static_cast<ap_uint<11> >(d1);
    ap_uint<11> d2x24 = (static_cast<ap_uint<11> >(d2) << 4)
                      + (static_cast<ap_uint<11> >(d2) << 3);
    ap_uint<11> d3x18 = (static_cast<ap_uint<11> >(d3) << 4)
                      + (static_cast<ap_uint<11> >(d3) << 1);
    ap_uint<11> d5x7  = (static_cast<ap_uint<11> >(d5) << 2)
                      + (static_cast<ap_uint<11> >(d5) << 1)
                      +  static_cast<ap_uint<11> >(d5);
    ap_uint<11> d6x24 = (static_cast<ap_uint<11> >(d6) << 4)
                      + (static_cast<ap_uint<11> >(d6) << 3);

    ap_uint<11> s = static_cast<ap_uint<11> >(d0)
                  + d1x7
                  + d2x24
                  + d3x18
                  + static_cast<ap_uint<11> >(d4)
                  + d5x7
                  + d6x24;

    ap_uint<9> r0 = static_cast<ap_uint<5> >(s & 31U)
                  + (static_cast<ap_uint<9> >(s >> 5) << 2)
                  + (static_cast<ap_uint<9> >(s >> 5) << 1)
                  +  static_cast<ap_uint<9> >(s >> 5);
    ap_uint<7> r1 = static_cast<ap_uint<5> >(r0 & 31U)
                  + (static_cast<ap_uint<7> >(r0 >> 5) << 2)
                  + (static_cast<ap_uint<7> >(r0 >> 5) << 1)
                  +  static_cast<ap_uint<7> >(r0 >> 5);
    ap_uint<6> r = static_cast<ap_uint<5> >(r1 & 31U)
                 + (static_cast<ap_uint<6> >(r1 >> 5) << 2)
                 + (static_cast<ap_uint<6> >(r1 >> 5) << 1)
                 +  static_cast<ap_uint<6> >(r1 >> 5);

    if (r >= 25U) {
        r -= 25U;
    }
    if (r >= 25U) {
        r -= 25U;
    }
    return static_cast<ap_uint<5> >(r);
}

static inline ap_uint<10> fast_mod_1004(uint32_t x) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    // 2^10 == 1024 == 20 (mod 1004).
    // Every shift operand is widened BEFORE shifting; ap_uint shifts
    // retain the width of their left operand and otherwise truncate bits.
    // All stage maxima fit the widths declared below for any uint32_t x.
    const ap_uint<27> h0 = static_cast<ap_uint<27> >(x >> 10);
    ap_uint<27> y0_partial;
    #pragma HLS BIND_OP variable=y0_partial op=add impl=fabric latency=1
    y0_partial = static_cast<ap_uint<27> >(x & 1023U) + (h0 << 4);
    const ap_uint<27> y0 = y0_partial + (h0 << 2);

    const ap_uint<21> h1 = static_cast<ap_uint<21> >(y0 >> 10);
    const ap_uint<21> y1 = static_cast<ap_uint<21> >(y0 & 1023U)
                         + (h1 << 4) + (h1 << 2);

    const ap_uint<16> h2 = static_cast<ap_uint<16> >(y1 >> 10);
    const ap_uint<16> y2 = static_cast<ap_uint<16> >(y1 & 1023U)
                         + (h2 << 4) + (h2 << 2);

    const ap_uint<11> h3 = static_cast<ap_uint<11> >(y2 >> 10);
    ap_uint<11> r = static_cast<ap_uint<11> >(y2 & 1023U)
                  + (h3 << 4) + (h3 << 2);
    if (r >= 1004U) {
        r -= 1004U;
    }
    return static_cast<ap_uint<10> >(r);
}

static inline ap_uint<17> fast_mod_87040(uint32_t x) {
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1
    const ap_uint<24> recip = 12632256U; // floor(2^40 / 87040)
    ap_uint<56> prod = static_cast<ap_uint<32> >(x) * recip;
    ap_uint<32> q = prod >> 40;
    ap_uint<64> r = static_cast<ap_uint<64> >(x) - static_cast<ap_uint<64> >(q) * 87040U;
    if (r >= 87040U) {
        r -= 87040U;
    }
    return static_cast<ap_uint<17> >(r);
}

#endif // _FAST_MOD_H
