`default_nettype none

module dummy (
    input  wire logic        clk_i,
    input  wire logic        rst_ni,
    input  wire logic [31:0] data_i,
    output logic      [31:0] data_o
);
  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      data_o <= 32'd0;
    end else begin
      data_o <= ~data_i;
    end
  end
endmodule
