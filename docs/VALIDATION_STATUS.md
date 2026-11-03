# Validation and evidence status

**Historical review baseline:** 2026-11-02. GitHub source/CI status updated 2026-11-03. Archived full-system tests are distinct from the standalone GitHub CI checks below.

| Item | Recorded status | What it establishes |
| --- | --- | --- |
| Candidate-B archived delivery integrity | PASS | All 17,886 payload entries matched the archived size and SHA-256 manifest; the ZIP includes two additional manifest/check files (17,888 total). |
| Initial pattern-scan triage | 49 FALSE_POSITIVE, 0 CONFIRMED, 0 UNRESOLVED | The 49 originally flagged patterns were found in protected vendor data blocks; this is not a comprehensive secrets clearance. |
| Isolated internal technical candidate | 30 files, RIGHTS_PENDING | A limited internal-review selection, **not** a release-approved or complete source archive. |
| Candidate-B source-to-final BIT rebuild | NOT VERIFIED | The exact final bitstream has not been independently reproduced from a publishable source package. |
| Exact original ELF reproduction | NOT VERIFIED | The archived software build identity has not been reproduced exactly. |
| Candidate-B board-test version identity | UNKNOWN | Archived BIT/ELF/platform provenance does not establish that the final Candidate-B pairing underwent the claimed board tests. |
| CDC issues | OPEN | Critical clock-domain-crossing concerns remain unresolved. |
| Vivado/Vitis tool execution for the reduced candidate | BLOCKED | Launch attempts failed or stalled in the recorded review environment; no new HLS simulation, project-open PASS or PS build PASS was obtained. |
| Ethernet MAC / related vendor IP licenses | PENDING | Third-party configurations and generated files require their applicable permissions. |
| GitHub source release | PARTIAL | Six auxiliary RTL, six HLS-related and one PS source file are present. Third-party protected implementations and core build dependencies remain absent. |

## Interpretation of prior tests

Archived project evidence contains HLS, FPGA implementation and software checks and reports of statistical accuracy. Those are **historical records**, not tests rerun on the GitHub repository contents. Internal cycle-interval comparisons must not be presented as end-to-end Ethernet packet throughput.

## Evidence availability

Most original logs, full data sets, bitstreams and vendor-generated artifacts remain in the internal delivery evidence. Selected RTL/HLS/PS source files and an S2 measurement summary **are** published here; their presence does not prove full-system reproduction. Any later test result should carry a date, target revision, actual command/tool version, input hash and log reference.

The repository can independently run limited auxiliary RTL checks, but does not yet support complete HLS, PS, Vivado or board-level reconstruction.

## GitHub-hosted regression evidence

| Check | Source files and scope | Recorded outcome |
| --- | --- | --- |
| Icarus Verilog helper RTL smoke test | `verification/rtl/rtl_smoke_tb.sv` and five standalone helpers | PASS in GitHub Actions on [2026-11-02](https://github.com/2w69vgrp97-afk/F21C-FPGA-Network-Traffic/actions/runs/41468797933) |
| Yosys generic synthesis and structural checks | Same five vendor-independent helper modules; excludes `pl_idelayctrl.v` | PASS in the [same CI run](https://github.com/2w69vgrp97-afk/F21C-FPGA-Network-Traffic/actions/runs/41468797933) |
| FastMod HLS arithmetic C simulation | `verification/hls/run_fastmod_csim.tcl` with original `FastMod.h` | NOT RUN; script has been published, no actual Vitis HLS test result yet |

Neither CI PASS above is evidence of Ethernet MAC integration, the Funnel-Sketch backend, CDC closure, a full bitstream, or board-level operation.
