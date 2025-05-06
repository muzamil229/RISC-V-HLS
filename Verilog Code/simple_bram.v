`timescale 1 ns / 1 ps
module simple_bram #(
    parameter ADDR_WIDTH = 10,       // For 1024-depth memory
    parameter DATA_WIDTH = 32
)(
    input  wire                     clk,   // Clock signal
    input  wire                     rst,   // Reset signal
    input  wire                     ce,    // Chip enable
    input  wire                     we,    // Write enable
    input  wire [ADDR_WIDTH-1:0]    addr,  // Address input
    input  wire [DATA_WIDTH-1:0]    din,   // Data input
    output reg  [DATA_WIDTH-1:0]    dout   // Data output
);

    // Memory array: 2^ADDR_WIDTH deep, each DATA_WIDTH wide
    reg [DATA_WIDTH-1:0] mem [0:(1 << ADDR_WIDTH)-1];

    always @(posedge clk) begin
        if (rst) begin
            dout <= {DATA_WIDTH{1'b0}};
        end else if (ce) begin
            if (we)
                mem[addr] <= din;
            dout <= mem[addr];
        end
    end

endmodule
