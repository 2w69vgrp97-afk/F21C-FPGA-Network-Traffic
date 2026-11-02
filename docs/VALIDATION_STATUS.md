# Validation and evidence status

**Status date:** 2026-11-02. This page summarizes existing review records; no new FPGA compilation, simulation or physical-board test is claimed by this commit.

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
| GitHub source release | NOT APPROVED | Source and IP implementations are intentionally absent. |

## Interpretation of prior tests

Archived project evidence contains HLS, FPGA implementation and software checks and reports of statistical accuracy. Those are **historical records**, not tests rerun on the GitHub repository contents. Internal cycle-interval comparisons must not be presented as end-to-end Ethernet packet throughput.

## Evidence availability

Original logs, data sets, design sources, bitstreams and vendor-generated artifacts are held separately in the project's internal delivery evidence. They have **not** been published here and must not be inferred from this documentation. Any later test result should carry a date, target revision, actual command/tool version, input hash and log reference.

The documentation-only repository is not buildable and cannot be used to verify hardware behavior directly.
