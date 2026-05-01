/*

    PROJECT: RISC-V CPU Core

    MODULE: Register File (reg_file)
    DESCRIPTION: Arquivo de registradores que armazena dados temporários durante a execução da CPU.

    AUTHOR: André Solano Ferreira Rodrigues Maiolini
    DATE: 2026-04-30

    PARAMETERS:
    - WIDTH: Largura do barramento de dados (padrão 32 bits para RV32I).

    INPUTS:
    - clk_i: Clock de entrada (transição de subida)
    - we_i: Write Enable (Habilita escrita no registrador de destino)
    - rs1_addr_i: Endereço do registrador fonte 1
    - rs2_addr_i: Endereço do registrador fonte 2
    - rd_addr_i: Endereço do registrador de destino
    - rd_data_i: Dado a ser escrito no registrador de destino

    OUTPUTS:
    - rs1_data_o: Dado lido da porta fonte 1
    - rs2_data_o: Dado lido da porta fonte 2

    BEHAVIOR:
    - Leitura puramente combinacional (assíncrona).
    - Escrita síncrona na borda de subida do clock.
    - Registrador x0 (endereço 0) é hardwired em zero, ignorando escritas.

*/

module reg_file #(

    // Default WIDTH values to 32 bits for RV32I
    parameter int WIDTH = 32

) (

    input logic clk_i,
    input logic we_i,

    input logic [      4:0] rs1_addr_i,
    input logic [      4:0] rs2_addr_i,
    input logic [      4:0] rd_addr_i,
    input logic [WIDTH-1:0] rd_data_i,

    output logic [WIDTH-1:0] rs1_data_o,
    output logic [WIDTH-1:0] rs2_data_o

);

  // Array interno de 32 registradores
  logic [WIDTH-1:0] registers[32];

  // --------------------------------------------------------
  // Lógica de Escrita (Síncrona)
  // --------------------------------------------------------

  always_ff @(posedge clk_i) begin

    // Escreve apenas se o Enable estiver ativo e não for o registrador x0
    if (we_i && (rd_addr_i != 5'b00000)) begin
      registers[rd_addr_i] <= rd_data_i;
    end

  end

  // --------------------------------------------------------
  // Lógica de Leitura (Combinacional)
  // --------------------------------------------------------

  always_comb begin

    // Retorna o valor do array, ou força 0 se o endereço for 0
    rs1_data_o = (rs1_addr_i == 5'b00000) ? '0 : registers[rs1_addr_i];
    rs2_data_o = (rs2_addr_i == 5'b00000) ? '0 : registers[rs2_addr_i];

  end

endmodule
