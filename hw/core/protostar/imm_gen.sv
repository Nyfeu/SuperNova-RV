/*

    PROJECT: SuperNova-RV
    MODULE: Immediate Generator (imm_gen)
    DESCRIPTION: Purely combinational unit for extracting and sign-extending immediate values from RISC-V instructions based on the instruction format.

    AUTHOR: André Solano Ferreira Rodrigues Maiolini
    DATE: 2026-05-01

*/

// `default_nettype none is a good practice to prevent implicit wire declarations, which can lead to hard-to-find bugs.
`default_nettype none

module imm_gen (

    // The raw 32-bit instruction fetched from memory
    // Bits [6:0] (opcode) are ignored here as they are decoded by the Control Unit
    input logic [31:7] instr_i,

    // Control signal indicating the instruction format
    // We reference the package explicitly (::) in the port declaration
    input supernova_pkg::imm_type_e imm_type_i,

    // The reconstructed, 32-bit sign-extended immediate value
    output logic [31:0] imm_o

);

  // We should import the package inside the module to use its definitions
  // (like IMM_I, IMM_S) without polluting the global namespace.
  import supernova_pkg::*;

  // Combinational logic for Immediate Extraction
  always_comb begin

    case (imm_type_i)

      // I-Type: Loads, I-ALU, jalr (Sign-extend bits 31:20)
      ImmI: imm_o = {{20{instr_i[31]}}, instr_i[31:20]};

      // S-Type: Stores (Sign-extend bit 31, concat 30:25 and 11:7)
      ImmS: imm_o = {{20{instr_i[31]}}, instr_i[31:25], instr_i[11:7]};

      // B-Type: Branches (Sign-extend bit 31, mix 7, 30:25, 11:8, implicit 0)
      ImmB: imm_o = {{20{instr_i[31]}}, instr_i[7], instr_i[30:25], instr_i[11:8], 1'b0};

      // U-Type: lui, auipc (Bits 31:12, pad lower 12 bits with 0)
      ImmU: imm_o = {instr_i[31:12], 12'b0};

      // J-Type: jal (Sign-extend bit 31, mix 19:12, 20, 30:21, implicit 0)
      ImmJ: imm_o = {{12{instr_i[31]}}, instr_i[19:12], instr_i[20], instr_i[30:21], 1'b0};

      // Default fallback to 0 to prevent latches and handle R-Type/unknowns
      default: imm_o = '0;

    endcase

  end

endmodule
