# F21C FPGA Network Flow Measurement

F21C measures per-flow packet frequencies on the AC7021B Zynq-7020 platform. Programmable logic parses eligible IPv4 TCP/UDP frames, preprocesses flow hashes and updates a Funnel-Sketch statistics core. The design adapts Funnel-Sketch into HLS tasks with FIFO decoupling and an ordered backend for updates, queries and resets.

Ethernet frames enter through PHY/RGMII and the AXI Ethernet subsystem. The PS initializes the datapath, sends query keys to HLS `query_id_in` through **DMA1 MM2S**, and reads estimated counts from HLS `query_size_out` through **DMA0 S2MM**. See the [system architecture](docs/SYSTEM_ARCHITECTURE.md).

## Project entry points

| Area | Repository location |
| --- | --- |
| Frozen Candidate-B V3 | [candidate-b/](candidate-b/) |
| Vivado project and block design | [FINAL_RUNTIME.xpr](candidate-b/project/FINAL_RUNTIME.xpr), [FS_Design.bd](candidate-b/project/FINAL_RUNTIME.srcs/sources_1/bd/FS_Design/FS_Design.bd), [project configuration](candidate-b/project/) |
| Funnel-Sketch and HLS top | [Frozen HLS inputs and testbenches](candidate-b/snapshots/main_workspace/02_WORK/hls_synthesis_iteration_1/project_retry2/inputs/) |
| PS application | [main.c](candidate-b/snapshots/ps_workspace/F21C_FINAL_RUNTIME_APP/src/main.c) |
| Build entry points | [candidate-b/scripts/](candidate-b/scripts/), [reproduction guide](docs/REPRODUCTION_GUIDE.md) |
| Standalone test inputs | [hardware/](hardware/), [hls/](hls/), [software/](software/), [verification/](verification/) |
| Evidence and technical notes | [V3 evidence](candidate-b/evidence/), [docs/](docs/), [R2 test-only notes](verification/r2/README.md) |

## Build and validation

Use Vivado, Vitis and Vitis HLS 2025.2 with Zynq-7000 device support. The build guide covers HLS IP export, Vivado catalog registration, BD validation and PS platform generation. The repository contains the frozen HLS source and Vivado configuration. The pre-exported HLS IP 2.0, matching XSA/BSP and built BIT/ELF are external build inputs or outputs as described in [dependencies](DEPENDENCIES.md).

Five standalone RTL helpers passed Icarus simulation and Yosys generic synthesis in the [recorded CI run](https://github.com/2w69vgrp97-afk/F21C-FPGA-Network-Traffic/actions/runs/41468797933). In a separate local run, Vivado recognized and instantiated the archived HLS IP 2.0, opened the full XPR and validated the BD. The frozen FastMod host regression recorded 9,329 mismatches, and the HLS CSim attempts did not reach PASS. Full synthesis, implementation, matched BIT/XSA/ELF reproduction and board validation remain open. See [validation status](docs/VALIDATION_STATUS.md).

## Competition delivery

[Submission status](SUBMISSION_STATUS.md) identifies V3 and the local-only V4 candidate. Historical reports and R2 test-only materials are labeled in their respective directories. This repository provides technical documentation and verification records. Competition reports and demo materials are handled separately. The [AI collaboration note](docs/AI_COLLABORATION.md) describes the scope of assistance. The [release review skill](skill/fpga-release-audit/SKILL.md) is a draft with independent runtime validation pending.

The repository is private. Reviewers need access granted by the team. AXI Ethernet uses separately licensed AMD IP, and the generated HLS IP package has a separate distribution review.
