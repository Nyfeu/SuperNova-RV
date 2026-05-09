`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Write-Back Stage (writeback_stage)
    DESCRIPTION: Contains the final multiplexer that selects the data to be
                 written back into the Register File.
*/

module writeback_stage (
    // Data Inputs
    input logic [31:0] alu_result_i,  // From Execute
    input logic [31:0] lsu_rdata_i,   // From Memory
    input logic [31:0] pc_plus_4_i,   // From Fetch

    // Control Inputs
    input supernova_pkg::wb_src_e wb_src_i,

    // Data Outputs
    output logic [31:0] wb_data_o  // To Decode (RegFile)
);

  import supernova_pkg::*;

  always_comb begin
    case (wb_src_i)
      WbSrcAlu: wb_data_o = alu_result_i;
      WbSrcMem: wb_data_o = lsu_rdata_i;
      WbSrcPc:  wb_data_o = pc_plus_4_i;
      default:  wb_data_o = alu_result_i;
    endcase
  end

endmodule
