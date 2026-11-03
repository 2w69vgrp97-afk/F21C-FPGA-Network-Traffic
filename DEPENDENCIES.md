# Dependencies

- **Included in V3.** The [frozen tree](candidate-b/) includes custom RTL, HLS source and headers, functional testbenches, the PS application and build inputs, XPR, BD, XCI, XDC and build scripts.
- **Toolchain.** AMD Vivado, Vitis and Vitis HLS 2025.2 with Zynq-7000 support for `xc7z020clg484-2`, targeting AC7021B. The standalone RTL checks use Python, Icarus Verilog and Yosys.
- **HLS IP.** The BD and XCI reference `xilinx.com:hls:fs_hls_test:2.0`. The pre-exported IP directory is available for local review and its distribution rights are pending. The [build guide](docs/REPRODUCTION_GUIDE.md) describes the export path from the included source.
- **Generated platform and software.** Use a matching Vivado hardware handoff XSA to generate the Cortex-A9 standalone platform, BSP, `xparameters.h`, DMA/Ethernet/HLS drivers and PS ELF. Generated AMD catalog IP outputs are produced by the installed tools.
- **Ethernet licensing.** AXI Ethernet 8.0 and its TEMAC dependencies require applicable AMD rights. The local Vivado IP status reported hardware evaluation for Ethernet/TEMAC. Any production use or distribution of evaluation images requires separate confirmation. The project source submission authorization and generated IP distribution rights are separate.
