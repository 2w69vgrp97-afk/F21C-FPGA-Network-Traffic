#!/usr/bin/env python3
"""Run a small iverilog/vvp smoke test of standalone RTL helpers.

This is not a Vivado build and does not validate the whole FPGA project.
"""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
SOURCES = [
    "hardware/rtl/axis_idle_source_32.v",
    "hardware/rtl/axis_idle_source_32_100.v",
    "hardware/rtl/axis_sink_32.v",
    "hardware/rtl/clock_div2.v",
    "hardware/rtl/pl_axi_eth_clk_en.v",
]


def main() -> int:
    missing = [str(ROOT / path) for path in SOURCES if not (ROOT / path).is_file()]
    if missing:
        print("Missing RTL sources:\n" + "\n".join(missing), file=sys.stderr)
        return 2

    iverilog = shutil.which("iverilog")
    vvp = shutil.which("vvp")
    if not iverilog or not vvp:
        print(
            "RTL smoke test NOT RUN: install Icarus Verilog (iverilog and vvp), "
            "then rerun this script.",
            file=sys.stderr,
        )
        return 2

    with tempfile.TemporaryDirectory(prefix="f21c_rtl_smoke_") as directory:
        output = Path(directory) / "rtl_smoke.vvp"
        command = [
            iverilog, "-g2012", "-s", "rtl_smoke_tb",
            "-o", str(output), str(HERE / "rtl_smoke_tb.sv"),
            *(str(ROOT / path) for path in SOURCES),
        ]
        compiled = subprocess.run(command, check=False)
        if compiled.returncode:
            print("RTL smoke test compile FAILED", file=sys.stderr)
            return compiled.returncode
        executed = subprocess.run([vvp, str(output)], check=False)
        if executed.returncode:
            print("RTL smoke test execution FAILED", file=sys.stderr)
        return executed.returncode


if __name__ == "__main__":
    sys.exit(main())
