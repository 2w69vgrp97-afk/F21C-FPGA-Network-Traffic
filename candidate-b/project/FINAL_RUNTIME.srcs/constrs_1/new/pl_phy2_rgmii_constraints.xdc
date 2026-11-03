# AX7021B PL Ethernet PHY5 pin constraints
# Source: user-provided AX7021B PHY5 RGMII pin table.
# This file only covers pin assignment and basic IO standards.
# RGMII timing constraints still need board-level timing closure work if you later target line-rate traffic.
set_property PACKAGE_PIN AB15 [get_ports pl_phy2_rgmii_txc]
set_property PACKAGE_PIN V13 [get_ports {pl_phy2_rgmii_td[0]}]
set_property PACKAGE_PIN W13 [get_ports {pl_phy2_rgmii_td[1]}]
set_property PACKAGE_PIN Y13 [get_ports {pl_phy2_rgmii_td[2]}]
set_property PACKAGE_PIN AA13 [get_ports {pl_phy2_rgmii_td[3]}]
set_property PACKAGE_PIN AB14 [get_ports pl_phy2_rgmii_tx_ctl]

set_property PACKAGE_PIN W17 [get_ports pl_phy2_rgmii_rxc]
set_property PACKAGE_PIN AB16 [get_ports {pl_phy2_rgmii_rd[0]}]
set_property PACKAGE_PIN AA16 [get_ports {pl_phy2_rgmii_rd[1]}]
set_property PACKAGE_PIN Y15 [get_ports {pl_phy2_rgmii_rd[2]}]
set_property PACKAGE_PIN W15 [get_ports {pl_phy2_rgmii_rd[3]}]
set_property PACKAGE_PIN W18 [get_ports pl_phy2_rgmii_rx_ctl]

set_property PACKAGE_PIN Y18 [get_ports pl_phy2_mdio_mdc]
set_property PACKAGE_PIN AA18 [get_ports pl_phy2_mdio_mdio_io]
set_property PACKAGE_PIN V19 [get_ports {pl_phy2_reset_n[0]}]

set_property IOSTANDARD LVCMOS33 [get_ports pl_phy2_rgmii_txc]
set_property IOSTANDARD LVCMOS33 [get_ports {pl_phy2_rgmii_td[*]}]
set_property IOSTANDARD LVCMOS33 [get_ports pl_phy2_rgmii_tx_ctl]
set_property IOSTANDARD LVCMOS33 [get_ports pl_phy2_rgmii_rxc]
set_property IOSTANDARD LVCMOS33 [get_ports {pl_phy2_rgmii_rd[*]}]
set_property IOSTANDARD LVCMOS33 [get_ports pl_phy2_rgmii_rx_ctl]
set_property IOSTANDARD LVCMOS33 [get_ports pl_phy2_mdio_mdc]
set_property IOSTANDARD LVCMOS33 [get_ports pl_phy2_mdio_mdio_io]
set_property IOSTANDARD LVCMOS33 [get_ports {pl_phy2_reset_n[0]}]

set_property SLEW FAST [get_ports pl_phy2_rgmii_txc]
set_property SLEW FAST [get_ports {pl_phy2_rgmii_td[*]}]
set_property SLEW FAST [get_ports pl_phy2_rgmii_tx_ctl]
set_property DRIVE 12 [get_ports pl_phy2_rgmii_txc]
set_property DRIVE 12 [get_ports {pl_phy2_rgmii_td[*]}]
set_property DRIVE 12 [get_ports pl_phy2_rgmii_tx_ctl]
set_property PULLTYPE PULLUP [get_ports pl_phy2_mdio_mdio_io]


