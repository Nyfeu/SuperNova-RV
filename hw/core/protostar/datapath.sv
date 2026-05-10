`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Datapath
    DESCRIPTION: The structural core of the processor. Instantiates and wires
                 together the 5 logical stages: Fetch, Decode, Execute,
                 Memory, and Write-Back, including pipeline registers and forwarding.
*/

module datapath #(
    parameter logic [31:0] BOOT_ADDR = 32'h0000_0000
) (
    input logic clk_i,
    input logic rst_ni,

    // IMEM Interface
    output logic [31:0] instr_addr_o,
    input  logic [31:0] instr_rdata_i,

    // DMEM Interface
    output logic [31:0] dmem_addr_o,
    output logic [31:0] dmem_wdata_o,
    output logic [ 3:0] dmem_be_o,
    output logic        dmem_we_o,
    input  logic [31:0] dmem_rdata_i,

    // Control Unit Signals (Inputs to Datapath)
    input logic stall_i,
    input logic is_branch_i,
    input logic is_jump_i,
    input logic jalr_sel_i,

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

    // Outputs to BU and Control Unit
    output logic [6:0] opcode_o,
    output logic [2:0] funct3_o,
    output logic       funct7_5_o
);

  import supernova_pkg::*;

  // ========================================================
  // PIPELINE REGISTERS (State Variables)
  // ========================================================
  if_id_t if_id_reg, if_id_next;
  id_ex_t id_ex_reg, id_ex_next;
  ex_mem_t ex_mem_reg, ex_mem_next;
  mem_wb_t mem_wb_reg, mem_wb_next;

  // ========================================================
  // INTERNAL WIRES
  // ========================================================
  logic [31:0] fetch_pc, fetch_pc_next, fetch_instr;
  logic [31:0] branch_target;

  logic [31:0] decode_rs1, decode_rs2, decode_imm;

  logic [31:0] exec_alu_res, exec_target_addr, exec_jalr_addr;
  logic [1:0] forward_a, forward_b;
  logic [31:0] alu_mux_a_out, alu_mux_b_out;

  logic [31:0] mem_lsu_rdata;
  logic [31:0] wb_final_data;

  // ========================================================
  // HAZARD DETECTION UNIT (Load-Use Hazard)
  // ========================================================
  logic load_use_hazard;
  logic pipeline_stall;
  logic [4:0] if_id_rs1;
  logic [4:0] if_id_rs2;

  assign if_id_rs1 = if_id_reg.instr[19:15];
  assign if_id_rs2 = if_id_reg.instr[24:20];

  always_comb begin
    load_use_hazard = 1'b0;
    // Se a instrução no EX for um LOAD (lê da memória e escreve no registrador)
    if (id_ex_reg.ctrl.reg_we && (id_ex_reg.ctrl.wb_src == WbSrcMem)) begin
      // Se o destino desse Load for exigido pela instrução logo atrás no ID
      if ((id_ex_reg.rd_addr == if_id_rs1) || (id_ex_reg.rd_addr == if_id_rs2)) begin
        if (id_ex_reg.rd_addr != 5'd0) begin
          load_use_hazard = 1'b1;  // Detectou! O pipeline precisa parar 1 ciclo.
        end
      end
    end
  end

  // O stall da pipeline é ativado externamente OU pelo Load-Use
  assign pipeline_stall = stall_i | load_use_hazard;

  // ========================================================
  // PIPELINE REGISTER INFERENCE
  // ========================================================
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      if_id_reg  <= '0;
      id_ex_reg  <= '0;
      ex_mem_reg <= '0;
      mem_wb_reg <= '0;
    end else if (branch_valid_ex) begin
      // FLUSH! O salto foi tomado. Limpa as instruções que foram buscadas por engano.
      if_id_reg  <= '0;
      id_ex_reg  <= '0;
      ex_mem_reg <= ex_mem_next;
      mem_wb_reg <= mem_wb_next;
    end else if (pipeline_stall) begin
      // STALL! Congela o Fetch/Decode e injeta uma bolha no Execute
      if_id_reg  <= if_id_reg;  // Mantém a instrução presa no Decode
      id_ex_reg  <= '0;  // Injeta BUBBLE (NOP) no estágio Execute
      ex_mem_reg <= ex_mem_next;
      mem_wb_reg <= mem_wb_next;
    end else begin
      // Fluxo Normal
      if_id_reg  <= if_id_next;
      id_ex_reg  <= id_ex_next;
      ex_mem_reg <= ex_mem_next;
      mem_wb_reg <= mem_wb_next;
    end
  end

  // ========================================================
  // ROUTING & EXTERNAL CONTROL EXPORTS
  // ========================================================
  // O Control Unit fora do datapath tem que olhar para a instrução no IF/ID
  assign opcode_o = if_id_reg.instr[6:0];
  assign funct3_o = if_id_reg.instr[14:12];
  assign funct7_5_o = if_id_reg.instr[30];

  // A Branch Unit externa avalia os dados já no estágio EX
  assign branch_target = id_ex_reg.ctrl.jalr_sel ? exec_jalr_addr : exec_target_addr;

  // ========================================================
  // STAGE 1: FETCH
  // ========================================================
  fetch_stage #(
      .BOOT_ADDR(BOOT_ADDR)
  ) u_fetch_stage (
      .clk_i          (clk_i),
      .rst_ni         (rst_ni),
      .stall_i        (pipeline_stall),
      .branch_valid_i (branch_valid_ex),
      .branch_target_i(branch_target),

      .instr_addr_o (instr_addr_o),
      .instr_rdata_i(instr_rdata_i),

      .pc_o     (fetch_pc),
      .pc_next_o(fetch_pc_next),
      .instr_o  (fetch_instr)
  );

  assign if_id_next.pc        = fetch_pc;
  assign if_id_next.pc_plus_4 = fetch_pc_next;
  assign if_id_next.instr     = fetch_instr;

  // ========================================================
  // STAGE 2: DECODE
  // ========================================================
  decode_stage u_decode_stage (
      .clk_i          (clk_i),
      .instr_payload_i(if_id_reg.instr[31:7]),
      .imm_type_i     (imm_type_i),

      // As portas do loop de Write-Back que consertamos antes!
      .wb_rd_addr_i(mem_wb_reg.rd_addr),
      .wb_rd_data_i(wb_final_data),
      .wb_reg_we_i (mem_wb_reg.ctrl.reg_we),

      .rs1_data_o(decode_rs1),
      .rs2_data_o(decode_rs2),
      .imm_o     (decode_imm)
  );

  // Empacotando os dados E os sinais de controle gerados para ir para o EX
  assign id_ex_next.pc                = if_id_reg.pc;
  assign id_ex_next.pc_plus_4         = if_id_reg.pc_plus_4;
  assign id_ex_next.rs1_data          = decode_rs1;
  assign id_ex_next.rs2_data          = decode_rs2;
  assign id_ex_next.imm               = decode_imm;
  assign id_ex_next.rs1_addr          = if_id_reg.instr[19:15];
  assign id_ex_next.rs2_addr          = if_id_reg.instr[24:20];
  assign id_ex_next.rd_addr           = if_id_reg.instr[11:7];

  assign id_ex_next.ctrl.reg_we       = reg_we_i;
  assign id_ex_next.ctrl.wb_src       = wb_src_i;
  assign id_ex_next.ctrl.mem_size     = mem_size_i;
  assign id_ex_next.ctrl.mem_we       = mem_we_i;
  assign id_ex_next.ctrl.mem_unsigned = mem_unsigned_i;
  assign id_ex_next.ctrl.alu_src_a    = alu_src_a_i;
  assign id_ex_next.ctrl.alu_src_b    = alu_src_b_i;
  assign id_ex_next.ctrl.alu_op       = alu_op_i;
  assign id_ex_next.ctrl.jalr_sel     = jalr_sel_i;
  assign id_ex_next.ctrl.is_branch    = is_branch_i;
  assign id_ex_next.ctrl.is_jump      = is_jump_i;
  assign id_ex_next.funct3            = if_id_reg.instr[14:12];

  // ========================================================
  // FORWARDING & HAZARD RESOLUTION (Intercepta os dados antes do EX)
  // ========================================================
  forwarding_unit u_forwarding_unit (
      .id_ex_rs1_addr_i(id_ex_reg.rs1_addr),
      .id_ex_rs2_addr_i(id_ex_reg.rs2_addr),

      .ex_mem_rd_addr_i(ex_mem_reg.rd_addr),
      .ex_mem_reg_we_i (ex_mem_reg.ctrl.reg_we),

      .mem_wb_rd_addr_i(mem_wb_reg.rd_addr),
      .mem_wb_reg_we_i (mem_wb_reg.ctrl.reg_we),

      .forward_a_o(forward_a),
      .forward_b_o(forward_b)
  );

  assign alu_mux_a_out = (forward_a == 2'b10) ? ex_mem_reg.alu_result :
                         (forward_a == 2'b01) ? wb_final_data :
                                                id_ex_reg.rs1_data;

  assign alu_mux_b_out = (forward_b == 2'b10) ? ex_mem_reg.alu_result :
                         (forward_b == 2'b01) ? wb_final_data :
                                                id_ex_reg.rs2_data;

  // ========================================================
  // STAGE 3: EXECUTE
  // ========================================================
  execute_stage u_execute_stage (
      .pc_i      (id_ex_reg.pc),
      .rs1_data_i(alu_mux_a_out),  // Dados limpos (com forwarding)
      .rs2_data_i(alu_mux_b_out),  // Dados limpos (com forwarding)
      .imm_i     (id_ex_reg.imm),

      .alu_src_a_i(id_ex_reg.ctrl.alu_src_a),
      .alu_src_b_i(id_ex_reg.ctrl.alu_src_b),
      .alu_op_i   (id_ex_reg.ctrl.alu_op),

      .alu_result_o (exec_alu_res),
      .target_addr_o(exec_target_addr),
      .jalr_addr_o  (exec_jalr_addr)
  );

  assign ex_mem_next.alu_result = exec_alu_res;
  assign ex_mem_next.rs2_data   = alu_mux_b_out;  // Stores usam dado mais recente
  assign ex_mem_next.pc_plus_4  = id_ex_reg.pc_plus_4;
  assign ex_mem_next.rd_addr    = id_ex_reg.rd_addr;
  assign ex_mem_next.ctrl       = id_ex_reg.ctrl;

  logic branch_taken_ex;
  logic branch_valid_ex;

  branch_unit u_branch_unit (
      .rs1_data_i    (alu_mux_a_out),             // Usa o dado COM forwarding
      .rs2_data_i    (alu_mux_b_out),             // Usa o dado COM forwarding
      .is_branch_i   (id_ex_reg.ctrl.is_branch),  // Sinal viajou pelo pipeline
      .funct3_i      (id_ex_reg.funct3),          // Funct3 viajou pelo pipeline
      .branch_taken_o(branch_taken_ex)
  );

  // A decisão final de salto é tomada AQUI, no estágio EX
  assign branch_valid_ex = branch_taken_ex || id_ex_reg.ctrl.is_jump;

  // ========================================================
  // STAGE 4: MEMORY
  // ========================================================
  memory_stage u_memory_stage (
      .alu_result_i(ex_mem_reg.alu_result),
      .rs2_data_i  (ex_mem_reg.rs2_data),

      .mem_we_i      (ex_mem_reg.ctrl.mem_we),
      .mem_size_i    (ex_mem_reg.ctrl.mem_size),
      .mem_unsigned_i(ex_mem_reg.ctrl.mem_unsigned),

      .dmem_rdata_i(dmem_rdata_i),
      .dmem_addr_o (dmem_addr_o),
      .dmem_wdata_o(dmem_wdata_o),
      .dmem_be_o   (dmem_be_o),

      .lsu_rdata_o(mem_lsu_rdata)
  );

  assign mem_wb_next.alu_result = ex_mem_reg.alu_result;
  assign mem_wb_next.lsu_rdata  = mem_lsu_rdata;
  assign mem_wb_next.pc_plus_4  = ex_mem_reg.pc_plus_4;
  assign mem_wb_next.rd_addr    = ex_mem_reg.rd_addr;
  assign mem_wb_next.ctrl       = ex_mem_reg.ctrl;
  assign dmem_we_o              = ex_mem_reg.ctrl.mem_we;

  // ========================================================
  // STAGE 5: WRITE-BACK
  // ========================================================
  writeback_stage u_writeback_stage (
      .alu_result_i(mem_wb_reg.alu_result),
      .lsu_rdata_i (mem_wb_reg.lsu_rdata),
      .pc_plus_4_i (mem_wb_reg.pc_plus_4),

      .wb_src_i(mem_wb_reg.ctrl.wb_src),

      .wb_data_o(wb_final_data)
  );

endmodule
