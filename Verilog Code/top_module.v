`timescale 1 ns / 1 ps
module top_module (
    input  wire         ac701_clk_p,  // Differential clock positive
    input  wire         ac701_clk_n,  // Differential clock negative
    input  wire         ap_rst,       // System reset
    output wire [31:0]  dummy_out     // Dummy output to prevent empty design
);

    // Convert the differential clock to a single-ended clock
    wire ibufds_out;
    IBUFDS ibufds_inst (
        .I(ac701_clk_p),
        .IB(ac701_clk_n),
        .O(ibufds_out)
    );

    // Buffer the single-ended clock for global distribution
    wire ap_clk;
    BUFG bufg_inst (
        .I(ibufds_out),
        .O(ap_clk)
    );

    // Internal signals for memory interface
    wire [9:0]  mem_address0;
    wire        mem_ce0;
    wire        mem_we0;
    wire [31:0] mem_d0;
    wire [31:0] mem_q0;

    // Instantiate the CPU module (your generated CPU)
    cpu cpu_inst (
        .ap_clk(ap_clk),
        .ap_rst(ap_rst),
        .mem_address0(mem_address0),
        .mem_ce0(mem_ce0),
        .mem_we0(mem_we0),
        .mem_d0(mem_d0),
        .mem_q0(mem_q0)
    );

    // Instantiate the simple single-port BRAM module
    simple_bram #(
        .ADDR_WIDTH(10),  // 10-bit address (1024-depth)
        .DATA_WIDTH(32)   // 32-bit wide
    ) bram_inst (
        .clk(ap_clk),
        .rst(ap_rst),
        .addr(mem_address0),
        .ce(mem_ce0),
        .we(mem_we0),
        .din(mem_d0),
        .dout(mem_q0)
    );

    // Connect the internal BRAM output to a top-level output
    assign dummy_out = mem_q0;

endmodule
