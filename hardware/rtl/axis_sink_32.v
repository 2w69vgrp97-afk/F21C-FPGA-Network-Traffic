module axis_sink_32 (
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF s_axis,FREQ_HZ 100000000" *)
    input  wire        aclk,
    input  wire        aresetn,

    input  wire [31:0] s_axis_tdata,
    input  wire [3:0]  s_axis_tkeep,
    input  wire        s_axis_tvalid,
    output wire        s_axis_tready,
    input  wire        s_axis_tlast
);

// AXI Ethernet exposes a receive-status AXI stream. The status words must be
// consumed, otherwise the MAC/buffer can stop completing receive frames.
assign s_axis_tready = aresetn;

endmodule
