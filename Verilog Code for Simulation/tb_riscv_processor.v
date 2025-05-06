`timescale 1 ns / 1 ps
module tb_riscv_processor;

    reg         ap_clk;
    reg         ap_rst;
    wire [31:0] dummy_out;  

    integer file, file_reg, i;

    top_module dut (
        .ap_clk(ap_clk),
        .ap_rst(ap_rst),
        .dummy_out(dummy_out)
    );

    // Clock generation (10 ns period)
    initial begin
        ap_clk = 0;
        forever #5 ap_clk = ~ap_clk;
    end

    // Reset generation: assert for 20 ns then deassert
    initial begin
        ap_rst = 1;
        #20;
        ap_rst = 0;
    end

    // Open file for dumping BRAM contents
    initial begin
        file = $fopen("dummy_out.txt", "w");
        if (file == 0) begin
            $display("Error: could not open file for writing.");
            $finish;
        end
    end

    // Open file for dumping register file contents
    initial begin
        file_reg = $fopen("reg_file.txt", "w");
        if (file_reg == 0) begin
            $display("Error: could not open reg_file.txt for writing.");
            $finish;
        end
    end

    // Dump BRAM and register file contents after simulation time
    initial begin
        #5000;  
        
        // Dump BRAM contents
        for (i = 0; i < 1024; i = i + 1) begin
            $fdisplay(file, "BRAM[%0d] = %h", i, dut.bram_inst.mem[i]);
        end
        $fclose(file);
        
        // Dump register file contents using the correct hierarchy:
        // The register file is inside the CPU (instance: cpu_inst) and is named reg_file_U.
        for (i = 0; i < 32; i = i + 1) begin
            $fdisplay(file_reg, "Register[%0d] = %h", i, dut.cpu_inst.reg_file_U.ram0[i]);
        end
        $fclose(file_reg);
        $finish;
    end

endmodule
