# F21C FPGA Network Flow Measurement

This private repository contains the **frozen Candidate-B competition source delivery** under [candidate-b/](candidate-b/), including the RTL, HLS and PS source snapshots, Vivado BD/XCI/XDC/XPR inputs and reconstruction scripts. The original top-level RTL/HLS/PS files and tests are retained separately for standalone verification. Inclusion of source files does **not** mean the full design has been independently rebuilt.

The prototype targets Zynq-7020 on the AC7021B platform and uses a Funnel-Sketch-based design with PS/PL cooperation. The archived project requires AMD Vivado, Vitis and Vitis HLS 2025.2, device support, platform/BSP and appropriately licensed IP, including AXI Ethernet. The [candidate-b/](candidate-b/) delivery now includes project BD/IP configuration and HLS/PS build entry points. AMD-licensed generated IP output products, a matching built XSA/BSP, and generated drivers must still be produced with the applicable toolchain and licenses.

Historical delivery records report limited project-opening, HLS C simulation and PS build checks. A complete source-to-final-BIT rebuild and exact original ELF reproduction remain unverified. The Candidate-B board-test version identity is unknown and CDC risks remain open. The latest recorded B bitstream is TEST_ONLY. A source-level competition delivery is present, but a full source-to-bitstream and PS application reproduction from this delivery has **not** yet been verified.

Additional source files require appropriate sharing rights and a sensitive-information review. Competition portal uploads (including the separately required RAR source package) are distinct from this GitHub repository. Because this repository is private, judges require explicitly granted access.

## Standalone RTL smoke test

Run `python3 verification/rtl/run_rtl_smoke.py` with Icarus Verilog (`iverilog` and `vvp`) installed. GitHub Actions executes this test and five-helper-module generic Yosys synthesis/structural checks. [Both passed in the recorded CI run](https://github.com/2w69vgrp97-afk/F21C-FPGA-Network-Traffic/actions/runs/41468797933). This smoke test covers only the vendor-independent helper RTL, not the HLS core, Ethernet MAC, or full board-level design.

## Published code and tests

- [RTL helpers](hardware/rtl/): six Verilog modules; [RTL testbench and runner](verification/rtl/) cover five vendor-independent helpers. The sixth RTL helper instantiates AMD's IDELAYCTRL primitive and is outside this generic smoke/synthesis test.
- [Top-level HLS test subset](hls/): selected implementation and test programs whose includes are incomplete **when used alone**. The frozen Candidate-B HLS sources, including `Funnelsketch.h`, `Top.h`, `Middle.h`, `param.h`, and `BOBHash32.h`, are now included under [candidate-b/snapshots/](candidate-b/snapshots/); HLS C simulation and full IP rebuild remain unverified.
- [PS application](software/ps/main.c): the software control source, which needs a matching XSA/platform, BSP and generated peripheral drivers.
- [FastMod regression](verification/hls/): a Vitis HLS C-simulation check of the existing modulo functions. Vitis HLS 2025.2 timed out before C simulation began. The same regression compiled with the 2025.2 `ap_int` headers and run as a [host executable](verification/hls/fastmod_host_2025_2.log) reported 9,329 mismatches.
- [Measurement summary](verification/measurements/s2_1024b_ladder_summary.csv): a subset of historical S2 experiment data, not a fresh full-system benchmark.

Use `python3 verification/rtl/run_rtl_smoke.py` for helper RTL simulation and `python3 verification/rtl/run_rtl_synth.py` with Yosys for generic synthesis checks. See [validation status](docs/VALIDATION_STATUS.md) for what these results establish.

## Candidate-B competition source

[candidate-b/README.md](candidate-b/README.md) describes the frozen competition
source and Vivado, HLS and PS build entry points. The full source tree lives
under candidate-b/. The existing top-level source paths remain available for
the standalone verification programs. See the build guide for the FastMod
regression and the current limits of full-system validation.
