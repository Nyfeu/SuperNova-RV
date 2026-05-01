/*
    PROJECT: SuperNova-RV
    MODULE: SuperNova Package (supernova_pkg)
    DESCRIPTION: Global package containing type definitions, enums, and constants shared across the entire processor architecture.

    AUTHOR: André Solano Ferreira Rodrigues Maiolini
    DATE: 2026-04-30
*/

package supernova_pkg;

  // Definition of ALU operation codes using strong typing (enum)
  // Names in PascalCase to follow Verible Linter rules

  typedef enum logic [3:0] {

    AluAdd  = 4'b0000,
    AluSub  = 4'b1000,
    AluSll  = 4'b0001,
    AluSlt  = 4'b0010,
    AluSltu = 4'b0011,
    AluXor  = 4'b0100,
    AluSrl  = 4'b0101,
    AluSra  = 4'b1101,
    AluOr   = 4'b0110,
    AluAnd  = 4'b0111

  } alu_op_e;

endpackage
