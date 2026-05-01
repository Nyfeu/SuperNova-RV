`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Memory Stage (memory_stage)
    DESCRIPTION: Encapsulates the Load-Store Unit (LSU). Interfaces with the
                 external Data Memory (DMEM) to format reads and writes.
*/

module memory_stage (
    // Data Inputs (From Execute/Decode)
    input logic [31:0] alu_result_i,
    input logic [31:0] rs2_data_i,

    // Control Inputs
    input logic                     mem_we_i,
    input supernova_pkg::mem_size_e mem_size_i,
    input logic                     mem_unsigned_i,

    // DMEM External Interface
    input  logic [31:0] dmem_rdata_i,
    output logic [31:0] dmem_addr_o,
    output logic [31:0] dmem_wdata_o,
    output logic [ 3:0] dmem_be_o,

    // Data Outputs (To Write-Back)
    output logic [31:0] lsu_rdata_o
);

  import supernova_pkg::*;

  // --------------------------------------------------------
  // Load-Store Unit Instantiation
  // --------------------------------------------------------
  lsu u_lsu (
      .addr_i        (alu_result_i),
      .wdata_i       (rs2_data_i),
      .we_i          (mem_we_i),
      .mem_size_i    (mem_size_i),
      .mem_unsigned_i(mem_unsigned_i),
      .mem_rdata_i   (dmem_rdata_i),

      .mem_addr_o (dmem_addr_o),
      .mem_wdata_o(dmem_wdata_o),
      .mem_be_o   (dmem_be_o),
      .rdata_o    (lsu_rdata_o)
  );

endmodule
