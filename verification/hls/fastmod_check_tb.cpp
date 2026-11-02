// Exactness regression for the *actual* HLS ap_uint implementation.
// Run with: vitis_hls -f verification/hls/run_fastmod_csim.tcl
// from the GitHub repository root using a suitable Vitis HLS 2025.2 shell.
#include <cstdint>
#include <cstdio>
#include "ap_int.h"

void fastmod_check(
    uint32_t input, ap_uint<5> &mod25,
    ap_uint<10> &mod1004, ap_uint<17> &mod87040
);

static unsigned mismatches = 0;
static void verify(uint32_t x) {
    ap_uint<5> a;
    ap_uint<10> b;
    ap_uint<17> c;
    fastmod_check(x, a, b, c);
    const uint32_t got25 = a.to_uint();
    const uint32_t got1004 = b.to_uint();
    const uint32_t got87040 = c.to_uint();

    if (got25 != x % 25U || got1004 != x % 1004U ||
        got87040 != x % 87040U) {
        if (mismatches < 16U) {
            std::printf(
                "MISMATCH x=0x%08X: mod25 %u/%u; mod1004 %u/%u; mod87040 %u/%u\n",
                static_cast<unsigned>(x),
                static_cast<unsigned>(got25), static_cast<unsigned>(x % 25U),
                static_cast<unsigned>(got1004), static_cast<unsigned>(x % 1004U),
                static_cast<unsigned>(got87040), static_cast<unsigned>(x % 87040U)
            );
        }
        ++mismatches;
    }
}

int main() {
    const uint32_t boundaries[] = {
        0U, 1U, 24U, 25U, 26U,
        1003U, 1004U, 1005U,
        87039U, 87040U, 87041U,
        0x000FFFFFU, 0x00FFFFFFU,
        0x08000000U, 0x10000000U, 0x20000000U,
        0x40000000U, 0x80000000U,
        0x12345678U, 0xFFFFFFFFU
    };
    for (unsigned i = 0; i < sizeof(boundaries) / sizeof(boundaries[0]); ++i) {
        verify(boundaries[i]);
    }

    uint32_t state = 0xC001D00DU;
    for (unsigned i = 0; i < 10000U; ++i) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        verify(state);
    }

    if (mismatches != 0U) {
        std::printf("FAIL: %u FastMod exactness mismatch(es)\n", mismatches);
        return 1;
    }
    std::printf("PASS: FastMod exactness (20 boundaries + 10000 samples)\n");
    return 0;
}
