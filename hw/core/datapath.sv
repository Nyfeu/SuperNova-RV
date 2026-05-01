`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Datapath
    DESCRIPTION: The structural core of the processor. Instantiates and wires
                 together the 5 logical stages: Fetch, Decode, Execute,
                 Memory, and Write-Back.
*/

module datapath #(
    parameter logic [31:0] BOOT_ADDR = 32'h0000_0000
) (
    input logic clk_i,
    input logic rst_ni,

    // ----------------------------------------------------
    // Instruction Memory (IMEM) Interface
    // ----------------------------------------------------
    output logic [31:0] instr_addr_o,
    input  logic [31:0] instr_rdata_i,

    // ----------------------------------------------------
    // Data Memory (DMEM) Interface
    // ----------------------------------------------------
    output logic [31:0] dmem_addr_o,
    output logic [31:0] dmem_wdata_o,
    output logic [ 3:0] dmem_be_o,
    input  logic [31:0] dmem_rdata_i,

    // ----------------------------------------------------
    // Control Unit Signals (Inputs to Datapath)
    // ----------------------------------------------------
    // Pipeline & Flow Control
    input logic stall_i,
    input logic branch_valid_i,
    input logic jalr_sel_i,      // 1: JALR Target, 0: Branch/JAL Target

    // Decode Control
    input logic                     reg_we_i,
    input supernova_pkg::imm_type_e imm_type_i,

    // Execute Control
    input  supernova_pkg::alu_src_a_e alu_src_a_i,
    input  supernova_pkg::alu_src_b_e alu_src_b_i,
    input  supernova_pkg::alu_op_e    alu_op_i,

    // Memory Control
    input logic                     mem_we_i,
    input supernova_pkg::mem_size_e mem_size_i,
    input logic                     mem_unsigned_i,

    // Write-Back Control
    input supernova_pkg::wb_src_e wb_src_i,

    // ----------------------------------------------------
    // Branch Unit Interface (Exports to external BU)
    // ----------------------------------------------------
    output logic [31:0] rs1_data_o,
    output logic [31:0] rs2_data_o,

    // ----------------------------------------------------
    // Decoder Interface (Exports to external Control Unit)
    // ----------------------------------------------------
    output logic [6:0] opcode_o,
    output logic [2:0] funct3_o,
    output logic       funct7_5_o
);

  import supernova_pkg::*;

  // ========================================================
  // INTERNAL WIRES (Os "Barramentos" entre os estágios)
  // ========================================================

  // Fetch Wires
  logic [31:0] pc;
  logic [31:0] pc_next;
  logic [31:0] instr;
  logic [31:0] branch_target;

  // Decode Wires
  logic [31:0] rs1_data;
  logic [31:0] rs2_data;
  logic [31:0] imm;

  // Execute Wires
  logic [31:0] alu_result;
  logic [31:0] target_addr;
  logic [31:0] jalr_addr;

  // Memory Wires
  logic [31:0] lsu_rdata;

  // Write-Back Wires
  logic [31:0] wb_data;

  // ========================================================
  // ROUTING LOGIC
  // ========================================================

  // Fatiamento e exportação dos sinais de controle para o Controlpath
  assign opcode_o = instr[6:0];
  assign funct3_o = instr[14:12];
  assign funct7_5_o = instr[30];

  // Multiplexador para escolher o endereço do salto (JALR vs Branch/JAL)
  assign branch_target = jalr_sel_i ? jalr_addr : target_addr;

  // Exportar os registradores lidos para a Unidade de Branch externa
  assign rs1_data_o = rs1_data;
  assign rs2_data_o = rs2_data;

  // ========================================================
  // STAGE INSTANTIATIONS
  // ========================================================

  // 1. FETCH STAGE
  fetch_stage #(
      .BOOT_ADDR(BOOT_ADDR)
  ) u_fetch_stage (
      .clk_i          (clk_i),
      .rst_ni         (rst_ni),
      .stall_i        (stall_i),
      .branch_valid_i (branch_valid_i),
      .branch_target_i(branch_target),

      .instr_addr_o (instr_addr_o),
      .instr_rdata_i(instr_rdata_i),

      .pc_o     (pc),
      .pc_next_o(pc_next),
      .instr_o  (instr)
  );

  // 2. DECODE STAGE
  decode_stage u_decode_stage (
      .clk_i          (clk_i),
      .instr_payload_i(instr[31:7]),  // Fatiamento cirúrgico!
      .rd_data_i      (wb_data),

      .reg_we_i  (reg_we_i),
      .imm_type_i(imm_type_i),

      .rs1_data_o(rs1_data),
      .rs2_data_o(rs2_data),
      .imm_o     (imm)
  );

  // 3. EXECUTE STAGE
  execute_stage u_execute_stage (
      .pc_i      (pc),
      .rs1_data_i(rs1_data),
      .rs2_data_i(rs2_data),
      .imm_i     (imm),

      .alu_src_a_i(alu_src_a_i),
      .alu_src_b_i(alu_src_b_i),
      .alu_op_i   (alu_op_i),

      .alu_result_o (alu_result),
      .target_addr_o(target_addr),
      .jalr_addr_o  (jalr_addr)
  );

  // 4. MEMORY STAGE
  memory_stage u_memory_stage (
      .alu_result_i(alu_result),
      .rs2_data_i  (rs2_data),

      .mem_we_i      (mem_we_i),
      .mem_size_i    (mem_size_i),
      .mem_unsigned_i(mem_unsigned_i),

      .dmem_rdata_i(dmem_rdata_i),
      .dmem_addr_o (dmem_addr_o),
      .dmem_wdata_o(dmem_wdata_o),
      .dmem_be_o   (dmem_be_o),

      .lsu_rdata_o(lsu_rdata)
  );

  // 5. WRITE-BACK STAGE
  writeback_stage u_writeback_stage (
      .alu_result_i(alu_result),
      .lsu_rdata_i (lsu_rdata),
      .pc_plus_4_i (pc_next),     // pc_next do Fetch é exatamente PC + 4

      .wb_src_i(wb_src_i),

      .wb_data_o(wb_data)
  );

endmodule
