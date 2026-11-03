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
