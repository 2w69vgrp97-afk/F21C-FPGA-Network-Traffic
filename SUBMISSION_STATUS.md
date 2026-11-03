# Competition submission status

The submission baseline is Candidate-B V3. The [frozen delivery tree](candidate-b/) contains 118 files and corresponds to commit `6d2a960a80d2f09cc06da4019ae9fb65ffc0c20b`.

| V3 submission archive | Value |
| --- | --- |
| File | `F21C_完整工程源码_竞赛提交版_V3.rar` |
| Format and size | Single-volume RAR5, 322,731 bytes |
| SHA-256 | `3D7642A55D8F0B43A799ED71A3FB2A8F94AA424475B2D623E036568C10B55347` |

V3 includes the frozen HLS sources and headers, six custom RTL helpers, the PS application, the Vivado XPR, three BD files, 60 XCI files, four XDC files, reconstruction scripts and verification material. The submission archive is provided separately through the competition portal.

The pre-exported HLS IP 2.0 directory and prebuilt BIT, XSA and ELF are outside the V3 delivery. A local-only V4 candidate adds the original 137-file HLS IP directory, totaling 5,076,303 uncompressed bytes. Its file checks passed. Vivado also registered and instantiated the archived IP and validated the full BD in local isolation. Distribution rights for the entire generated directory are pending. V4 is outside the GitHub and portal submissions.

The R2 board handoff is TEST_ONLY. The [R2 notes](candidate-b/board/R2_TEST_ONLY/) record the static artifact pairing and the session scripts. End-to-end reconstruction and board acceptance remain open. See [validation status](docs/VALIDATION_STATUS.md).

Repository access is private and must be granted to authorized reviewers.
