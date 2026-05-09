/*

    PROJECT: SuperNova-RV
    MODULE: Arithmetic Logic Unit (alu)
    DESCRIPTION: Purely combinational unit for mathematical and logical operations.

    AUTHOR: André Solano Ferreira Rodrigues Maiolini
    DATE: 2026-04-30

*/

module alu #(

    // Default WIDTH values to 32 bits for RV32I
    parameter int WIDTH = 32

) (

    // Input operands (A and B)
    input logic [WIDTH-1:0] a_i,
    input logic [WIDTH-1:0] b_i,

    // ALU operation code (4 bits, defined in supernova_pkg)
    input supernova_pkg::alu_op_e alu_op_i,

    // We reference the package explicitly (::) in the port declaration
    // to avoid polluting the global namespace and to make it clear where the type is defined.

    // Output result of the ALU operation
    output logic [WIDTH-1:0] result_o

);

  // We should import the package inside the module to use its definitions
  // without polluting the global namespace.
  import supernova_pkg::*;

  // Combinational logic for ALU operations
  always_comb begin

    case (alu_op_i)

      AluAdd: result_o = a_i + b_i;
      AluSub: result_o = a_i - b_i;
      AluSll: result_o = a_i << b_i[4:0];

      // $signed() tells the synthesizer to treat the bits as two's complement
      AluSlt:  result_o = {31'b0, ($signed(a_i) < $signed(b_i))};
      AluSltu: result_o = {31'b0, (a_i < b_i)};
      AluXor:  result_o = a_i ^ b_i;
      AluSrl:  result_o = a_i >> b_i[4:0];

      // The >>> operator in SV performs an arithmetic shift only if the operand is signed
      AluSra: result_o = $unsigned($signed(a_i) >>> b_i[4:0]);
      AluOr:  result_o = a_i | b_i;
      AluAnd: result_o = a_i & b_i;

      // Default case to handle undefined operations
      // It should never be hit if the control logic is correct!
      default: result_o = '0;

    endcase

  end

endmodule
// Comentário para forçar CI no Nebula
