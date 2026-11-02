# Reproduction prerequisites and limits

**This is a prerequisite guide, not an executable build recipe.** No Vivado project, HLS implementation, board design, platform/BSP, software source, or vendor IP configuration is currently supplied in this repository.

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
