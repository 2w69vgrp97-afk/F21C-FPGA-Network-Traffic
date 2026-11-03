# Reproduction prerequisites and limits

**This is a prerequisite guide, not a full-system build recipe.** Selected RTL helpers, a partial HLS implementation, PS application source and self-contained RTL tests are included. Complete Vivado project/BD/IP configuration, full HLS dependencies, platform/BSP and vendor IP are not supplied.

## Expected environment (from internal project records)

1. A machine with AMD Vivado, Vitis and Vitis HLS 2025.2 installed, with support for the Zynq-7020 device.
2. Appropriate access to AXI Ethernet and other required AMD/Xilinx IP and to any vendor-supplied generated drivers.
3. The project's authorized RTL/HLS sources and headers, board constraints, XPR/BD/IP configurations, and rebuild Tcl where applicable.
4. A matching PS software application, hardware platform/XSA and BSP generated with the correct tool/device settings.
5. A reproducibility log recording command lines, tool versions, source/IP configuration hashes, errors and timing/DRC/CDC results.

## What the archived review located

The full archived project was reported to contain an XPR, a block design, approximately 60 project XCI files, HLS component metadata and reconstruction Tcl. These were over-filtered from the reduced technical candidate. Their existence in the **internal** archive does not establish permission to redistribute them or prove that they rebuild the final BIT.

Some HLS headers were reproduced as separately sanitized internal-review copies. A PS app configuration retained an absolute machine path, and generated PS drivers and platform dependencies were not included in the reduced candidate.

## Suggested future gated validation

- **Gate 1: Rights and security.** Establish owners and applicable third-party licenses; review each actual release file, strip machine-specific paths/secrets and preserve required notices.
- **Gate 2: Dependency closure.** Trace RTL instances, HLS includes, BD/IP references and PS software/BSP dependencies.
- **Gate 3: Isolated tool run.** Launch the tools successfully and record real project-open, HLS C simulation/compilation, IP validation and PS build outcomes.
- **Gate 4: Hardware evidence.** Independently rebuild the selected BIT and ELF, verify platform/revision identity and close CDC concerns before claiming a verified Candidate-B board-level reproduction.

Do not label an attempted tool launch, file include scan or historical unrelated test as a successful build.

See [VALIDATION_STATUS.md](VALIDATION_STATUS.md) for current state.

## Runnable checks for the published subset

From the repository root, after installing Icarus Verilog and Yosys:

```bash
python3 verification/rtl/run_rtl_smoke.py
python3 verification/rtl/run_rtl_synth.py
```

These checks ran successfully in [GitHub Actions](https://github.com/2w69vgrp97-afk/F21C-FPGA-Network-Traffic/actions/runs/41468797933), limited to five vendor-independent helper RTL modules.

An additional original-source arithmetic regression is available for AMD Vitis HLS 2025.2, but has **not** been executed or marked PASS:

```bash
vitis_hls -f verification/hls/run_fastmod_csim.tcl
```

The HLS main implementation references `Funnelsketch.h` and `param.h`; other missing dependencies include `Top.h`, `Middle.h` and the excluded `BOBHash32.h`. The PS application likewise requires a matching platform and BSP. These are direct compilation constraints, not problems solved by the above helper RTL tests.
