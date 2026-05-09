

/*

    PROJECT: RISC-V CPU Core

    MODULE: Program Counter Register (pc_reg)
    DESCRIPTION: Registro que armazena o endereço de programa (PC) atual com
                suporte a stall e reset síncrono.

    AUTHOR: André Solano Ferreira Rodrigues Maiolini
    DATE: 2026-04-30

    PARAMETERS:
    - RESET_VECTOR: Valor inicial do PC (padrão: 0x0000_0000)

    INPUTS:
    - clk_i: Clock de entrada (transição de subida)
    - rst_ni: Reset ativo baixo (assincrono)
    - stall_i: Sinal de stall (quando '1', PC mantém valor)
    - pc_next_i: Próximo valor do PC (32 bits)

    OUTPUTS:
    - pc_o: Valor atual do PC (32 bits)

    BEHAVIOR:
    - Em reset: PC é carregado com RESET_VECTOR
    - Durante stall: PC mantém valor anterior
    - Normal: PC é atualizado com pc_next_i na transição de clock

*/

// Netype default é none para evitar erros de conexão de fios não intencionais
`default_nettype none

// Definição do módulo pc_reg
module pc_reg #(
    parameter logic [31:0] RESET_VECTOR = 32'h0000_0000
) (

    input wire logic clk_i,
    input wire logic rst_ni,
    input wire logic stall_i,

    // Entrada direta do próximo PC (calculado pelo Datapath, igual ao seu VHDL)
    input wire logic [31:0] pc_next_i,

    // Saída do PC atual
    output logic [31:0] pc_o
);

  // Lógica de atualização do PC
  always_ff @(posedge clk_i or negedge rst_ni) begin

    if (!rst_ni) begin

      pc_o <= RESET_VECTOR;

    end else if (!stall_i) begin

      // Se stall_i for 1, pc_o mantém o valor atual
      pc_o <= pc_next_i;

    end

  end

endmodule
