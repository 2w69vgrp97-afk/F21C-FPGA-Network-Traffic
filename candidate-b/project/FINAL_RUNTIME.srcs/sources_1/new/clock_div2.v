`timescale 1ns / 1ps

module clock_div2 (
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 clk_in CLK" *)
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_RESET rstn,FREQ_HZ 5000000" *)
    input  wire clk_in,
    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 rstn RST" *)
    (* X_INTERFACE_PARAMETER = "POLARITY ACTIVE_LOW" *)
    input  wire rstn,
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 clk_out CLK" *)
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 2500000" *)
    output reg  clk_out
);

(* ASYNC_REG = "TRUE" *) reg [1:0] rst_sync = 2'b00;

always @(posedge clk_in) begin
    if (!rstn) begin
        rst_sync <= 2'b00;
    end else begin
        rst_sync <= {rst_sync[0], 1'b1};
    end
end

always @(posedge clk_in) begin
    if (!rst_sync[1]) begin
        clk_out <= 1'b0;
    end else begin
        clk_out <= ~clk_out;
    end
end

endmodule
