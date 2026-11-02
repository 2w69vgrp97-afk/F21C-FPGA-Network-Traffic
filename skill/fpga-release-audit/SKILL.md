---
name: fpga-release-audit
description: Evidence-based review of FPGA/Vivado/Vitis source release candidates, dependency closure, licensing and reproducibility status.
---

# FPGA release candidate audit

**Status: draft workflow, not independently validated.** Apply this skill to internal FPGA project handoff and source-release reviews. It performs no automatic actions by itself.

## Inputs

- An immutable source archive and its inventory/hash manifest.
- The actual RTL/HLS/PS project references, XPR/BD/XCI and build scripts when available.
- Historical build/test evidence and tool versions.
- Rights/authorization instructions from the project owner.

## Procedure

1. **Freeze evidence.** Record archive and manifest hashes; verify payload sizes and hashes. Do not alter the original archive.
2. **Separate classes.** Classify authored sources, third-party material, IP configuration, protected generated IP, caches, binaries, licenses and secrets. A file extension or project-folder name alone does not prove authorship.
3. **Trace dependencies.** Follow project references, source includes, instantiated modules and Tcl/BD/IP connections. Determine what is necessary for an architecture review, source compilation and complete FPGA rebuild separately.
4. **Review rights and sensitive data.** Retain required copyright notices; verify original licenses and team permissions. Review private keys, personal contact details, machine paths and generated protected vendor data. Record false positives with bounded explanations. Never publish tokens or credentials in reports.
5. **Prepare isolated candidates.** Copy only files authorized for the candidate's intended audience into a new directory. Keep every item's original path, hash, purpose, scan result and rights status in a manifest.
6. **Run only possible checks.** Distinguish static inspection, tool launch, HLS simulation, Vivado project validation, bitstream rebuild, PS/ELF build and board testing. Capture real logs, return codes and revision identity.
7. **Write a release decision.** Independently report TECHNICAL_READY, LICENSE_READY, TEAM_APPROVED and REMOTE_UPLOAD_VERIFIED, with PASS/BLOCKED/UNKNOWN and the exact evidence for each status.

## Mandatory stop conditions

- Do not upload `RIGHTS_PENDING` or vendor-protected source to a public repository.
- Do not add MIT/Apache/GPL licenses without rights-holder approval.
- Do not claim a project is reproducible from a documentation-only repository.
- Do not convert internal cycle measurements into network line-rate claims.
- Do not mark bitstream/ELF revision pairing or CDC closure as passed without direct evidence.

## Outputs

A candidate manifest, dependency and license blockers list, redacted scan triage, build/verification matrix, release status and a concise reviewer-oriented README.
