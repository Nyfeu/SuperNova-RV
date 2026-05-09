`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Decode Stage (decode_stage)
    DESCRIPTION: Extracts register addresses, reads from the Register File,
                 generates the immediate, and receives the Write-Back data
                 from the end of the pipeline.
*/

module decode_stage (

    // Clock Input
    input logic clk_i,

    // Data Inputs
    input logic [31:7] instr_payload_i,

    // Write-Back Loop (End of Pipeline)
    input logic [ 4:0] wb_rd_addr_i,
    input logic [31:0] wb_rd_data_i,
    input logic        wb_reg_we_i,

    // Control Inputs
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
      .we_i      (wb_reg_we_i),
      .rs1_addr_i(instr_payload_i[19:15]),
      .rs2_addr_i(instr_payload_i[24:20]),

      // The WB address and data are coming from the end of the pipeline,
      // so we need to connect them to the Register File for writing back the results.
      .rd_addr_i(wb_rd_addr_i),
      .rd_data_i(wb_rd_data_i),

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
