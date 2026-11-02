# AI-assisted engineering disclosure

This document describes classes of assistance used while preparing the F21C project documentation and internal engineering review. It is **not** an exported conversation log, a reproducibility proof or a claim that AI-generated output was used without human review.

## Documented workstreams

1. **Technical writing and communication:** drafting, editing and reviewing descriptions of the architecture, methods and test boundaries.
2. **Diagram review:** iterating system data-path and control-flow diagrams against known interfaces and source records; avoiding schematic details that cannot be backed by the implementation.
3. **Source-package release review:** checking delivery manifests, grouping candidate files, triaging sensitive-pattern alerts, documenting provenance, separating rights approval from technical readiness, and tracking missing rebuild dependencies.
4. **Tool troubleshooting:** recording unsuccessful Vivado/Vitis launch attempts without mislabeling them as synthesis or simulation failures.

## Quality and disclosure rules

- Maintain a distinction between historical logs, present measurements, and unverified statements.
- Use original source/logs to decide hardware-specific details; do not infer wiring, timing or FPGA resource behavior from prose alone.
- Never treat AI-written text as approval to redistribute third-party source/IP.
- Preserve experiment configuration, comparison conditions and revision identity when quoting any result.
- AI prompts and underlying project logs are not published in this repository; no fabricated prompt transcripts or evaluation screenshots are presented.

## Follow-up for competition evidence

If the event requires actual AI collaboration transcripts, prompt/response examples, correction trails or reusable skills, export and screen the real underlying records, associate them with human-reviewed deliverables, and seek approval before uploading.

An original **draft** review workflow is included at [../skill/fpga-release-audit/SKILL.md](../skill/fpga-release-audit/SKILL.md). Its presence does not mean it has passed an independent runtime test.
