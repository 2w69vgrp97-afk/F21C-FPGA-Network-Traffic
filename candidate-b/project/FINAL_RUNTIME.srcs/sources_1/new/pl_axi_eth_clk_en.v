`timescale 1ns / 1ps

module pl_axi_eth_clk_en (
    input  wire       clk,
    input  wire       resetn,
    input  wire [1:0] speed_mode,
    output reg        clk_en
);
    reg [5:0] div_cnt = 6'd0;
    reg [1:0] speed_q = 2'b10;
    (* ASYNC_REG = "TRUE" *) reg [1:0] speed_meta = 2'b10;
    (* ASYNC_REG = "TRUE" *) reg [1:0] reset_sync = 2'b00;

    always @(posedge clk) begin
        if (!resetn) begin
            reset_sync <= 2'b00;
        end else begin
            reset_sync <= {reset_sync[0], 1'b1};
        end
    end

    always @(posedge clk) begin
        if (!reset_sync[1]) begin
            div_cnt <= 6'd0;
            speed_meta <= 2'b10;
            speed_q <= 2'b10;
            clk_en  <= 1'b1;
        end else begin
            speed_meta <= speed_mode;
            speed_q <= speed_meta;

            case (speed_q)
                2'b10: begin
                    // AXI Ethernet 7.1 marks RX_CLK_ENABLE_IN as active-low.
                    // 1000M GMII: MAC runs every 125 MHz clock.
                    div_cnt <= 6'd0;
                    clk_en  <= 1'b0;
                end
                2'b01: begin
                    // 100M MII: active-low enable one cycle out of five.
                    if (div_cnt == 6'd4) begin
                        div_cnt <= 6'd0;
                        clk_en  <= 1'b0;
                    end else begin
                        div_cnt <= div_cnt + 6'd1;
                        clk_en  <= 1'b1;
                    end
                end
                2'b00: begin
                    // 10M MII: active-low enable one cycle out of fifty.
                    if (div_cnt == 6'd49) begin
                        div_cnt <= 6'd0;
                        clk_en  <= 1'b0;
                    end else begin
                        div_cnt <= div_cnt + 6'd1;
                        clk_en  <= 1'b1;
                    end
                end
                default: begin
                    // No resolved link speed.
                    div_cnt <= 6'd0;
                    clk_en  <= 1'b1;
                end
            endcase
        end
    end
endmodule
