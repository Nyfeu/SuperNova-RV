`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Execute Stage (execute_stage)
    DESCRIPTION: Encapsulates the ALU, operand multiplexers, and target
      address calculations. Purely combinational.
*/

module execute_stage (
    // Data Inputs
    input logic [31:0] pc_i,
    input logic [31:0] rs1_data_i,
    input logic [31:0] rs2_data_i,
    input logic [31:0] imm_i,

    // Control Inputs
    input  supernova_pkg::alu_src_a_e alu_src_a_i,
    input  supernova_pkg::alu_src_b_e alu_src_b_i,
    input  supernova_pkg::alu_op_e    alu_op_i,

    // Data Outputs
    output logic [31:0] alu_result_o,
    output logic [31:0] target_addr_o,
    output logic [31:0] jalr_addr_o
);

  import supernova_pkg::*;

  // Internal Signals
  logic [31:0] alu_in_a;
  logic [31:0] alu_in_b;

  // --------------------------------------------------------
  // Combinational Logic: Target Address Calculation
  // --------------------------------------------------------
  // Usado para saltos JAL e Branches (PC-relative)
  assign target_addr_o = pc_i + imm_i;

  // --------------------------------------------------------
  // Combinational Logic: ALU Multiplexers
  // --------------------------------------------------------

  // Mux A
  always_comb begin
    case (alu_src_a_i)
      AluSrcARs1:  alu_in_a = rs1_data_i;
      AluSrcAPc:   alu_in_a = pc_i;
      AluSrcAZero: alu_in_a = 32'b0;
      default:     alu_in_a = rs1_data_i;
    endcase
  end

  // Mux B
  assign alu_in_b = (alu_src_b_i == AluSrcBImm) ? imm_i : rs2_data_i;

  // --------------------------------------------------------
  // ALU Instantiation
  // --------------------------------------------------------
  // A ALU não possui flag Zero, pois a Unidade de Branch cuida disso.
  alu u_alu (
      .a_i     (alu_in_a),
      .b_i     (alu_in_b),
      .alu_op_i(alu_op_i),

      .result_o(alu_result_o)
  );

  // --------------------------------------------------------
  // Combinational Logic: JALR Address Masking
  // --------------------------------------------------------
  // A especificação RISC-V exige que o LSB seja zerado para o JALR
  // O JALR usa a ALU para calcular rs1 + imm.
  assign jalr_addr_o = {alu_result_o[31:1], 1'b0};

endmodule
