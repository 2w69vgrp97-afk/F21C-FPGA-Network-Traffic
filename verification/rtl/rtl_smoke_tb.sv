`timescale 1ns / 1ps

// Standalone smoke checks for vendor-independent helper RTL.
// This testbench does not test the Ethernet MAC, IDelay primitive,
// Funnel-Sketch HLS logic, PS application, or complete board build.
module rtl_smoke_tb;
    reg clk = 1'b0;
    always #5 clk = ~clk;

    reg rstn = 1'b0;
    reg [1:0] speed_mode = 2'b10;
    reg ready0 = 1'b0;
    reg ready1 = 1'b1;
    reg [31:0] sink_data = 32'h12345678;
    reg [3:0] sink_keep = 4'hf;
    reg sink_valid = 1'b1;
    reg sink_last = 1'b1;

    wire [31:0] idle_data0, idle_data1;
    wire [3:0] idle_keep0, idle_keep1;
    wire idle_valid0, idle_valid1, idle_last0, idle_last1;
    wire sink_ready, half_clk, clk_en;

    integer failures = 0;
    integer i, low_count, toggles;
    reg previous_half;

    axis_idle_source_32 u_idle0 (
        .aclk(clk), .m_axis_tdata(idle_data0), .m_axis_tkeep(idle_keep0),
        .m_axis_tvalid(idle_valid0), .m_axis_tready(ready0),
        .m_axis_tlast(idle_last0)
    );
    axis_idle_source_32_100 u_idle1 (
        .aclk(clk), .m_axis_tdata(idle_data1), .m_axis_tkeep(idle_keep1),
        .m_axis_tvalid(idle_valid1), .m_axis_tready(ready1),
        .m_axis_tlast(idle_last1)
    );
    axis_sink_32 u_sink (
        .aclk(clk), .aresetn(rstn), .s_axis_tdata(sink_data),
        .s_axis_tkeep(sink_keep), .s_axis_tvalid(sink_valid),
        .s_axis_tready(sink_ready), .s_axis_tlast(sink_last)
    );
    clock_div2 u_div2 (.clk_in(clk), .rstn(rstn), .clk_out(half_clk));
    pl_axi_eth_clk_en u_enable (
        .clk(clk), .resetn(rstn), .speed_mode(speed_mode),
        .clk_en(clk_en)
    );

    task automatic check(input logic ok, input string message);
        if (ok !== 1'b1) begin
            $display("FAIL: %s at %0t", message, $time);
            failures = failures + 1;
        end
    endtask

    initial begin
        // In reset, idle sources must remain idle and the status sink
        // must withhold readiness.
        repeat (4) @(negedge clk);
        check(idle_data0 === 32'd0 && idle_keep0 === 4'd0 &&
              idle_valid0 === 1'b0 && idle_last0 === 1'b0,
              "idle source 0");
        check(idle_data1 === 32'd0 && idle_keep1 === 4'd0 &&
              idle_valid1 === 1'b0 && idle_last1 === 1'b0,
              "idle source 1");
        check(sink_ready === 1'b0, "sink not ready during reset");
        check(clk_en === 1'b1, "clock-enable reset state");

        rstn = 1'b1;
        repeat (12) @(negedge clk);
        check(sink_ready === 1'b1, "sink ready after reset");

        // The divider toggles its output every full input cycle in
        // steady state, yielding half the input clock frequency.
        toggles = 0;
        previous_half = half_clk;
        for (i = 0; i < 20; i = i + 1) begin
            @(negedge clk);
            if (half_clk !== previous_half) toggles = toggles + 1;
            previous_half = half_clk;
        end
        check(toggles == 20, "clock divider toggles each input cycle");

        // The Ethernet enable output is active LOW, per this module.
        // 1000M mode: enabled on every clock.
        for (i = 0; i < 20; i = i + 1) begin
            @(negedge clk);
            check(clk_en === 1'b0, "1000M enable each cycle");
        end

        // 100M mode: one active-low enable in every five clocks.
        speed_mode = 2'b01;
        repeat (12) @(negedge clk);
        low_count = 0;
        for (i = 0; i < 50; i = i + 1) begin
            @(negedge clk);
            if (clk_en === 1'b0) low_count = low_count + 1;
            else check(clk_en === 1'b1, "100M enable defined");
        end
        check(low_count == 10, "100M enable 10 pulses in 50 clocks");

        // 10M mode: one active-low enable in every fifty clocks.
        speed_mode = 2'b00;
        repeat (12) @(negedge clk);
        low_count = 0;
        for (i = 0; i < 100; i = i + 1) begin
            @(negedge clk);
            if (clk_en === 1'b0) low_count = low_count + 1;
            else check(clk_en === 1'b1, "10M enable defined");
        end
        check(low_count == 2, "10M enable 2 pulses in 100 clocks");

        // Unresolved link speed disables the clock enable.
        speed_mode = 2'b11;
        repeat (12) @(negedge clk);
        for (i = 0; i < 10; i = i + 1) begin
            @(negedge clk);
            check(clk_en === 1'b1, "unknown link speed disabled");
        end

        ready0 = 1'b1;
        ready1 = 1'b0;
        sink_valid = 1'b0;
        repeat (2) @(negedge clk);
        check(idle_valid0 === 1'b0 && idle_valid1 === 1'b0,
              "idle sources unaffected by READY");
        check(sink_ready === 1'b1, "sink ready independent of VALID");

        if (failures == 0) begin
            $display("PASS: vendor-independent RTL helper smoke checks");
            $finish;
        end else begin
            $fatal(1, "RTL smoke checks failed: %0d errors", failures);
        end
    end
endmodule
