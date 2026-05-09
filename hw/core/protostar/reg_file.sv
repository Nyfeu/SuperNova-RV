`default_nettype none

/*
    PROJECT: RISC-V CPU Core

    MODULE: Register File (reg_file)
    DESCRIPTION: Register file that stores temporary data during CPU execution.
                 Features internal bypass (Write-First Forwarding) to resolve
                 structural hazards in pipelined architectures.

    AUTHOR: André Solano Ferreira Rodrigues Maiolini
    DATE: 2026-04-30

    PARAMETERS:
    - WIDTH: Data bus width (default 32 bits for RV32I).

    INPUTS:
    - clk_i: Input clock (rising edge transition)
    - we_i: Write Enable (Enables write to destination register)
    - rs1_addr_i: Address of source register 1
    - rs2_addr_i: Address of source register 2
    - rd_addr_i: Address of destination register
    - rd_data_i: Data to be written to destination register

    OUTPUTS:
    - rs1_data_o: Data read from source port 1
    - rs2_data_o: Data read from source port 2

    BEHAVIOR:
    - Combinational read with internal forwarding.
    - Synchronous write on the rising edge of the clock.
    - Register x0 (address 0) is hardwired to zero, ignoring writes.

*/

module reg_file #(

    parameter int WIDTH = 32

) (

    input logic clk_i,
    input logic we_i,

    input logic [      4:0] rs1_addr_i,
    input logic [      4:0] rs2_addr_i,
    input logic [      4:0] rd_addr_i,
    input logic [WIDTH-1:0] rd_data_i,

    output logic [WIDTH-1:0] rs1_data_o,
    output logic [WIDTH-1:0] rs2_data_o

);

  // Internal array of 32 registers
  logic [WIDTH-1:0] registers[32];

  // --------------------------------------------------------
  // Write Logic (Synchronous)
  // --------------------------------------------------------

  always_ff @(posedge clk_i) begin
    // Writes only if Enable is active and it's not register x0
    if (we_i && (rd_addr_i != 5'b00000)) begin
      registers[rd_addr_i] <= rd_data_i;
    end
  end

  // --------------------------------------------------------
  // Read Logic (Combinational with Internal Bypass)
  // --------------------------------------------------------

  always_comb begin
    // Returns 0 if address is 0.
    // If reading the same register that is currently being written,
    // bypass the old array value and forward the new data directly.
    // Otherwise, read normally from the array.

    rs1_data_o = (rs1_addr_i == 5'b00000) ? '0 :
                 (we_i && (rd_addr_i == rs1_addr_i)) ? rd_data_i :
                 registers[rs1_addr_i];

    rs2_data_o = (rs2_addr_i == 5'b00000) ? '0 :
                 (we_i && (rd_addr_i == rs2_addr_i)) ? rd_data_i :
                 registers[rs2_addr_i];
  end

endmodule
