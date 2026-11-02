`timescale 1ns / 1ps

module axis_idle_source_32_100 (
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 aclk CLK" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF m_axis,FREQ_HZ 100000000" *)
    input  wire        aclk,
    output wire [31:0] m_axis_tdata,
    output wire [3:0]  m_axis_tkeep,
    output wire        m_axis_tvalid,
    input  wire        m_axis_tready,
    output wire        m_axis_tlast
);

assign m_axis_tdata  = 32'd0;
assign m_axis_tkeep  = 4'd0;
assign m_axis_tvalid = 1'b0;
assign m_axis_tlast  = 1'b0;

endmodule
