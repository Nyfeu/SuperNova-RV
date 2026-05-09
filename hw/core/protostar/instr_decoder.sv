`default_nettype none

/*
    PROJECT: SuperNova-RV
    MODULE: Instruction Decoder (instr_decoder)
    DESCRIPTION: Purely combinational flat decoder. Receives only the strictly
                 necessary instruction fields to avoid dangling wires.

    AUTHOR: André Solano Ferreira Rodrigues Maiolini
    DATE: 2026-05-01
*/

module instr_decoder (
    // ========================================================================
    // INPUT: Instruction Fields (Sliced from the 32-bit instruction)
    // ========================================================================
    // The instruction is deconstructed into its essential fields to prevent
    // unnecessary routing of unused bits throughout the design. This approach
    // improves clarity and reduces power consumption.

    input logic [6:0] opcode_i,
    // Bits [6:0] of the instruction. Identifies the instruction type and
    // determines the primary control path (R-type, I-type, Load, Store, etc.)

    input logic [2:0] funct3_i,
    // Bits [14:12] of the instruction. Used to further decode operations
    // within a given opcode category (e.g., distinguishing ADD from SLL in R-type).

    input logic funct7_5_i,
    // Bit [30] of the instruction (the MSB of Funct7). Used to distinguish
    // between variants like ADD/SUB and SRL/SRA in R-type and I-type instructions.

    // ========================================================================
    // OUTPUT: Datapath Control Signals
    // ========================================================================
    // These signals drive the datapath multiplexers and functional units.

    output logic reg_we_o,
    // Register File Write Enable. High when the instruction writes to Rd.

    output supernova_pkg::imm_type_e imm_type_o,
    // Immediate type selector: ImmI, ImmS, ImmB, ImmU, or ImmJ.
    // Drives the Immediate Generator module to format the imm correctly.

    output supernova_pkg::alu_src_a_e alu_src_a_o,
    // First operand source: Rs1 (register), PC, or Zero constant.

    output supernova_pkg::alu_src_b_e alu_src_b_o,
    // Second operand source: Rs2 (register) or Imm (immediate value).

    output supernova_pkg::alu_op_e alu_op_o,
    // ALU operation code: ADD, SUB, SLL, SRL, SRA, AND, OR, XOR, SLT, SLTU.

    output logic mem_we_o,
    // Data Memory Write Enable. High for Store instructions (S-type).

    output supernova_pkg::mem_size_e mem_size_o,
    // Memory access size: Byte (8-bit), Half-word (16-bit), or Word (32-bit).

    output logic mem_unsigned_o,
    // Zero-extension flag for Load instructions. High for unsigned loads (LBU, LHU).

    output supernova_pkg::wb_src_e wb_src_o,
    // Write-back source: ALU result, Memory data, or PC+4 (for jumps).

    // ========================================================================
    // OUTPUT: Top-Level Control Flags
    // ========================================================================
    // These flags signal to the Branch/Jump resolution unit in the top-level
    // control logic that a change-of-flow instruction is being executed.

    output logic is_branch_o,
    // High for conditional branch instructions (B-type: BEQ, BNE, BLT, etc.)

    output logic is_jump_o
    // High for unconditional jump instructions (JAL, JALR).

);

  import supernova_pkg::*;

  // ========================================================================
  // Combinational Decoding Logic
  // ========================================================================
  // This always_comb block implements the flat decoding strategy. All outputs
  // are combinational functions of the inputs, with no state or sequential logic.
  // The critical path runs from inputs to outputs in a single stage.

  always_comb begin
    // ====================================================================
    // Default Values (Prevents latches and defines NOP behavior)
    // ====================================================================
    // All outputs are assigned default values first. This prevents unwanted
    // latches (which would introduce state) and ensures that unimplemented
    // instructions behave as NOPs. The assignment order matters: defaults are
    // set first, then overridden by specific instruction cases.

    reg_we_o       = 1'b0;  // Default: Do not write to register file
    imm_type_o     = ImmI;  // Default: Use I-type immediate format
    alu_src_a_o    = AluSrcARs1;  // Default: First ALU operand from Rs1
    alu_src_b_o    = AluSrcBRs2;  // Default: Second ALU operand from Rs2
    alu_op_o       = AluAdd;  // Default: ALU performs addition
    mem_we_o       = 1'b0;  // Default: Do not write to memory
    mem_size_o     = MemSizeWord;  // Default: Memory access is word-sized
    mem_unsigned_o = 1'b0;  // Default: Sign-extend on load
    wb_src_o       = WbSrcAlu;  // Default: Write-back from ALU
    is_branch_o    = 1'b0;  // Default: Not a branch instruction
    is_jump_o      = 1'b0;  // Default: Not a jump instruction

    // ====================================================================
    // Flat Decoding Logic: Case-by-Opcode with Nested Funct3/Funct7 Decoding
    // ====================================================================
    // The opcode (bits 6:0) serves as the primary decoder. Each opcode case
    // may further use Funct3 and Funct7 to refine the control signals.

    case (opcode_i)

      // -------- R-TYPE: Register-to-Register Operations (Opcode = 0x33) --------
      // Format: funct7[6:0] | rs2[4:0] | rs1[4:0] | funct3[2:0] | rd[4:0] | opcode[6:0]
      // Examples: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU
      OpcodeRType: begin
        reg_we_o    = 1'b1;  // R-type always writes to destination register
        alu_src_a_o = AluSrcARs1;  // First operand from Rs1
        alu_src_b_o = AluSrcBRs2;  // Second operand from Rs2
        wb_src_o    = WbSrcAlu;  // Write-back the ALU result

        // Decode the specific operation based on Funct3 and Funct7[5]
        case (funct3_i)
          3'b000:  alu_op_o = (funct7_5_i) ? AluSub : AluAdd;  // ADD (0x00) or SUB (0x20)
          3'b001:  alu_op_o = AluSll;  // SLL (Shift Left Logical)
          3'b010:  alu_op_o = AluSlt;  // SLT (Set Less Than)
          3'b011:  alu_op_o = AluSltu;  // SLTU (Set Less Than Unsigned)
          3'b100:  alu_op_o = AluXor;  // XOR
          3'b101:  alu_op_o = (funct7_5_i) ? AluSra : AluSrl;  // SRL (0x00) or SRA (0x20)
          3'b110:  alu_op_o = AluOr;  // OR
          3'b111:  alu_op_o = AluAnd;  // AND
          default: alu_op_o = AluAdd;  // Safety default
        endcase
      end

      // -------- I-TYPE: Immediate Arithmetic & Logic (Opcode = 0x13) --------
      // Format: imm[11:0] | rs1[4:0] | funct3[2:0] | rd[4:0] | opcode[6:0]
      // Examples: ADDI, SLLI, SRLI, SRAI, ANDI, ORI, XORI, SLTI, SLTIU
      OpcodeImm: begin
        reg_we_o    = 1'b1;  // I-type always writes to destination register
        imm_type_o  = ImmI;  // Use I-type immediate format
        alu_src_a_o = AluSrcARs1;  // First operand from Rs1
        alu_src_b_o = AluSrcBImm;  // Second operand from immediate
        wb_src_o    = WbSrcAlu;  // Write-back the ALU result

        // Decode the specific operation based on Funct3 and Funct7[5]
        case (funct3_i)
          3'b000:  alu_op_o = AluAdd;  // ADDI
          3'b001:  alu_op_o = AluSll;  // SLLI
          3'b010:  alu_op_o = AluSlt;  // SLTI
          3'b011:  alu_op_o = AluSltu;  // SLTIU
          3'b100:  alu_op_o = AluXor;  // XORI
          3'b101:  alu_op_o = (funct7_5_i) ? AluSra : AluSrl;  // SRLI (0x00) or SRAI (0x20)
          3'b110:  alu_op_o = AluOr;  // ORI
          3'b111:  alu_op_o = AluAnd;  // ANDI
          default: alu_op_o = AluAdd;  // Safety default
        endcase
      end

      // -------- LOAD: Load from Memory (Opcode = 0x03) --------
      // Format: imm[11:0] | rs1[4:0] | funct3[2:0] | rd[4:0] | opcode[6:0]
      // Funct3 encodes: size (byte, half, word) and signedness (signed or unsigned)
      // Examples: LB, LH, LW, LBU, LHU
      OpcodeLoad: begin
        reg_we_o    = 1'b1;  // Load always writes to destination register
        imm_type_o  = ImmI;  // Use I-type immediate for offset
        alu_src_a_o = AluSrcARs1;  // Base address from Rs1
        alu_src_b_o = AluSrcBImm;  // Offset from immediate
        alu_op_o    = AluAdd;  // Always add base + offset to compute address
        wb_src_o    = WbSrcMem;  // Write-back the loaded data from memory

        // Decode size and signedness based on Funct3
        case (funct3_i)
          3'b000: begin
            mem_size_o     = MemSizeByte;  // LB (Load Byte)
            mem_unsigned_o = 1'b0;  // Sign-extend
          end
          3'b001: begin
            mem_size_o     = MemSizeHalf;  // LH (Load Half-word)
            mem_unsigned_o = 1'b0;  // Sign-extend
          end
          3'b010: begin
            mem_size_o     = MemSizeWord;  // LW (Load Word)
            mem_unsigned_o = 1'b0;  // Sign-extend (no effect for full word)
          end
          3'b100: begin
            mem_size_o     = MemSizeByte;  // LBU (Load Byte Unsigned)
            mem_unsigned_o = 1'b1;  // Zero-extend
          end
          3'b101: begin
            mem_size_o     = MemSizeHalf;  // LHU (Load Half-word Unsigned)
            mem_unsigned_o = 1'b1;  // Zero-extend
          end
          default: begin
            mem_size_o     = MemSizeWord;  // Safety default
            mem_unsigned_o = 1'b0;
          end
        endcase
      end

      // -------- STORE: Write to Memory (Opcode = 0x23) --------
      // Format: imm[11:5] | rs2[4:0] | rs1[4:0] | funct3[2:0] | imm[4:0] | opcode[6:0]
      // Funct3 encodes the access size (byte, half, word)
      // Examples: SB, SH, SW
      OpcodeStore: begin
        imm_type_o  = ImmS;  // Use S-type immediate format for offset
        alu_src_a_o = AluSrcARs1;  // Base address from Rs1
        alu_src_b_o = AluSrcBImm;  // Offset from immediate
        alu_op_o    = AluAdd;  // Always add base + offset to compute address
        mem_we_o    = 1'b1;  // Enable memory write

        // Decode access size based on Funct3
        case (funct3_i)
          3'b000:  mem_size_o = MemSizeByte;  // SB (Store Byte)
          3'b001:  mem_size_o = MemSizeHalf;  // SH (Store Half-word)
          3'b010:  mem_size_o = MemSizeWord;  // SW (Store Word)
          default: mem_size_o = MemSizeWord;  // Safety default
        endcase
      end

      // -------- BRANCH: Conditional Branches (Opcode = 0x63) --------
      // Format: imm[12|10:5] | rs2[4:0] | rs1[4:0] | funct3[2:0] | imm[4:1|11] | opcode[6:0]
      // Funct3 encodes the branch condition (EQ, NE, LT, LTU, GE, GEU)
      // Examples: BEQ, BNE, BLT, BGE, BLTU, BGEU
      // Note: The branch resolution logic (comparing operands) is NOT in this decoder;
      //       it happens in a separate Branch Unit that uses Funct3 signals.
      OpcodeBranch: begin
        imm_type_o  = ImmB;  // Use B-type immediate format for offset
        alu_src_a_o = AluSrcARs1;  // First comparison operand from Rs1
        alu_src_b_o = AluSrcBRs2;  // Second comparison operand from Rs2
        alu_op_o    = AluAdd;  // ALU may compute target address (unused here)
        is_branch_o = 1'b1;  // Signal that this is a branch instruction
      end

      // -------- JAL: Unconditional Jump with Link (Opcode = 0x6F) --------
      // Format: imm[20|10:1|11|19:12] | rd[4:0] | opcode[6:0]
      // Always writes PC+4 to Rd (the return address). Used for function calls.
      OpcodeJal: begin
        reg_we_o    = 1'b1;  // JAL always writes the return address to Rd
        imm_type_o  = ImmJ;  // Use J-type immediate format
        alu_src_a_o = AluSrcAPc;  // First ALU operand is current PC
        alu_src_b_o = AluSrcBImm;  // Second ALU operand is the offset immediate
        alu_op_o    = AluAdd;  // ALU computes PC + offset for the target
        wb_src_o    = WbSrcPc;  // Write-back the return address (PC+4)
        is_jump_o   = 1'b1;  // Signal that this is a jump instruction
      end

      // -------- JALR: Unconditional Jump Register with Link (Opcode = 0x67) --------
      // Format: imm[11:0] | rs1[4:0] | funct3[2:0] | rd[4:0] | opcode[6:0]
      // Computes target address as Rs1 + immediate (12-bit signed).
      // Always writes PC+4 to Rd. Used for indirect jumps and function returns.
      OpcodeJalr: begin
        reg_we_o    = 1'b1;  // JALR always writes the return address to Rd
        imm_type_o  = ImmI;  // Use I-type immediate format
        alu_src_a_o = AluSrcARs1;  // First ALU operand is base address from Rs1
        alu_src_b_o = AluSrcBImm;  // Second ALU operand is the offset immediate
        alu_op_o    = AluAdd;  // ALU computes Rs1 + offset for the target
        wb_src_o    = WbSrcPc;  // Write-back the return address (PC+4)
        is_jump_o   = 1'b1;  // Signal that this is a jump instruction
      end

      // -------- LUI: Load Upper Immediate (Opcode = 0x37) --------
      // Format: imm[19:0] | rd[4:0] | opcode[6:0]
      // Loads a 20-bit immediate into the upper 20 bits of Rd, zeroing the lower 12 bits.
      OpcodeLui: begin
        reg_we_o    = 1'b1;  // LUI always writes to destination register
        imm_type_o  = ImmU;  // Use U-type immediate format
        alu_src_a_o = AluSrcAZero;  // First ALU operand is zero (constant)
        alu_src_b_o = AluSrcBImm;  // Second ALU operand is the 20-bit immediate
        alu_op_o    = AluAdd;  // ALU computes 0 + immediate = immediate
        wb_src_o    = WbSrcAlu;  // Write-back the computed value
      end

      // -------- AUIPC: Add Upper Immediate to PC (Opcode = 0x17) --------
      // Format: imm[19:0] | rd[4:0] | opcode[6:0]
      // Adds a 20-bit immediate (shifted left by 12 bits) to the current PC,
      // writes result to Rd. Useful for PC-relative addressing.
      OpcodeAuipc: begin
        reg_we_o    = 1'b1;  // AUIPC always writes to destination register
        imm_type_o  = ImmU;  // Use U-type immediate format
        alu_src_a_o = AluSrcAPc;  // First ALU operand is current PC
        alu_src_b_o = AluSrcBImm;  // Second ALU operand is the 20-bit immediate
        alu_op_o    = AluAdd;  // ALU computes PC + (imm << 12)
        wb_src_o    = WbSrcAlu;  // Write-back the computed value
      end

      // -------- DEFAULT: Unimplemented or Invalid Opcodes --------
      // Instructions with unimplemented opcodes fall through to the default case.
      // Since all outputs have been pre-assigned default values (NOP behavior),
      // these instructions behave as no-ops. A higher-level exception detector
      // should flag these as illegal instructions.
      default: ;  // NOP (all outputs retain their default values)
    endcase
  end

endmodule
