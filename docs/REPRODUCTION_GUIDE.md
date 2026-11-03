# Reproduction guide

Start from the repository root and use a writable copy of [candidate-b/](../candidate-b/), preferably at a short Windows path. Use AMD Vivado, Vitis and Vitis HLS 2025.2 with [the required device support and licenses](../DEPENDENCIES.md).

The [HLS inputs](../candidate-b/snapshots/main_workspace/02_WORK/hls_synthesis_iteration_1/project_retry2/inputs/) contain the core, headers and functional testbench. The [Vivado project](../candidate-b/project/) contains `FINAL_RUNTIME.xpr`, `FS_Design.bd`, XCI, XDC and custom RTL. The [PS application directory](../candidate-b/snapshots/ps_workspace/F21C_FINAL_RUNTIME_APP/src/) contains `main.c`, the linker script and CMake inputs.

## Build sequence

Run these from the root of the Candidate-B copy in a toolchain-enabled shell.

1. Run `vitis-run --mode hls --tcl scripts/rebuild_hls.tcl` for functional CSim. For the export path, set `F21C_HLS_STAGE=export` in the environment and run the same command. The script runs CSim before synthesis and export. Its intended output is `validation/hls_*/fs_hls_test_candidate_b_2_0.zip`.
2. Extract the resulting IP so `component.xml` is directly under `project/candidate_b_hls_repo/fs_hls_test_2_0/`. This is the repository used by the frozen XPR and build scripts. The alternate project recreation entry is `vivado -mode batch -source scripts/recreate_project.tcl`.
3. Run `vivado -mode batch -source scripts/export_platform.tcl`. The script opens the project, registers the IP, validates the BD, generates output products and attempts to export `build/FINAL_RUNTIME.xsa` without a bitstream.
4. Run `vitis -s scripts/rebuild_ps.py`. The script uses that XSA to build the standalone platform and PS application. `F21C_XSA` can point to another hardware handoff with independently established matching identity.

Only the catalog registration, instantiation, full project open and BD validation were confirmed in the isolated Vivado run with the already-exported local IP. The CSim/export path above and the platform and application build sequence have yet to complete end to end from this repository. Use the [validation status](VALIDATION_STATUS.md) when interpreting historical output.

From the repository root, `python verification/rtl/run_rtl_smoke.py` and `python verification/rtl/run_rtl_synth.py` run the five-helper Icarus and Yosys checks. The corresponding runners also exist inside the Candidate-B tree with paths adapted to the frozen inputs.
