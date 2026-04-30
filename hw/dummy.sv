`default_nettype none

module dummy (
    input  wire logic        clk,
    input  wire logic        rst_n,
    input  wire logic [31:0] data_in,
    output logic      [31:0] data_out
);
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      data_out <= 32'd0;
    end else begin
      data_out <= ~data_in;  // Inversão simples para teste
    end
  end
endmodule
