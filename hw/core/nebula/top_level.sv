`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Top Level
    DESCRIPTION: Instancia o Datapath e o Controlpath, conectando os
                 sinais de controle e expondo apenas as interfaces
                 padronizadas de Memória (IMEM e DMEM).
*/

module top_level #(
    parameter logic [31:0] BOOT_ADDR = 32'h0000_0000
) (
    input logic clk_i,
    input logic rst_ni,

    // ========================================================
    // Interface Externa: Instruction Memory (IMEM)
    // ========================================================
    output logic [31:0] imem_addr_o,
    input  logic [31:0] imem_rdata_i,

    // ========================================================
    // Interface Externa: Data Memory (DMEM)
    // ========================================================
    output logic [31:0] dmem_addr_o,
    output logic [31:0] dmem_wdata_o,
    output logic [ 3:0] dmem_be_o,
    output logic        dmem_we_o,
    input  logic [31:0] dmem_rdata_i
);

  import supernova_pkg::*;

  // ========================================================
  // FIOS INTERNOS (Cérebro <-> Músculos)
  // ========================================================
  // Fios de Feedback (Datapath -> Controlpath)
  logic       [ 6:0] opcode;
  logic       [ 2:0] funct3;
  logic              funct7_5;

  logic       [31:0] rs1_data;
  logic       [31:0] rs2_data;

  // Fios de Controle (Controlpath -> Datapath)
  logic              stall;
  logic              branch_valid;
  logic              jalr_sel;
  logic              reg_we;
  imm_type_e         imm_type;
  alu_src_a_e        alu_src_a;
  alu_src_b_e        alu_src_b;
  alu_op_e           alu_op;
  logic              mem_we;
  mem_size_e         mem_size;
  logic              mem_unsigned;
  wb_src_e           wb_src;

  // Exportar o Write Enable do Controlpath para fora do Top Level
  assign dmem_we_o = mem_we;

  // ========================================================
  // MÚSCULOS: DATAPATH
  // ========================================================
  datapath #(
      .BOOT_ADDR(BOOT_ADDR)
  ) u_datapath (
      .clk_i (clk_i),
      .rst_ni(rst_ni),

      // Memórias
      .instr_addr_o (imem_addr_o),
      .instr_rdata_i(imem_rdata_i),
      .dmem_addr_o  (dmem_addr_o),
      .dmem_wdata_o (dmem_wdata_o),
      .dmem_be_o    (dmem_be_o),
      .dmem_rdata_i (dmem_rdata_i),

      // Sinais de Controle (Entradas)
      .stall_i       (stall),
      .branch_valid_i(branch_valid),
      .jalr_sel_i    (jalr_sel),
      .reg_we_i      (reg_we),
      .imm_type_i    (imm_type),
      .alu_src_a_i   (alu_src_a),
      .alu_src_b_i   (alu_src_b),
      .alu_op_i      (alu_op),
      .mem_we_i      (mem_we),
      .mem_size_i    (mem_size),
      .mem_unsigned_i(mem_unsigned),
      .wb_src_i      (wb_src),

      // Sinais de Feedback (Saídas)
      .rs1_data_o(rs1_data),
      .rs2_data_o(rs2_data),
      .opcode_o  (opcode),
      .funct3_o  (funct3),
      .funct7_5_o(funct7_5)
  );

  // ========================================================
  // CÉREBRO: CONTROLPATH
  // ========================================================
  controlpath u_controlpath (
      // Feedback Fatiado (Entradas)
      .opcode_i  (opcode),
      .funct3_i  (funct3),
      .funct7_5_i(funct7_5),

      .rs1_data_i(rs1_data),
      .rs2_data_i(rs2_data),

      // Controle (Saídas)
      .stall_o       (stall),
      .branch_valid_o(branch_valid),
      .jalr_sel_o    (jalr_sel),
      .reg_we_o      (reg_we),
      .imm_type_o    (imm_type),
      .alu_src_a_o   (alu_src_a),
      .alu_src_b_o   (alu_src_b),
      .alu_op_o      (alu_op),
      .mem_we_o      (mem_we),
      .mem_size_o    (mem_size),
      .mem_unsigned_o(mem_unsigned),
      .wb_src_o      (wb_src)
  );

endmodule
