#include <iostream>
#include <iomanip>
#include <cstdint>
#include <random>
#include <vector>
#include "Valu.h"
#include "../include/testbench.hpp"

// Enums reflecting the ALU Opcodes from SystemVerilog
enum AluOp
{
    AluAdd = 0b0000,
    AluSub = 0b1000,
    AluSll = 0b0001,
    AluSlt = 0b0010,
    AluSltu = 0b0011,
    AluXor = 0b0100,
    AluSrl = 0b0101,
    AluSra = 0b1101,
    AluOr = 0b0110,
    AluAnd = 0b0111
};

// ============================================================================
// GOLDEN MODEL - C++ reference model for the ALU
// ============================================================================

// This function implements the same logic as the ALU hardware module, but in C++.
// The result of the hardware should match this function bit-for-bit for the same inputs.

uint32_t golden_model_alu(AluOp opcode, uint32_t a, uint32_t b)
{

    // Cast to signed integers for operations that require signed interpretation (SLT and SRA)
    int32_t a_s = static_cast<int32_t>(a);
    int32_t b_s = static_cast<int32_t>(b);

    // The shift amount is determined by the lower 5 bits of B, as per RISC-V spec and our hardware design.
    uint32_t shamt = b & 0x1F;

    switch (opcode)
    {
    case AluAdd:
        return a + b;
    case AluSub:
        return a - b;
    case AluSll:
        return a << shamt;
    case AluSlt:
        return (a_s < b_s) ? 1 : 0;
    case AluSltu:
        return (a < b) ? 1 : 0;
    case AluXor:
        return a ^ b;
    case AluSrl:
        return a >> shamt;

    // C++ guarantees that right shifts of signed types (int32_t) are arithmetic (preserve the sign bit),
    // just like Python and SystemVerilog.
    case AluSra:
        return static_cast<uint32_t>(a_s >> shamt);
    case AluOr:
        return a | b;
    case AluAnd:
        return a & b;
    default:
        return 0;
    }
}

// ============================================================================
// AUTOMATED TESTBENCH
// ============================================================================

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Testbench<Valu> tb("build/alu.vcd");

    std::cout << "\n==> Iniciando verificacao da ALU (Golden Model & Fuzzing)...\n\n";

    // ------------------------------------------------------------------------
    // Test 1: Basic Operations (ADD / SUB)
    // ------------------------------------------------------------------------

    std::cout << "--- Testando Operacoes Basicas ---\n";
    tb.dut->alu_op_i = AluAdd;
    tb.dut->a_i = 10;
    tb.dut->b_i = 32;
    tb.eval();
    CHECK_EQ(42, tb.dut->result_o, "ADD Basico: 10 + 32");

    tb.dut->alu_op_i = AluSub;
    tb.dut->a_i = 10;
    tb.dut->b_i = 10;
    tb.eval();
    CHECK_EQ(0, tb.dut->result_o, "SUB Basico: 10 - 10");

    // ------------------------------------------------------------------------
    // Test 2: Shift Operations
    // ------------------------------------------------------------------------

    std::cout << "\n--- Testando Shifts Específicos ---\n";
    uint32_t val_neg = 0xFFFFFFF0; // -16
    uint32_t shift = 4;

    tb.dut->a_i = val_neg;
    tb.dut->b_i = shift;

    // SRA
    tb.dut->alu_op_i = AluSra;
    tb.eval();
    CHECK_EQ(golden_model_alu(AluSra, val_neg, shift), tb.dut->result_o, "SRA: Extensao de sinal");

    // SRL
    tb.dut->alu_op_i = AluSrl;
    tb.eval();
    CHECK_EQ(golden_model_alu(AluSrl, val_neg, shift), tb.dut->result_o, "SRL: Preenchimento com zeros");

    // SLL
    tb.dut->alu_op_i = AluSll;
    tb.eval();
    CHECK_EQ(golden_model_alu(AluSll, val_neg, shift), tb.dut->result_o, "SLL: Shift para esquerda");

    // ------------------------------------------------------------------------
    // Test 3: Specific Comparisons (SLT / SLTU)
    // ------------------------------------------------------------------------

    std::cout << "\n--- Testando Comparacoes (Corner Cases) ---\n";
    tb.dut->a_i = 0xFFFFFFFF; // -1 (Signed) ou 4294967295 (Unsigned)
    tb.dut->b_i = 1;

    tb.dut->alu_op_i = AluSlt;
    tb.eval();
    CHECK_EQ(golden_model_alu(AluSlt, tb.dut->a_i, tb.dut->b_i), tb.dut->result_o, "SLT (Signed): -1 < 1");

    tb.dut->alu_op_i = AluSltu;
    tb.eval();
    CHECK_EQ(golden_model_alu(AluSltu, tb.dut->a_i, tb.dut->b_i), tb.dut->result_o, "SLTU (Unsigned): MAX_UINT < 1");

    // ------------------------------------------------------------------------
    // Test 4: FUZZ TESTING (Stress Testing - 10.000 iterations)
    // ------------------------------------------------------------------------

    std::cout << "\n--- Iniciando Teste de Estresse (Fuzzing) ---\n";

    // Fixed seed for reproducibility in CI environments
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist32(0, 0xFFFFFFFF);

    std::vector<AluOp> ops = {
        AluAdd, AluSub, AluSll, AluSlt, AluSltu,
        AluXor, AluSrl, AluSra, AluOr, AluAnd};

    // Primary, in COCOTB Python Framework, we had 1.000 iterations, but since C++ is much faster,
    // we can easily increase this to 10.000 iterations for a more thorough fuzzing without significantly impacting test time.

    const int NUM_ITERATIONS = 10000;
    bool fuzz_pass = true;

    for (int i = 0; i < NUM_ITERATIONS; i++)
    {
        // Generate random vectors for A, B, and a random ALU operation
        uint32_t a = dist32(rng);
        uint32_t b = dist32(rng);
        AluOp op = ops[rng() % ops.size()];

        // Apply to DUT (Hardware)
        tb.dut->a_i = a;
        tb.dut->b_i = b;
        tb.dut->alu_op_i = op;
        tb.eval();

        // Calculate Golden Model (Software) and compare results bit-by-bit
        uint32_t expected = golden_model_alu(op, a, b);
        uint32_t got = tb.dut->result_o;

        // If there's a mismatch, print detailed information about the failure and break the loop
        if (expected != got)
        {

            std::cerr << "\n❌ FALHA NO FUZZING na iteracao " << i << "\n"
                      << "   Opcode: 0x" << std::hex << op << "\n"
                      << "   A_i:    0x" << a << "\n"
                      << "   B_i:    0x" << b << "\n"
                      << "   Esp:    0x" << expected << "\n"
                      << "   Obt:    0x" << got << std::dec << "\n\n";
            fuzz_pass = false;

            // Break on first failure to avoid flooding the output
            break;
        }
    }

    if (fuzz_pass)
    {
        std::cout << "  ✓ " << NUM_ITERATIONS << " vetores aleatorios verificados com sucesso!\n";
        std::cout << "\n✅ Teste da ALU concluido com sucesso.\n\n";
    }

    return fuzz_pass ? 0 : 1;
}