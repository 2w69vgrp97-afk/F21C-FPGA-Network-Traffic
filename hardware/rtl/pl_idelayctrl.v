`timescale 1ns / 1ps

module pl_idelayctrl (
    input  wire refclk,
    input  wire resetn,
    input  wire locked,
    output wire rdy
);

wire idelay_rst = (~resetn) | (~locked);

(* IODELAY_GROUP = "gpr1" *)
IDELAYCTRL #(
    .SIM_DEVICE("7SERIES")
) idelayctrl_inst (
    .RDY(rdy),
    .REFCLK(refclk),
    .RST(idelay_rst)
);

endmodule
