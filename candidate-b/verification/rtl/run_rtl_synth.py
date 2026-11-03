#!/usr/bin/env python3
"""Run generic Yosys synthesis checks for standalone, vendor-free RTL helpers.

Scope: checks frontend parsing, elaboration, generic synthesis and structural
warnings only. Does not perform AMD Vivado synthesis, timing, CDC verification
or test vendor-specific IDELAYCTRL / Ethernet IP.
"""
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
RTL = ROOT / "project/FINAL_RUNTIME.srcs/sources_1/new"
TOPS = (
    "axis_idle_source_32",
    "axis_idle_source_32_100",
    "axis_sink_32",
    "clock_div2",
    "pl_axi_eth_clk_en",
)


def main() -> int:
    yosys = shutil.which("yosys")
    if yosys is None:
        print("NOT RUN: Yosys was not found on PATH.", file=sys.stderr)
        return 2

    for top in TOPS:
        source = RTL / f"{top}.v"
        if not source.is_file():
            print(f"MISSING: {source}", file=sys.stderr)
            return 2

        script = (
            f"read_verilog -sv {source.as_posix()}; "
            f"hierarchy -check -top {top}; "
            f"synth -top {top}; "
            "check -assert; stat"
        )
        print(f"GENERIC SYNTH: {top}", flush=True)
        result = subprocess.run(
            [yosys, "-Q", "-T", "-p", script],
            check=False,
        )
        if result.returncode:
            print(f"FAIL: generic synthesis of {top}", file=sys.stderr)
            return result.returncode

    print(f"PASS: Yosys generic synthesis and structural checks ({len(TOPS)} helper RTL modules)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
