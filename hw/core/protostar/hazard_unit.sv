`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Hazard Detection Unit (HDU)
    DESCRIPTION: Monitors the pipeline for Load-Use data hazards and
                 issues stall signals to freeze the Fetch/Decode stages.
*/

module hazard_unit (
    input logic       id_ex_mem_read_i,
    input logic [4:0] id_ex_rd_addr_i,
    input logic [4:0] if_id_rs1_addr_i,
    input logic [4:0] if_id_rs2_addr_i,

    output logic load_use_hazard_o
);

  always_comb begin
    load_use_hazard_o = 1'b0;

    // Se a instrução no EX for um LOAD
    if (id_ex_mem_read_i && (id_ex_rd_addr_i != 5'd0)) begin
      // E o destino desse Load for exigido pela instrução logo atrás no ID
      if ((id_ex_rd_addr_i == if_id_rs1_addr_i) || (id_ex_rd_addr_i == if_id_rs2_addr_i)) begin
        load_use_hazard_o = 1'b1;  // Detectou colisão temporal!
      end
    end
  end

endmodule
