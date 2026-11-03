//Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
//Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2025.2 (win64) Build 6299465 Fri Nov 14 19:35:11 GMT 2025
//Date        : Wed Sep 30 18:20:00 2026
//Command     : generate_target FS_Design_wrapper.bd
//Design      : FS_Design_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module FS_Design_wrapper
   (DDR_addr,
    DDR_ba,
    DDR_cas_n,
    DDR_ck_n,
    DDR_ck_p,
    DDR_cke,
    DDR_cs_n,
    DDR_dm,
    DDR_dq,
    DDR_dqs_n,
    DDR_dqs_p,
    DDR_odt,
    DDR_ras_n,
    DDR_reset_n,
    DDR_we_n,
    FIXED_IO_ddr_vrn,
    FIXED_IO_ddr_vrp,
    FIXED_IO_mio,
    FIXED_IO_ps_clk,
    FIXED_IO_ps_porb,
    FIXED_IO_ps_srstb,
    pl_phy2_mdio_mdc,
    pl_phy2_mdio_mdio_io,
    pl_phy2_reset_n,
    pl_phy2_rgmii_rd,
    pl_phy2_rgmii_rx_ctl,
    pl_phy2_rgmii_rxc,
    pl_phy2_rgmii_td,
    pl_phy2_rgmii_tx_ctl,
    pl_phy2_rgmii_txc);
  inout [14:0]DDR_addr;
  inout [2:0]DDR_ba;
  inout DDR_cas_n;
  inout DDR_ck_n;
  inout DDR_ck_p;
  inout DDR_cke;
  inout DDR_cs_n;
  inout [3:0]DDR_dm;
  inout [31:0]DDR_dq;
  inout [3:0]DDR_dqs_n;
  inout [3:0]DDR_dqs_p;
  inout DDR_odt;
  inout DDR_ras_n;
  inout DDR_reset_n;
  inout DDR_we_n;
  inout FIXED_IO_ddr_vrn;
  inout FIXED_IO_ddr_vrp;
  inout [53:0]FIXED_IO_mio;
  inout FIXED_IO_ps_clk;
  inout FIXED_IO_ps_porb;
  inout FIXED_IO_ps_srstb;
  output pl_phy2_mdio_mdc;
  inout pl_phy2_mdio_mdio_io;
  output [0:0]pl_phy2_reset_n;
  input [3:0]pl_phy2_rgmii_rd;
  input pl_phy2_rgmii_rx_ctl;
  input pl_phy2_rgmii_rxc;
  output [3:0]pl_phy2_rgmii_td;
  output pl_phy2_rgmii_tx_ctl;
  output pl_phy2_rgmii_txc;

  wire [14:0]DDR_addr;
  wire [2:0]DDR_ba;
  wire DDR_cas_n;
  wire DDR_ck_n;
  wire DDR_ck_p;
  wire DDR_cke;
  wire DDR_cs_n;
  wire [3:0]DDR_dm;
  wire [31:0]DDR_dq;
  wire [3:0]DDR_dqs_n;
  wire [3:0]DDR_dqs_p;
  wire DDR_odt;
  wire DDR_ras_n;
  wire DDR_reset_n;
  wire DDR_we_n;
  wire FIXED_IO_ddr_vrn;
  wire FIXED_IO_ddr_vrp;
  wire [53:0]FIXED_IO_mio;
  wire FIXED_IO_ps_clk;
  wire FIXED_IO_ps_porb;
  wire FIXED_IO_ps_srstb;
  wire pl_phy2_mdio_mdc;
  wire pl_phy2_mdio_mdio_i;
  wire pl_phy2_mdio_mdio_io;
  wire pl_phy2_mdio_mdio_o;
  wire pl_phy2_mdio_mdio_t;
  wire [0:0]pl_phy2_reset_n;
  wire [3:0]pl_phy2_rgmii_rd;
  wire pl_phy2_rgmii_rx_ctl;
  wire pl_phy2_rgmii_rxc;
  wire [3:0]pl_phy2_rgmii_td;
  wire pl_phy2_rgmii_tx_ctl;
  wire pl_phy2_rgmii_txc;

  FS_Design FS_Design_i
       (.DDR_addr(DDR_addr),
        .DDR_ba(DDR_ba),
        .DDR_cas_n(DDR_cas_n),
        .DDR_ck_n(DDR_ck_n),
        .DDR_ck_p(DDR_ck_p),
        .DDR_cke(DDR_cke),
        .DDR_cs_n(DDR_cs_n),
        .DDR_dm(DDR_dm),
        .DDR_dq(DDR_dq),
        .DDR_dqs_n(DDR_dqs_n),
        .DDR_dqs_p(DDR_dqs_p),
        .DDR_odt(DDR_odt),
        .DDR_ras_n(DDR_ras_n),
        .DDR_reset_n(DDR_reset_n),
        .DDR_we_n(DDR_we_n),
        .FIXED_IO_ddr_vrn(FIXED_IO_ddr_vrn),
        .FIXED_IO_ddr_vrp(FIXED_IO_ddr_vrp),
        .FIXED_IO_mio(FIXED_IO_mio),
        .FIXED_IO_ps_clk(FIXED_IO_ps_clk),
        .FIXED_IO_ps_porb(FIXED_IO_ps_porb),
        .FIXED_IO_ps_srstb(FIXED_IO_ps_srstb),
        .pl_phy2_mdio_mdc(pl_phy2_mdio_mdc),
        .pl_phy2_mdio_mdio_i(pl_phy2_mdio_mdio_i),
        .pl_phy2_mdio_mdio_o(pl_phy2_mdio_mdio_o),
        .pl_phy2_mdio_mdio_t(pl_phy2_mdio_mdio_t),
        .pl_phy2_reset_n(pl_phy2_reset_n),
        .pl_phy2_rgmii_rd(pl_phy2_rgmii_rd),
        .pl_phy2_rgmii_rx_ctl(pl_phy2_rgmii_rx_ctl),
        .pl_phy2_rgmii_rxc(pl_phy2_rgmii_rxc),
        .pl_phy2_rgmii_td(pl_phy2_rgmii_td),
        .pl_phy2_rgmii_tx_ctl(pl_phy2_rgmii_tx_ctl),
        .pl_phy2_rgmii_txc(pl_phy2_rgmii_txc));
  IOBUF pl_phy2_mdio_mdio_iobuf
       (.I(pl_phy2_mdio_mdio_o),
        .IO(pl_phy2_mdio_mdio_io),
        .O(pl_phy2_mdio_mdio_i),
        .T(pl_phy2_mdio_mdio_t));
endmodule
