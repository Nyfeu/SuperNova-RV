#include "../include/testbench.hpp"
#include "Vexecute_stage.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <random>

// Enum mappings from supernova_pkg.sv
enum AluSrcA
{
    AluSrcARs1 = 0,
    AluSrcAPc = 1,
    AluSrcAZero = 2
};
enum AluSrcB
{
    AluSrcBRs2 = 0,
    AluSrcBImm = 1
};
enum AluOp
{
    AluAdd = 0,
    AluSll = 1,
    AluSlt = 2,
    AluSltu = 3,
    AluXor = 4,
    AluSrl = 5,
    AluOr = 6,
    AluAnd = 7,
    AluSub = 8,
    AluSra = 13
};

// --------------------------------------------------------
// Golden Model: ALU & Data Routing
// --------------------------------------------------------
uint32_t golden_alu(uint32_t a, uint32_t b, AluOp op)
{
    uint32_t shamt = b & 0x1F;
    switch (op)
    {
    case AluAdd:
        return a + b;
    case AluSub:
        return a - b;
    case AluSll:
        return a << shamt;
    case AluSlt:
        return ((int32_t)a < (int32_t)b) ? 1 : 0;
    case AluSltu:
        return (a < b) ? 1 : 0;
    case AluXor:
        return a ^ b;
    case AluSrl:
        return a >> shamt;
    case AluSra:
        return (uint32_t)((int32_t)a >> shamt);
    case AluOr:
        return a | b;
    case AluAnd:
        return a & b;
    default:
        return 0;
    }
}

int main(int argc, char **argv)
{
    Testbench<Vexecute_stage> tb("trace_execute_stage.vcd");
    int failures = 0;

    std::cout << "\n==> Starting Execute Stage verification..." << std::endl;

    // --------------------------------------------------------
    // Directed Test 1: R-Type (Rs1 + Rs2)
    // --------------------------------------------------------
    std::cout << "\n--- Directed Tests: R-Type Operations ---" << std::endl;
    tb.dut->pc_i = 0x00001000;
    tb.dut->rs1_data_i = 150;
    tb.dut->rs2_data_i = 50;
    tb.dut->imm_i = 0; // Not used

    tb.dut->alu_src_a_i = AluSrcARs1;
    tb.dut->alu_src_b_i = AluSrcBRs2;
    tb.dut->alu_op_i = AluSub;
    tb.eval();

    if (tb.dut->alu_result_o == 100)
    {
        std::cout << "  \033[1;32m✓\033[0m Mux Rs1/Rs2 + ALU SUB routed correctly." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Mux Rs1/Rs2 routing failed. Expected 100, got: " << tb.dut->alu_result_o << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Directed Test 2: I-Type (Rs1 + Imm)
    // --------------------------------------------------------
    std::cout << "\n--- Directed Tests: I-Type Operations ---" << std::endl;
    tb.dut->rs1_data_i = 1000;
    tb.dut->imm_i = 500;

    tb.dut->alu_src_a_i = AluSrcARs1;
    tb.dut->alu_src_b_i = AluSrcBImm;
    tb.dut->alu_op_i = AluAdd;
    tb.eval();

    if (tb.dut->alu_result_o == 1500)
    {
        std::cout << "  \033[1;32m✓\033[0m Mux Rs1/Imm + ALU ADD routed correctly." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Mux Rs1/Imm routing failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Directed Test 3: PC-Relative (PC + Imm) e.g., AUIPC
    // --------------------------------------------------------
    std::cout << "\n--- Directed Tests: PC-Relative Operations ---" << std::endl;
    tb.dut->pc_i = 0x00004000;
    tb.dut->imm_i = 0x00001000;

    tb.dut->alu_src_a_i = AluSrcAPc;
    tb.dut->alu_src_b_i = AluSrcBImm;
    tb.dut->alu_op_i = AluAdd;
    tb.eval();

    if (tb.dut->alu_result_o == 0x00005000)
    {
        std::cout << "  \033[1;32m✓\033[0m AUIPC (PC + Imm) routed correctly." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m AUIPC routing failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Directed Test 4: Jump & Branch Addresses
    // --------------------------------------------------------
    std::cout << "\n--- Directed Tests: Target & JALR Address ---" << std::endl;
    tb.dut->pc_i = 0x00001000;
    tb.dut->rs1_data_i = 0x00002005; // Note the odd address
    tb.dut->imm_i = 0x00000010;

    tb.dut->alu_src_a_i = AluSrcARs1;
    tb.dut->alu_src_b_i = AluSrcBImm;
    tb.dut->alu_op_i = AluAdd;
    tb.eval();

    // Target = PC + Imm
    if (tb.dut->target_addr_o == 0x00001010)
    {
        std::cout << "  \033[1;32m✓\033[0m Dedicated Target Adder (PC + Imm) verified." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Target Adder failed." << std::endl;
        failures++;
    }

    // JALR = (Rs1 + Imm) with LSB cleared to 0
    // Rs1 + Imm = 0x2005 + 0x10 = 0x2015. Masking LSB makes it 0x2014.
    if (tb.dut->jalr_addr_o == 0x00002014)
    {
        std::cout << "  \033[1;32m✓\033[0m JALR address LSB masking verified." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m JALR mask failed. Got: 0x" << std::hex << tb.dut->jalr_addr_o << std::dec << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // FUZZING: Fuzzing the Execute Combinations
    // --------------------------------------------------------
    std::cout << "\n--- Fuzzing (Stress Test: Routing & ALU) ---" << std::endl;
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist_32(0, 0xFFFFFFFF);
    std::uniform_int_distribution<int> dist_srcA(0, 2);
    std::uniform_int_distribution<int> dist_srcB(0, 1);

    // Lista de operações válidas da ALU
    std::vector<AluOp> valid_ops = {AluAdd, AluSll, AluSlt, AluSltu, AluXor, AluSrl, AluOr, AluAnd, AluSub, AluSra};
    std::uniform_int_distribution<int> dist_op(0, valid_ops.size() - 1);

    const int NUM_FUZZ_TESTS = 10000;
    int fuzz_failures = 0;

    for (int i = 0; i < NUM_FUZZ_TESTS; i++)
    {
        uint32_t pc = dist_32(rng);
        uint32_t rs1 = dist_32(rng);
        uint32_t rs2 = dist_32(rng);
        uint32_t imm = dist_32(rng);

        AluSrcA src_a = static_cast<AluSrcA>(dist_srcA(rng));
        AluSrcB src_b = static_cast<AluSrcB>(dist_srcB(rng));
        AluOp op = valid_ops[dist_op(rng)];

        tb.dut->pc_i = pc;
        tb.dut->rs1_data_i = rs1;
        tb.dut->rs2_data_i = rs2;
        tb.dut->imm_i = imm;
        tb.dut->alu_src_a_i = src_a;
        tb.dut->alu_src_b_i = src_b;
        tb.dut->alu_op_i = op;

        tb.eval();

        // Calcular Golden Result
        uint32_t exp_a = (src_a == AluSrcARs1) ? rs1 : (src_a == AluSrcAPc) ? pc
                                                                            : 0;
        uint32_t exp_b = (src_b == AluSrcBImm) ? imm : rs2;
        uint32_t exp_alu_res = golden_alu(exp_a, exp_b, op);
        uint32_t exp_target = pc + imm;
        uint32_t exp_jalr = exp_alu_res & 0xFFFFFFFE; // Zera o LSB

        if (tb.dut->alu_result_o != exp_alu_res ||
            tb.dut->target_addr_o != exp_target ||
            tb.dut->jalr_addr_o != exp_jalr)
        {

            std::cerr << "  \033[1;31m❌\033[0m Fuzzing failed at iteration " << i << std::endl;
            fuzz_failures++;
            if (fuzz_failures > 5)
                break;
        }
    }

    if (fuzz_failures == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m " << NUM_FUZZ_TESTS << " random execute cycles routed and computed successfully!" << std::endl;
    }
    else
    {
        failures += fuzz_failures;
    }

    std::cout << std::endl;
    if (failures == 0)
    {
        std::cout << "✅ Execute Stage tests PASSED.\n"
                  << std::endl;
    }
    else
    {
        std::cout << "❌ Execute Stage tests FAILED with " << failures << " error(s).\n"
                  << std::endl;
    }

    return failures == 0 ? 0 : 1;
}