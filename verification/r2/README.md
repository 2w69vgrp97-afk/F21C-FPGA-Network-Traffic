# Candidate-B R2 board-test scripts

This folder contains exact, unmodified text copies of two scripts from the archived `F21C_CANDIDATE_B_R2_BOARD_TEST_001.zip` package.

| Script | Role |
| --- | --- |
| [scripts/VERIFY_PACKAGE.ps1](scripts/VERIFY_PACKAGE.ps1) | Checks the complete R2 package payload, paths, manifest coverage and fixed BIT/XSA/ELF/PS-init hashes |
| [scripts/R2_SESSION.tcl](scripts/R2_SESSION.tcl) | XSCT board-test session with explicit operator/board identity checks and staged manual commands |

**Original R2 ZIP SHA-256:** `92E07A04C12FA69032FA5ACCC39928A01639B091804D894C99709A8F0B32233E`

**Status:** `TEST_ONLY`. The source archive describes the R2 package as ready for controlled board-test preparation. It does **not** establish a successful physical-board test.

These scripts expect the **complete, unchanged original R2 package layout**, including signed manifests and matching BIT/XSA/ELF and PS initialization assets. Those binaries and vendor-generated files are **not distributed in this GitHub repository**; running these scripts directly from this partial repository will fail package verification. The scripts are provided for engineering review and provenance only.

Sourcing `R2_SESSION.tcl` defines procedures without performing hardware operations. Connection, FPGA programming, ELF download and execution require separate explicit calls and a reviewed board-test procedure. No physical board action was performed when adding these files to GitHub.
