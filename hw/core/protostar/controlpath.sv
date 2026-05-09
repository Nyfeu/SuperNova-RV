/*
    PROJECT: SuperNova-RV
    MODULE: Controlpath
    DESCRIPTION: O cérebro do processador. Encapsula o decodificador de
                 instruções e a unidade de branch, gerando todos os sinais
                 de controle para o Datapath.
*/

`default_nettype none

module controlpath (
    // Entradas vindas do Datapath (Instrução Fatiada)
    input logic [6:0] opcode_i,
    input logic [2:0] funct3_i,
    input logic       funct7_5_i,

    input logic [31:0] rs1_data_i,
    input logic [31:0] rs2_data_i,

    // Saídas de Controle para o Datapath
    output logic                      stall_o,
    output logic                      branch_valid_o,
    output logic                      jalr_sel_o,
    output logic                      reg_we_o,
    output supernova_pkg::imm_type_e  imm_type_o,
    output supernova_pkg::alu_src_a_e alu_src_a_o,
    output supernova_pkg::alu_src_b_e alu_src_b_o,
    output supernova_pkg::alu_op_e    alu_op_o,
    output logic                      mem_we_o,
    output supernova_pkg::mem_size_e  mem_size_o,
    output logic                      mem_unsigned_o,
    output supernova_pkg::wb_src_e    wb_src_o
);

  import supernova_pkg::*;

  // Sinais internos de interconexão
  logic is_branch;
  logic is_jump;
  logic branch_taken;

  // ========================================================
  // DECODIFICADOR DE INSTRUÇÕES
  // ========================================================
  instr_decoder u_decoder (
      .opcode_i      (opcode_i),
      .funct3_i      (funct3_i),
      .funct7_5_i    (funct7_5_i),
      .reg_we_o      (reg_we_o),
      .imm_type_o    (imm_type_o),
      .alu_src_a_o   (alu_src_a_o),
      .alu_src_b_o   (alu_src_b_o),
      .alu_op_o      (alu_op_o),
      .mem_we_o      (mem_we_o),
      .mem_size_o    (mem_size_o),
      .mem_unsigned_o(mem_unsigned_o),
      .wb_src_o      (wb_src_o),
      .is_branch_o   (is_branch),
      .is_jump_o     (is_jump)
  );

  // ========================================================
  // UNIDADE DE BRANCH
  // ========================================================
  branch_unit u_branch_unit (
      .rs1_data_i    (rs1_data_i),
      .rs2_data_i    (rs2_data_i),
      .is_branch_i   (is_branch),
      .funct3_i      (funct3_i),
      .branch_taken_o(branch_taken)
  );

  // ========================================================
  // LÓGICA DE RESOLUÇÃO DE FLUXO (PC)
  // ========================================================
  assign branch_valid_o = is_jump || branch_taken;
  assign jalr_sel_o     = (opcode_i == OpcodeJalr);
  assign stall_o        = 1'b0;

endmodule
