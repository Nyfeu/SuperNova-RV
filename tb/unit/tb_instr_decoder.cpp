#include "../include/testbench.hpp"
#include "Vinstr_decoder.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <random>

// ============================================================================
// Enum Mappings from supernova_pkg.sv
// ============================================================================
// These enums mirror the SystemVerilog package definitions to enable direct
// comparison between hardware outputs and C++ expected values.

enum ImmType
{
    ImmI = 0, // I-type immediate (12-bit signed)
    ImmS = 1, // S-type immediate (12-bit signed, store format)
    ImmB = 2, // B-type immediate (12-bit signed, branch format)
    ImmU = 3, // U-type immediate (20-bit upper immediate)
    ImmJ = 4  // J-type immediate (20-bit signed, jump format)
};

enum AluSrcA
{
    AluSrcARs1 = 0, // First ALU operand from Register Rs1
    AluSrcAPc = 1,  // First ALU operand from Program Counter (PC)
    AluSrcAZero = 2 // First ALU operand is zero (constant)
};

enum AluSrcB
{
    AluSrcBRs2 = 0, // Second ALU operand from Register Rs2
    AluSrcBImm = 1  // Second ALU operand from Immediate value
};

enum AluOp
{
    AluAdd = 0,  // Addition
    AluSll = 1,  // Shift Left Logical
    AluSlt = 2,  // Set Less Than (signed)
    AluSltu = 3, // Set Less Than Unsigned
    AluXor = 4,  // Bitwise XOR
    AluSrl = 5,  // Shift Right Logical
    AluOr = 6,   // Bitwise OR
    AluAnd = 7,  // Bitwise AND
    AluSub = 8,  // Subtraction (R-type only)
    AluSra = 13  // Shift Right Arithmetic
};

enum MemSize
{
    MemSizeByte = 0, // 8-bit memory access (byte)
    MemSizeHalf = 1, // 16-bit memory access (half-word)
    MemSizeWord = 2  // 32-bit memory access (word)
};

enum WbSrc
{
    WbSrcAlu = 0, // Write-back source: ALU result
    WbSrcMem = 1, // Write-back source: Memory data (load result)
    WbSrcPc = 2   // Write-back source: PC+4 (jump return address)
};

// ============================================================================
// Struct to hold expected decoder outputs for a given instruction
// ============================================================================
// This structure encapsulates the complete set of control signals that the
// decoder should produce for a given instruction. It is used for both the
// golden model and for test result validation.
struct DecoderExpected
{
    bool reg_we;
    ImmType imm_type;
    AluSrcA alu_src_a;
    AluSrcB alu_src_b;
    AluOp alu_op;
    bool mem_we;
    MemSize mem_size;
    bool mem_unsigned;
    WbSrc wb_src;
    bool is_branch;
    bool is_jump;
};

// ============================================================================
// Golden Model: Software Implementation of the Instruction Decoder
// ============================================================================
// This function implements the exact same decoding logic as the hardware
// decoder module (instr_decoder.sv). It serves as the reference for validating
// the hardware implementation. By keeping the golden model in C++, we can
// easily trace through the decoding logic and verify correctness.
//
// INPUT:  32-bit instruction encoding (RV32I format)
// OUTPUT: DecoderExpected structure with all control signals
//
// The golden model follows the RV32I specification and mimics the flat
// decoding strategy: extract opcode, funct3, and funct7[5], then decode
// all control signals in one stage.
DecoderExpected golden_decoder(uint32_t instr)
{
    // Initialize with default values (NOP behavior for unimplemented instructions)
    DecoderExpected exp = {false, ImmI, AluSrcARs1, AluSrcBRs2, AluAdd, false, MemSizeWord, false, WbSrcAlu, false, false};

    uint32_t opcode = instr & 0x7F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    bool bit30 = (instr >> 30) & 1;

    // Decode based on opcode
    switch (opcode)
    {
    case 0x33: // R-Type: Register-to-Register operations (ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU)
        // All R-type instructions write to the destination register (Rd)
        // Both operands come from registers (Rs1 and Rs2)
        exp.reg_we = true;
        exp.wb_src = WbSrcAlu;
        if (funct3 == 0)
            exp.alu_op = bit30 ? AluSub : AluAdd;
        else if (funct3 == 1)
            exp.alu_op = AluSll;
        else if (funct3 == 2)
            exp.alu_op = AluSlt;
        else if (funct3 == 3)
            exp.alu_op = AluSltu;
        else if (funct3 == 4)
            exp.alu_op = AluXor;
        else if (funct3 == 5)
            exp.alu_op = bit30 ? AluSra : AluSrl;
        else if (funct3 == 6)
            exp.alu_op = AluOr;
        else if (funct3 == 7)
            exp.alu_op = AluAnd;
        break;
    case 0x13: // I-Type: Immediate arithmetic and logic operations (ADDI, SLLI, SRLI, SRAI, ANDI, ORI, XORI, SLTI, SLTIU)
        // All I-type instructions write to the destination register (Rd)
        // First operand is from register (Rs1), second is from immediate (12-bit signed)
        exp.reg_we = true;
        exp.imm_type = ImmI;
        exp.alu_src_b = AluSrcBImm;
        exp.wb_src = WbSrcAlu;
        if (funct3 == 0)
            exp.alu_op = AluAdd;
        else if (funct3 == 1)
            exp.alu_op = AluSll;
        else if (funct3 == 2)
            exp.alu_op = AluSlt;
        else if (funct3 == 3)
            exp.alu_op = AluSltu;
        else if (funct3 == 4)
            exp.alu_op = AluXor;
        else if (funct3 == 5)
            exp.alu_op = bit30 ? AluSra : AluSrl;
        else if (funct3 == 6)
            exp.alu_op = AluOr;
        else if (funct3 == 7)
            exp.alu_op = AluAnd;
        break;
    case 0x03: // Load: Load data from memory (LB, LH, LW, LBU, LHU)
        // Load instructions write the loaded value to the destination register (Rd)
        // Memory address is computed as Rs1 + 12-bit immediate
        // Funct3 encodes both size (byte, half-word, word) and signedness (signed or unsigned)
        exp.reg_we = true;
        exp.imm_type = ImmI;
        exp.alu_src_b = AluSrcBImm;
        exp.alu_op = AluAdd;
        exp.wb_src = WbSrcMem;
        if (funct3 == 0)
        {
            exp.mem_size = MemSizeByte;
            exp.mem_unsigned = false;
        }
        else if (funct3 == 1)
        {
            exp.mem_size = MemSizeHalf;
            exp.mem_unsigned = false;
        }
        else if (funct3 == 2)
        {
            exp.mem_size = MemSizeWord;
            exp.mem_unsigned = false;
        }
        else if (funct3 == 4)
        {
            exp.mem_size = MemSizeByte;
            exp.mem_unsigned = true;
        }
        else if (funct3 == 5)
        {
            exp.mem_size = MemSizeHalf;
            exp.mem_unsigned = true;
        }
        break;
    case 0x23: // Store: Write data to memory (SB, SH, SW)
        // Store instructions do NOT write to registers (reg_we = false)
        // Memory address is computed as Rs1 + 12-bit signed immediate (S-type format)
        // The value to store comes from register Rs2
        exp.imm_type = ImmS;
        exp.alu_src_b = AluSrcBImm;
        exp.alu_op = AluAdd;
        exp.mem_we = true;
        if (funct3 == 0)
            exp.mem_size = MemSizeByte;
        else if (funct3 == 1)
            exp.mem_size = MemSizeHalf;
        else if (funct3 == 2)
            exp.mem_size = MemSizeWord;
        break;
    case 0x63: // Branch: Conditional branch instructions (BEQ, BNE, BLT, BGE, BLTU, BGEU)
        // Branch instructions do NOT write to registers (reg_we = false)
        // Funct3 encodes the branch condition
        // The actual comparison happens in the Branch Unit (not in this decoder)
        exp.imm_type = ImmB;
        exp.alu_op = AluAdd;
        exp.is_branch = true;
        break;
    case 0x6F: // JAL: Jump And Link (unconditional jump with return address save)
        // JAL always writes the return address (PC+4) to the destination register (Rd)
        // Target address is computed as PC + 20-bit signed immediate
        exp.reg_we = true;
        exp.imm_type = ImmJ;
        exp.alu_src_a = AluSrcAPc;
        exp.alu_src_b = AluSrcBImm;
        exp.alu_op = AluAdd;
        exp.wb_src = WbSrcPc;
        exp.is_jump = true;
        break;
    case 0x67: // JALR: Jump And Link Register (indirect jump with return address save)
        // JALR always writes the return address (PC+4) to the destination register (Rd)
        // Target address is computed as Rs1 + 12-bit signed immediate
        exp.reg_we = true;
        exp.imm_type = ImmI;
        exp.alu_src_b = AluSrcBImm;
        exp.alu_op = AluAdd;
        exp.wb_src = WbSrcPc;
        exp.is_jump = true;
        break;
    case 0x37: // LUI: Load Upper Immediate
        // LUI loads a 20-bit immediate into the upper 20 bits of Rd (bits 31:12)
        // Lower 12 bits of Rd are zeroed
        // Implementation: 0 + (imm << 12) = imm << 12
        exp.reg_we = true;
        exp.imm_type = ImmU;
        exp.alu_src_a = AluSrcAZero;
        exp.alu_src_b = AluSrcBImm;
        exp.alu_op = AluAdd;
        exp.wb_src = WbSrcAlu;
        break;
    case 0x17: // AUIPC: Add Upper Immediate to PC
        // AUIPC adds a 20-bit immediate (shifted left by 12 bits) to the current PC
        // Result is written to Rd
        // Useful for PC-relative addressing (e.g., position-independent code)
        exp.reg_we = true;
        exp.imm_type = ImmU;
        exp.alu_src_a = AluSrcAPc;
        exp.alu_src_b = AluSrcBImm;
        exp.alu_op = AluAdd;
        exp.wb_src = WbSrcAlu;
        break;
    }
    return exp;
}

// ============================================================================
// Universal Signal Validation Function
// ============================================================================
// This function compares all decoder outputs against expected values.
// It performs a bit-by-bit comparison of all 11 control signal outputs.
//
// RETURNS: true if all signals match expected values, false otherwise
bool check_signals(Testbench<Vinstr_decoder> &tb, DecoderExpected exp)
{
    return (tb.dut->reg_we_o == exp.reg_we) &&
           (tb.dut->imm_type_o == exp.imm_type) &&
           (tb.dut->alu_src_a_o == exp.alu_src_a) &&
           (tb.dut->alu_src_b_o == exp.alu_src_b) &&
           (tb.dut->alu_op_o == exp.alu_op) &&
           (tb.dut->mem_we_o == exp.mem_we) &&
           (tb.dut->mem_size_o == exp.mem_size) &&
           (tb.dut->mem_unsigned_o == exp.mem_unsigned) &&
           (tb.dut->wb_src_o == exp.wb_src) &&
           (tb.dut->is_branch_o == exp.is_branch) &&
           (tb.dut->is_jump_o == exp.is_jump);
}

// ============================================================================
// Directed Test Runner
// ============================================================================
// This function executes a single directed test case:
// 1. Slices the 32-bit instruction into opcode, funct3, and funct7[5]
// 2. Applies these to the DUT
// 3. Evaluates the combinational logic
// 4. Checks all outputs against expected values
// 5. Reports pass/fail with colored output
//
// PARAMETERS:
//   tb   : Reference to the testbench object
//   desc : Human-readable description of the test (e.g., "ADD x1, x2, x3")
//   instr: 32-bit instruction encoding
//   exp  : Expected control signals
//
// RETURNS: true if test passed, false if any signal mismatched
bool run_directed_test(Testbench<Vinstr_decoder> &tb, const std::string &desc, uint32_t instr, DecoderExpected exp)
{
    // Extract instruction fields (same slicing as hardware decoder)
    tb.dut->opcode_i = instr & 0x7F;          // Bits 6:0
    tb.dut->funct3_i = (instr >> 12) & 0x7;   // Bits 14:12
    tb.dut->funct7_5_i = (instr >> 30) & 0x1; // Bit 30

    // Evaluate combinational logic
    tb.eval();

    // Check if outputs match expectations
    if (check_signals(tb, exp))
    {
        // Test passed: print green checkmark and description
        std::cout << "  \033[1;32m✓\033[0m " << desc << std::endl;
        return true;
    }
    else
    {
        // Test failed: print red X mark with description
        std::cerr << "  \033[1;31m❌\033[0m " << desc << " (Mismatch in control signals)" << std::endl;
        return false;
    }
}

// ============================================================================
// Main Test Program
// ============================================================================

int main(int argc, char **argv)
{
    // Create testbench instance with VCD tracing for debugging
    Testbench<Vinstr_decoder> tb("trace_instr_decoder.vcd");
    int failures = 0;

    std::cout << "\n==> Starting Instruction Decoder verification..." << std::endl;

    // ====================================================================
    // DIRECTED TESTS: Test specific instruction encodings
    // ====================================================================
    // These tests verify that the decoder correctly handles representative
    // examples from each instruction category.

    std::cout << "\n--- Arithmetic / R-Type Instructions ---" << std::endl;
    // Test: ADD x1, x2, x3
    // Encoding: funct7=0x00, rs2=3, rs1=2, funct3=0, rd=1, opcode=0x33
    DecoderExpected exp_add = {true, ImmI, AluSrcARs1, AluSrcBRs2, AluAdd, false, MemSizeWord, false, WbSrcAlu, false, false};
    if (!run_directed_test(tb, "ADD x1, x2, x3", 0x003100b3, exp_add))
        failures++;

    // Test: SUB x4, x5, x6
    // Encoding: funct7=0x20, rs2=6, rs1=5, funct3=0, rd=4, opcode=0x33
    DecoderExpected exp_sub = {true, ImmI, AluSrcARs1, AluSrcBRs2, AluSub, false, MemSizeWord, false, WbSrcAlu, false, false};
    if (!run_directed_test(tb, "SUB x4, x5, x6", 0x40628233, exp_sub))
        failures++;

    std::cout << "\n--- Immediate / I-Type Instructions ---" << std::endl;
    // Test: ADDI x1, x0, 10
    // Encoding: imm=10, rs1=0, funct3=0, rd=1, opcode=0x13
    DecoderExpected exp_addi = {true, ImmI, AluSrcARs1, AluSrcBImm, AluAdd, false, MemSizeWord, false, WbSrcAlu, false, false};
    if (!run_directed_test(tb, "ADDI x1, x0, 10", 0x00a00093, exp_addi))
        failures++;

    // Test: SRAI x2, x3, 4
    // Encoding: funct7=0x20, rs2=4 (shamt), rs1=3, funct3=5, rd=2, opcode=0x13
    DecoderExpected exp_srai = {true, ImmI, AluSrcARs1, AluSrcBImm, AluSra, false, MemSizeWord, false, WbSrcAlu, false, false};
    if (!run_directed_test(tb, "SRAI x2, x3, 4", 0x4041d113, exp_srai))
        failures++;

    std::cout << "\n--- Memory Instructions ---" << std::endl;
    // Test: LW x5, 4(x1)
    // Encoding: imm=4, rs1=1, funct3=2, rd=5, opcode=0x03
    DecoderExpected exp_lw = {true, ImmI, AluSrcARs1, AluSrcBImm, AluAdd, false, MemSizeWord, false, WbSrcMem, false, false};
    if (!run_directed_test(tb, "LW x5, 4(x1)", 0x0040a283, exp_lw))
        failures++;

    // Test: SB x6, 8(x2)
    // Encoding: imm[11:5]=0, rs2=6, rs1=2, funct3=0, imm[4:0]=8, opcode=0x23
    DecoderExpected exp_sb = {false, ImmS, AluSrcARs1, AluSrcBImm, AluAdd, true, MemSizeByte, false, WbSrcAlu, false, false};
    if (!run_directed_test(tb, "SB x6, 8(x2)", 0x00610423, exp_sb))
        failures++;

    std::cout << "\n--- Control Flow Instructions ---" << std::endl;
    // Test: BEQ x1, x2, 16
    // Encoding: imm[12|10:5]=0, rs2=2, rs1=1, funct3=0, imm[4:1|11]=8, opcode=0x63
    DecoderExpected exp_beq = {false, ImmB, AluSrcARs1, AluSrcBRs2, AluAdd, false, MemSizeWord, false, WbSrcAlu, true, false};
    if (!run_directed_test(tb, "BEQ x1, x2, 16", 0x01008863, exp_beq))
        failures++;

    // Test: JAL x0, 2048
    // Encoding: imm[20|10:1|11|19:12]=0x80000, rd=0, opcode=0x6F
    DecoderExpected exp_jal = {true, ImmJ, AluSrcAPc, AluSrcBImm, AluAdd, false, MemSizeWord, false, WbSrcPc, false, true};
    if (!run_directed_test(tb, "JAL x0, 2048", 0x8000006f, exp_jal))
        failures++;

    std::cout << "\n--- Upper Immediate Instructions ---" << std::endl;
    // Test: LUI x5, 0x12345
    // Encoding: imm=0x12345, rd=5, opcode=0x37
    DecoderExpected exp_lui = {true, ImmU, AluSrcAZero, AluSrcBImm, AluAdd, false, MemSizeWord, false, WbSrcAlu, false, false};
    if (!run_directed_test(tb, "LUI x5, 0x12345", 0x123452b7, exp_lui))
        failures++;

    // ====================================================================
    // FUZZING (Randomized Stress Testing)
    // ====================================================================
    // Generate random instruction encodings and verify they decode correctly
    // according to the golden model. This catches edge cases and unexpected
    // behaviors that might not be covered by directed tests.
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist_instr(0, 0xFFFFFFFF);

    const int NUM_FUZZ_TESTS = 10000;
    int fuzz_failures = 0;

    for (int i = 0; i < NUM_FUZZ_TESTS; i++)
    {
        // Generate a random 32-bit instruction
        uint32_t rand_instr = dist_instr(rng);

        // Slice the instruction into opcode, funct3, and funct7[5] before applying to DUT
        // This mimics how the hardware would extract these fields from the full instruction
        tb.dut->opcode_i = rand_instr & 0x7F;
        tb.dut->funct3_i = (rand_instr >> 12) & 0x7;
        tb.dut->funct7_5_i = (rand_instr >> 30) & 0x1;

        // Evaluate the combinational logic
        tb.eval();

        // Get the expected outputs from the golden model
        DecoderExpected exp = golden_decoder(rand_instr);

        // Compare hardware outputs with golden model expectations
        if (!check_signals(tb, exp))
        {
            std::cerr << "  \033[1;31m❌\033[0m Fuzzing iteration " << i << " failed." << std::endl;
            std::cerr << "     Random Instruction: 0x" << std::hex << std::setfill('0') << std::setw(8) << rand_instr << std::dec << std::endl;
            fuzz_failures++;
            failures++;
            // Stop after 5 failures to avoid excessive output
            if (fuzz_failures > 5)
                break;
        }
    }

    // Report fuzzing results
    if (fuzz_failures == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m All " << NUM_FUZZ_TESTS << " randomized instructions decoded correctly!" << std::endl;
    }

    // ====================================================================
    // TEST SUMMARY AND FINAL REPORT
    // ====================================================================

    std::cout << std::endl;
    if (failures == 0)
    {
        std::cout << "✅ Instruction Decoder tests PASSED.\n"
                  << std::endl;
    }
    else
    {
        std::cout << "❌ Instruction Decoder tests FAILED with " << failures << " error(s).\n"
                  << std::endl;
    }

    // Return exit code: 0 for success, 1 for failure (used by test harness)
    return failures == 0 ? 0 : 1;
}