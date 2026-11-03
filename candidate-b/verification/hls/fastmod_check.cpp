// Standalone Vitis HLS C-simulation wrapper for the existing FastMod.h.
// Does not modify the implementation under test and does not need
// FunnelSketch.h, BOBHash32.h, param.h, or a Vivado block design.
#include <cstdint>
#include "../../snapshots/main_workspace/02_WORK/hls_synthesis_iteration_1/project_retry2/inputs/candidate_b/FunnelSketch-main/FastMod.h"

void fastmod_check(
    uint32_t input,
    ap_uint<5> &mod25,
    ap_uint<10> &mod1004,
    ap_uint<17> &mod87040
) {
    mod25 = fast_mod_25(input);
    mod1004 = fast_mod_1004(input);
    mod87040 = fast_mod_87040(input);
}
