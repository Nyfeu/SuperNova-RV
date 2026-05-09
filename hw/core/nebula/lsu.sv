/*

    PROJECT: SuperNova-RV
    MODULE: Load Store Unit (LSU) (lsu)
    DESCRIPTION: This module handles all data memory access operations for the CPU.
        It manages byte/half-word/word loads and stores with proper alignment
        handling and sign/zero extension for sub-word loads.

    AUTHOR: André Solano Ferreira Rodrigues Maiolini
    DATE: 2026-05-01

*/

`default_nettype none

module lsu (
    input logic                     [31:0] addr_i,
    input logic                     [31:0] wdata_i,
    input logic                            we_i,
    input supernova_pkg::mem_size_e        mem_size_i,
    input logic                            mem_unsigned_i,
    input logic                     [31:0] mem_rdata_i,

    output logic [31:0] mem_addr_o,
    output logic [31:0] mem_wdata_o,
    output logic [ 3:0] mem_be_o,
    output logic [31:0] rdata_o
);

  import supernova_pkg::*;

  // ========================================================================
  // Address Alignment and Offset Calculation
  // ========================================================================
  // The memory is word-aligned (32-bit), so we ignore the lower 2 bits
  // and align the address to word boundary (bits [31:2]). The lower 2 bits
  // are preserved as an offset to determine the byte position within the word.

  assign mem_addr_o = {addr_i[31:2], 2'b00};
  logic [1:0] offset;
  assign offset = addr_i[1:0];  // Extract byte offset (0-3)

  // ========================================================================
  // STORE LOGIC (Writing data to memory)
  // ========================================================================
  // This block prepares data for writing to memory, handling different
  // data sizes and their alignment within a 32-bit word. The mem_be_o
  // signal (Byte Enable) tells the memory which bytes are valid.
  // ========================================================================

  // Store Logic
  always_comb begin
    mem_wdata_o = '0;
    mem_be_o    = 4'b0000;  // Byte Enable: each bit enables one byte in memory

    // Only prepare write data if the write enable signal is active
    if (we_i) begin
      case (mem_size_i)

        // BYTE Store: Write a single byte at the aligned position
        // Example: store 0x42 at address 0x1001 (offset=1)
        //   - Shift byte left by 8 bits: 0x4200
        //   - Enable byte 1: mem_be_o = 0010
        MemSizeByte: begin
          mem_wdata_o = (wdata_i & 32'h000000FF) << (offset * 8);
          mem_be_o    = 4'b0001 << offset;
        end

        // HALF-WORD Store: Write 2 bytes (16 bits)
        // Example: store 0x4242 at address 0x1000 (offset=0)
        //   - Data stays at position 0: 0x4242
        //   - Enable bytes 0-1: mem_be_o = 0011
        // If offset=2, enable bytes 2-3: mem_be_o = 1100
        MemSizeHalf: begin
          mem_wdata_o = (wdata_i & 32'h0000FFFF) << (offset * 8);
          mem_be_o    = (offset[1]) ? 4'b1100 : 4'b0011;
        end

        // WORD Store: Write all 4 bytes (32 bits)
        // Always enable all bytes and write the full word
        MemSizeWord: begin
          mem_wdata_o = wdata_i;
          mem_be_o    = 4'b1111;  // All bytes enabled
        end

        default: ;
      endcase
    end
  end

  // ========================================================================
  // LOAD LOGIC (Reading data from memory)
  // ========================================================================
  // This block extracts the correct byte(s) from the 32-bit word returned
  // by memory, shifts them to the LSB position, and applies sign or zero
  // extension based on the load type and the mem_unsigned_i flag.
  // ========================================================================

  logic [15:0] shifted;  // Extracted data shifted to LSB position
  logic [ 7:0] byte_data;  // 8-bit data for byte loads
  logic [15:0] half_data;  // 16-bit data for half-word loads

  always_comb begin
    // Shift the data from memory based on the byte offset
    // Example: If offset=2, shift right by 16 bits to get bytes [31:16] at [15:0]
    shifted   = 16'((mem_rdata_i >> (offset * 8)));
    byte_data = shifted[7:0];  // Extract the lower 8 bits
    half_data = shifted[15:0];  // Extract the lower 16 bits

    case (mem_size_i)
      // BYTE Load: Extract 1 byte and apply sign/zero extension
      // Zero Extension: {24'b0, byte_data}     → Pad with zeros (for unsigned)
      // Sign Extension: {{24{byte_data[7]}}, byte_data} → Replicate sign bit (for signed)
      MemSizeByte: rdata_o = mem_unsigned_i ? {24'b0, byte_data} : {{24{byte_data[7]}}, byte_data};

      // HALF-WORD Load: Extract 2 bytes and apply sign/zero extension
      // Example (signed): Load 0xFF00 → becomes 0xFFFFFF00 (sign extended)
      // Example (unsigned): Load 0xFF00 → becomes 0x0000FF00 (zero extended)
      MemSizeHalf: rdata_o = mem_unsigned_i ? {16'b0, half_data} : {{16{half_data[15]}}, half_data};

      // WORD Load: Return the full 32-bit word as-is (no extension needed)
      MemSizeWord: rdata_o = mem_rdata_i;

      default: rdata_o = '0;  // Safety: output zeros for invalid operations
    endcase
  end

endmodule
