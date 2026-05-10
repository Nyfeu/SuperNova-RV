/*
    PROJECT: SuperNova-RV
    MODULE: Controlpath
    DESCRIPTION: O cérebro do processador. No modelo Pipeline, ele atua apenas
                 no estágio de Decode, decodificando a instrução e repassando
                 as flags (is_branch, is_jump) para o Datapath resolver o fluxo.
*/

`default_nettype none

module controlpath (
    // Entradas vindas do Datapath (Instrução Fatiada no estágio IF/ID)
    input logic [6:0] opcode_i,
    input logic [2:0] funct3_i,
    input logic       funct7_5_i,

    // Note que removemos rs1_data_i e rs2_data_i daqui!

    // Saídas de Controle para o Datapath
    output logic                      stall_o,
    output logic                      is_branch_o,
    output logic                      is_jump_o,
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

      // Ligamos os fios internos do decoder direto para fora do Controlpath
      .is_branch_o(is_branch_o),
      .is_jump_o  (is_jump_o)

  );

  // ========================================================
  // LÓGICA FIXA
  // ========================================================
  assign jalr_sel_o = (opcode_i == OpcodeJalr);
  assign stall_o    = 1'b0;

endmodule
