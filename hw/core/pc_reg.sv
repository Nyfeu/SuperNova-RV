// Netype default é none para evitar erros de conexão de fios não intencionais
`default_nettype none



module pc_reg #(
    parameter logic [31:0] RESET_VECTOR = 32'h0000_0000
) (
    input wire logic clk_i,
    input wire logic rst_ni,
    input wire logic stall_i,

    // Entrada direta do próximo PC (calculado pelo Datapath, igual ao seu VHDL)
    input wire logic [31:0] pc_next_i,

    output logic [31:0] pc_o
);

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      pc_o <= RESET_VECTOR;
    end else if (!stall_i) begin
      pc_o <= pc_next_i;
    end
    // Se stall_i for 1, pc_o mantém o valor atual
  end

endmodule
