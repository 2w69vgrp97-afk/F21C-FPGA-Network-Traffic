# F21C FPGA Network Flow Measurement

This private repository now contains a **partial source release** of the F21C Candidate-B FPGA network-flow measurement prototype: six auxiliary RTL modules, six HLS-related source/header/test files, one PS application source file, and a standalone RTL smoke test. It is **not** a complete competition source delivery.

The prototype targets Zynq-7020 on the AC7021B platform and uses a Funnel-Sketch-based design with PS/PL cooperation. The archived project requires AMD Vivado, Vitis and Vitis HLS 2025.2, device support, platform/BSP and appropriately licensed IP, including AXI Ethernet. Selected source files and an RTL test runner are provided under `hardware/`, `hls/`, `software/`, and `verification/`. Complete Vivado BD/IP configuration, generated drivers, BSP and full-system build scripts are not provided.

Historical delivery records report limited project-opening, HLS C simulation and PS build checks. A complete source-to-final-BIT rebuild and exact original ELF reproduction remain unverified. The Candidate-B board-test version identity is unknown and CDC risks remain open. The latest recorded B bitstream is TEST_ONLY. The currently published subset cannot reproduce the complete system.

Additional source files require appropriate sharing rights and a sensitive-information review. Competition submission requirements and reviewer access remain to be confirmed.

## Standalone RTL smoke test

Run `python3 verification/rtl/run_rtl_smoke.py` with Icarus Verilog (`iverilog` and `vvp`) installed. GitHub Actions executes the same test on changes to the helper RTL and test files; [the initial CI run passed on 2026-11-02](https://github.com/2w69vgrp97-afk/F21C-FPGA-Network-Traffic/actions/runs/41467787233). This smoke test covers only the vendor-independent helper RTL, not the HLS core, Ethernet MAC, or full board-level design.
