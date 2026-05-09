`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Decode Stage (decode_stage)
    DESCRIPTION: Encapsulates the Register File and Immediate Generator.
                 Receives only the instruction payload (bits 31 to 7) to
                 avoid dangling opcode wires.
*/

module decode_stage (

    // Clock Input
    input logic clk_i,

    // Data Inputs
    input logic [31:7] instr_payload_i,
    input logic [31:0] rd_data_i,        // From Write-Back stage

    // Control Inputs
    input logic                     reg_we_i,
    input supernova_pkg::imm_type_e imm_type_i,

    // Data Outputs
    output logic [31:0] rs1_data_o,
    output logic [31:0] rs2_data_o,
    output logic [31:0] imm_o

);

  import supernova_pkg::*;

  // --------------------------------------------------------
  // Register File Instantiation
  // --------------------------------------------------------
  reg_file u_reg_file (
      .clk_i     (clk_i),
      .we_i      (reg_we_i),
      .rs1_addr_i(instr_payload_i[19:15]),
      .rs2_addr_i(instr_payload_i[24:20]),
      .rd_addr_i (instr_payload_i[11:7]),
      .rd_data_i (rd_data_i),

      .rs1_data_o(rs1_data_o),
      .rs2_data_o(rs2_data_o)
  );

  // --------------------------------------------------------
  // Immediate Generator Instantiation
  // --------------------------------------------------------
  // O imm_gen já foi projetado para receber os bits [31:7]
  imm_gen u_imm_gen (
      .instr_i   (instr_payload_i),
      .imm_type_i(imm_type_i),

      .imm_o(imm_o)
  );

endmodule
