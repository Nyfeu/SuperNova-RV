`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Forwarding Unit
    DESCRIPTION: Resolves Read-After-Write (RAW) data hazards by comparing
                 source registers in the EX stage with destination registers
                 in the MEM and WB stages.
*/

module forwarding_unit (
    // Endereços de origem da instrução atual (entrando na ALU)
    input logic [4:0] id_ex_rs1_addr_i,
    input logic [4:0] id_ex_rs2_addr_i,

    // Informações da instrução no estágio MEM (1 ciclo à frente)
    input logic [4:0] ex_mem_rd_addr_i,
    input logic       ex_mem_reg_we_i,

    // Informações da instrução no estágio WB (2 ciclos à frente)
    input logic [4:0] mem_wb_rd_addr_i,
    input logic       mem_wb_reg_we_i,

    // Sinais de controle dos Multiplexadores de Forwarding
    // 00: Caminho normal (do RegFile)
    // 01: Forwarding do estágio MEM/WB (Prioridade 2)
    // 10: Forwarding do estágio EX/MEM (Prioridade 1)
    output logic [1:0] forward_a_o,
    output logic [1:0] forward_b_o
);

  // Lógica para a entrada A (rs1)
  always_comb begin
    forward_a_o = 2'b00;

    if (ex_mem_reg_we_i && (ex_mem_rd_addr_i != 5'd0)
        && (ex_mem_rd_addr_i == id_ex_rs1_addr_i))begin
      forward_a_o = 2'b10;
    end
    else if (mem_wb_reg_we_i && (mem_wb_rd_addr_i != 5'd0)
        && (mem_wb_rd_addr_i == id_ex_rs1_addr_i)) begin
      forward_a_o = 2'b01;
    end
  end

  // Lógica para a entrada B (rs2)
  always_comb begin
    forward_b_o = 2'b00;

    if (ex_mem_reg_we_i && (ex_mem_rd_addr_i != 5'd0)
        && (ex_mem_rd_addr_i == id_ex_rs2_addr_i)) begin
      forward_b_o = 2'b10;
    end
      else if (mem_wb_reg_we_i && (mem_wb_rd_addr_i != 5'd0)
        && (mem_wb_rd_addr_i == id_ex_rs2_addr_i)) begin
      forward_b_o = 2'b01;
    end
  end

endmodule
