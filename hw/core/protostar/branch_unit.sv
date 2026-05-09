`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Branch Unit (branch_unit)
    DESCRIPTION: Dedicated combinational comparator for evaluating conditional
                 branch instructions. Operates in parallel with the ALU.
*/

module branch_unit (
    input logic [31:0] rs1_data_i,
    input logic [31:0] rs2_data_i,
    input logic        is_branch_i,
    input logic [ 2:0] funct3_i,

    output logic branch_taken_o
);

  always_comb begin
    // Default: do not take the branch
    branch_taken_o = 1'b0;

    // Only evaluate if the Instruction Decoder flagged this as a branch
    if (is_branch_i) begin
      case (funct3_i)
        3'b000:  branch_taken_o = (rs1_data_i == rs2_data_i);  // BEQ
        3'b001:  branch_taken_o = (rs1_data_i != rs2_data_i);  // BNE
        3'b100:  branch_taken_o = ($signed(rs1_data_i) < $signed(rs2_data_i));  // BLT
        3'b101:  branch_taken_o = ($signed(rs1_data_i) >= $signed(rs2_data_i));  // BGE
        3'b110:  branch_taken_o = (rs1_data_i < rs2_data_i);  // BLTU
        3'b111:  branch_taken_o = (rs1_data_i >= rs2_data_i);  // BGEU
        default: branch_taken_o = 1'b0;
      endcase
    end
  end

endmodule
