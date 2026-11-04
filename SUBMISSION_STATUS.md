# Competition submission status

The public [frozen Candidate-B V3 delivery tree](candidate-b/) contains 118 files and corresponds to commit `6d2a960a80d2f09cc06da4019ae9fb65ffc0c20b`. The latest separate local candidates are:

| Candidate | Purpose | Size | SHA-256 |
| --- | --- | ---: | --- |
| A: `F21C_完整工程与验证成果_本地交接候选.rar` | Complete local engineering handoff; not an upload-ready release | 28,648,385 bytes | `84AFA4EF777151B0D38562E2FF9F1A47AC081E7D3A577A526EE5DE2B5E806DD9` |
| B: `F21C_竞赛平台上传候选_最终验收.rar` | Competition-upload candidate | 325,352 bytes | `D191436A3A8B9F431EE61D2EF93C39E291270070B39E7148CB57093FA0D4B713` |

A competition portal submission receipt has not been confirmed. The candidate designation and archive integrity do not establish distribution permission or an actual submission.

The pre-exported HLS IP 2.0 and prebuilt BIT, XSA and ELF are not in this repository. The HLS IP was recognized and the full BD validated in a separate local environment; distribution of the generated IP and R2 runtime artifacts requires separate authorization. R2 remains TEST_ONLY; end-to-end reconstruction and board acceptance remain open. See [validation status](docs/VALIDATION_STATUS.md).

The optional `checkRequiredFiles` check in [recreate_review_project.tcl](candidate-b/validation/vivado/recreate_review_project.tcl) prepends `origin_dir` to an already normalized HLS IP path, potentially producing an incorrect doubled path. This is recorded for review; the frozen script has not been changed or rerun.
