# F21C Candidate-B competition source

This release uses the frozen Candidate-B source ZIP (SHA-256
83047509F6F41FBA0A380B590608A3A6A776E3A59F673733FED7499E8375A487).
Use Vivado, Vitis and Vitis HLS 2025.2 with Zynq-7000 support and the
applicable AMD IP licenses, including AXI Ethernet 8.0.

- project/ contains FINAL_RUNTIME.xpr, six custom RTL modules, three BD files,
  60 XCI files, four XDC files and the structural BD wrapper.
- snapshots/main_workspace/02_WORK/hls_synthesis_iteration_1/project_retry2/inputs/
  contains the complete frozen Candidate-B HLS source, headers and testbenches.
- snapshots/ps_workspace/F21C_FINAL_RUNTIME_APP/src/ contains the PS application,
  linker script and CMake configuration.
- scripts/ and validation/vivado/ contain the build entry points and project recreation Tcl.

Run the following from the release root, using a writable copy and an AMD 2025.2 shell.

1. Run vitis-run --mode hls --tcl scripts/rebuild_hls.tcl for the original
   Candidate-B functional CSim. To synthesize and export, set F21C_HLS_STAGE
   to export before running that command. The script writes its result under
   validation/hls_*/fs_hls_test_candidate_b_2_0.zip.
2. Unpack the exported IP so component.xml sits directly under
   project/candidate_b_hls_repo/fs_hls_test_2_0/. The active BD and XCI bind
   xilinx.com:hls:fs_hls_test:2.0. AMD catalog IP output products are generated
   by Vivado. The Tcl project recreation entry is scripts/recreate_project.tcl.
3. Run vivado -mode batch -source scripts/export_platform.tcl. This validates
   the BD, generates output products and exports build/FINAL_RUNTIME.xsa
   without a bitstream. The XSA must come from this design.
4. Run vitis -s scripts/rebuild_ps.py. It creates a Cortex-A9 standalone
   platform and builds the PS app. Set F21C_XSA to an alternative matching
   XSA if required. The generated BSP supplies xparameters.h, DMA and
   Ethernet drivers and the HLS peripheral driver, including xfs_hls_test.h.

Validation status: the six helper RTL modules compiled and elaborated in
Vivado Simulator 2025.2. Both Vitis HLS CSim launches timed out before
simulation. The original FastMod host regression with AMD 2025.2 ap_int
headers reported 9,329 mismatches, including mod1004 returning 400 for
0x10000000 (expected 996). The frozen implementation is included. The
FastMod fix in PR #1 is separate. Vivado project reconstruction, IP export,
a matching XSA and full PS/BIT builds remain unverified on this host.

## V2 verification and evidence

- FPGA top and BD: project/FINAL_RUNTIME.srcs/sources_1/bd/FS_Design/.
  Custom RTL is under project/FINAL_RUNTIME.srcs/sources_1/new/.
- The frozen HLS inputs above contain Top.h, Middle.h, Bottom.h, the
  fs_hls_test top and the functional testbench. The testbenches generate
  stimulus in code and require no external capture dataset.
- The PS entry is snapshots/ps_workspace/F21C_FINAL_RUNTIME_APP/src/main.c.
  The local linker and CMake inputs are beside it. Tool-generated driver
  and platform files come from the matching XSA and BSP.
- With Icarus Verilog or Yosys on PATH, run
  python verification/rtl/run_rtl_smoke.py or
  python verification/rtl/run_rtl_synth.py from the package root.
  These checks cover the five standalone helpers.
- Run vitis-run --mode hls --tcl verification/hls/run_fastmod_csim.tcl
  from the package root for the frozen arithmetic regression. The included
  host transcript records 9,329 mismatches. No fresh CSim PASS is claimed.
- evidence/provenance.csv records source and delivered hashes. The selected
  archived HLS, Vivado and traffic results under HISTORICAL_REFERENCE
  are historical context. Their presence does not establish that a
  historical BIT, XSA or ELF was produced from this exact release.
  R2 board scripts remain in the repository's separate verification/r2
  tree because their artifacts have a separate identity.

V2 local check: Icarus smoke PASS on 2026-11-04. Yosys was unavailable.

## V3 board handoff

board/R2_TEST_ONLY contains the original relative-path XSCT session and
integrity scripts, candidate hashes, and static pairing results for AC7021B.
The R2 XSA embeds the exact R2 BIT, its PS init matches, and the archived
platform records link its ELF to that XSA. R2 is for controlled testing.
The frozen handoff XSA embeds a different BIT and advertises HLS 1.1.
No matched release image was found. R2 binaries and PS initialization
are retained locally pending confirmation of AMD evaluation-IP distribution
rights. The archived R2 BIT uses Ethernet evaluation IP.
The original R2 board package is required to exercise the session scripts.
