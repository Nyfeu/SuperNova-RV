/*
    PROJECT: SuperNova-RV
    MODULE: Instruction Fetch Stage (fetch_stage)
    DESCRIPTION:
        First stage of the processor pipeline. Responsible for calculating
        the next sequential PC (PC + 4), handling branch redirections,
        and exposing the memory fetch interface.
        Instantiates the pc_reg module for state retention.

    AUTHOR: André Solano Ferreira Rodrigues Maiolini
    DATE: 2026-04-30
*/

`default_nettype none

module fetch_stage #(

    // Reset address where the CPU starts executing code
    parameter logic [31:0] BOOT_ADDR = 32'h0000_0000

) (

    input wire logic clk_i,
    input wire logic rst_ni,

    // Pipeline Control
    input wire logic stall_i,

    // Branch Control (From Execute/Branch Unit)
    input wire logic        branch_valid_i,
    input wire logic [31:0] branch_target_i,

    // Instruction Memory Interface (External Harvard Bus)
    output logic      [31:0] instr_addr_o,
    input  wire logic [31:0] instr_rdata_i,

    // Outputs to Decode Stage
    output logic [31:0] pc_o,
    output logic [31:0] pc_next_o,
    output logic [31:0] instr_o

);

  // Internal wires
  logic [31:0] pc_q;
  logic [31:0] next_pc;

  // --------------------------------------------------------
  // Program Counter Register Instantiation
  // --------------------------------------------------------

  pc_reg #(
      .RESET_VECTOR(BOOT_ADDR)
  ) u_pc_reg (
      .clk_i    (clk_i),
      .rst_ni   (rst_ni),
      .stall_i  (stall_i),
      .pc_next_i(next_pc),
      .pc_o     (pc_q)
  );

  // --------------------------------------------------------
  // Combinational Logic: Next PC Calculation
  // --------------------------------------------------------

  // The RISC-V ISA is byte-addressed, and standard instructions are 32-bit (4 bytes).
  assign pc_next_o = pc_q + 32'd4;

  always_comb begin
    // Multiplexer to select between a taken branch or sequential execution
    if (branch_valid_i) begin
      next_pc = branch_target_i;
    end else begin
      next_pc = pc_next_o;
    end
  end

  // --------------------------------------------------------
  // Output Assignments
  // --------------------------------------------------------

  assign pc_o         = pc_q;
  assign instr_addr_o = pc_q;  // Expose current PC to the memory
  assign instr_o      = instr_rdata_i;  // Route fetched data down the pipeline

endmodule
