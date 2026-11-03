# F21C FPGA Network Flow Measurement

This private repository now contains a **partial source release** of the F21C Candidate-B FPGA network-flow measurement prototype: six auxiliary RTL modules, six HLS-related source/header/test files, one PS application source file, and a standalone RTL smoke test. It is **not** a complete competition source delivery.

The prototype targets Zynq-7020 on the AC7021B platform and uses a Funnel-Sketch-based design with PS/PL cooperation. The archived project requires AMD Vivado, Vitis and Vitis HLS 2025.2, device support, platform/BSP and appropriately licensed IP, including AXI Ethernet. Selected source files and an RTL test runner are provided under `hardware/`, `hls/`, `software/`, and `verification/`. Complete Vivado BD/IP configuration, generated drivers, BSP and full-system build scripts are not provided.

Historical delivery records report limited project-opening, HLS C simulation and PS build checks. A complete source-to-final-BIT rebuild and exact original ELF reproduction remain unverified. The Candidate-B board-test version identity is unknown and CDC risks remain open. The latest recorded B bitstream is TEST_ONLY. The currently published subset cannot reproduce the complete system.

Additional source files require appropriate sharing rights and a sensitive-information review. Competition portal uploads (including the separately required RAR source package) are distinct from this GitHub repository. Because this repository is private, judges require explicitly granted access.

## Standalone RTL smoke test

Run `python3 verification/rtl/run_rtl_smoke.py` with Icarus Verilog (`iverilog` and `vvp`) installed. GitHub Actions executes this test and five-helper-module generic Yosys synthesis/structural checks. [Both passed in the recorded CI run](https://github.com/2w69vgrp97-afk/F21C-FPGA-Network-Traffic/actions/runs/41468797933). This smoke test covers only the vendor-independent helper RTL, not the HLS core, Ethernet MAC, or full board-level design.

## Published code and tests

- [RTL helpers](hardware/rtl/): six Verilog modules; [RTL testbench and runner](verification/rtl/) cover five vendor-independent helpers. The sixth RTL helper instantiates AMD's IDELAYCTRL primitive and is outside this generic smoke/synthesis test.
- [HLS subset](hls/): selected Funnel-Sketch implementation, headers and test programs. The complete HLS core cannot currently be built from these files because of missing dependencies including `Funnelsketch.h`, `Top.h`, `Middle.h`, `param.h`, and the excluded `BOBHash32.h` implementation.
- [PS application](software/ps/main.c): the software control source, which needs a matching XSA/platform, BSP and generated peripheral drivers.
- [FastMod regression](verification/hls/): a Vitis HLS C-simulation check of the existing modulo functions. **Not yet run in Vitis HLS**; no PASS is claimed for this test.
- [Measurement summary](verification/measurements/s2_1024b_ladder_summary.csv): a subset of historical S2 experiment data, not a fresh full-system benchmark.

Use `python3 verification/rtl/run_rtl_smoke.py` for helper RTL simulation and `python3 verification/rtl/run_rtl_synth.py` with Yosys for generic synthesis checks. See [validation status](docs/VALIDATION_STATUS.md) for what these results establish.
