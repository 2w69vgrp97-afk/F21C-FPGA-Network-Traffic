# Competition delivery status

**Last updated:** 2026-11-03. The primary design report is being revised separately and is intentionally **not** committed here.

| Material | This repository | Status |
| --- | --- | --- |
| GitHub repository link | Yes, private | Link created; reviewer visibility requires access |
| README and dependencies | Yes | Available |
| System architecture notes | Yes | Documentation only |
| Verification/evidence status summary | Yes | GitHub CI has separately passed helper RTL simulation and generic Yosys synthesis; historical full-system logs are not included |
| Reproduction prerequisites | Yes | Not an executable, verified build |
| AI-assistance disclosure | Yes | Summary, not a raw collaboration log |
| Draft reusable FPGA audit Skill | Yes | Draft; no independent Skill execution test |
| RTL / HLS / PS source | Yes, partial | Six auxiliary RTL modules, six HLS-related files and one PS C source file; main HLS dependencies and complete project are still missing |
| Vivado project, BD/XCI/Tcl, platform/BSP | No | Rights and independent rebuild validation pending |
| Third-party IP and generated drivers | No | Licenses not cleared for redistribution |
| Report PDF | No | Still under revision |
| Demonstration video | No | Final selection and upload not completed |
| Original raw test data / boards / binaries | Partial summary only | A historical S2 summary CSV is published; raw logs, full test data and binaries remain separate |

## Important: GitHub is not the competition submission portal

A private repository URL is real but cannot be read by judges without granted access. The official competition upload form, permitted visibility and required attachment fields must be checked independently. No claim of final competition submission is made here.

Before changing repository visibility or publishing project source, obtain actual rights-holder approval and review the public release file-by-file.

## Current independently runnable GitHub checks

- Five standalone auxiliary RTL modules: Icarus Verilog smoke test and Yosys generic synthesis/structural checks, [CI PASS](https://github.com/2w69vgrp97-afk/F21C-FPGA-Network-Traffic/actions/runs/41468797933).
- `verification/hls/run_fastmod_csim.tcl`: available for AMD Vitis HLS execution, but no C-simulation PASS has yet been obtained.
- Complete Funnel-Sketch HLS, Vivado platform, PS BSP and original Candidate-B BIT/ELF pairing remain outside the independently reproducible scope of this repository.
