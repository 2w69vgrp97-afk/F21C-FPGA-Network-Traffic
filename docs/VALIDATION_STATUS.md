# Validation status

| Scope | Observed result |
| --- | --- |
| Five standalone RTL helpers | Icarus smoke simulation and Yosys generic synthesis and structural checks passed in [GitHub Actions](https://github.com/2w69vgrp97-afk/F21C-FPGA-Network-Traffic/actions/runs/41468797933). |
| Frozen HLS source | The V3 tree includes the source, headers and testbench. Vitis HLS CSim attempts did not produce a PASS. |
| Frozen FastMod | The AMD 2025.2 `ap_int` host regression reported 9,329 mismatches, including `mod1004`. See the [host log](../verification/hls/fastmod_host_2025_2.log). |
| Local archived HLS IP 2.0 | All 137 files and 136 component references verified. In a minimal Vivado 2025.2 project, IP Catalog lookup, `create_ip` and `create_bd_cell` passed. |
| Local complete XPR and BD | With the archived IP attached to an isolated copy, `open_project` and `validate_bd_design` passed. IP status collected after BD load showed HLS 2.0 and DMA available, with Ethernet/TEMAC under hardware evaluation. These checks were run locally, outside GitHub CI. |
| Local DMA experiment | Disabling DMA1 S2MM and DMA0 MM2S removed the unconnected-channel warning in the isolated BD. The experiment remains outside frozen V3 and would require compatible PS initialization and further verification. |
| Full rebuild and board | Full synthesis, implementation, matched BIT/XSA/ELF production and board validation of this repository delivery remain open. CDC closure remains unverified. |

Historical reports are catalogued in [evidence provenance](../candidate-b/evidence/provenance.csv). [R2](../candidate-b/board/R2_TEST_ONLY/) has a separately tracked test-only artifact identity. The [submission status](../SUBMISSION_STATUS.md) describes the local-only V4 candidate and its distribution boundary.
