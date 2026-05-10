#include "../include/testbench.hpp"
#include "Vcontrolpath.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <random>

// ========================================================
// Helper: Check if an opcode is part of the RV32I ISA
// ========================================================
bool is_valid_opcode(uint8_t op)
{
    return (op == 0x03 || op == 0x13 || op == 0x17 || op == 0x23 ||
            op == 0x33 || op == 0x37 || op == 0x63 || op == 0x67 || op == 0x6F);
}

int main(int argc, char **argv)
{
    Testbench<Vcontrolpath> tb("trace_controlpath_coverage.vcd");

    std::cout << "\n==> Controlpath Unit Verification (High Coverage & Fuzzing)\n"
              << std::endl;

    // ====================================================
    // TEST 1: R-Type (Differentiating ADD and SUB via Funct7)
    // ====================================================
    std::cout << "\n--- Test 1: R-Type (ADD vs SUB) ---" << std::endl;

    tb.dut->opcode_i = 0x33;
    tb.dut->funct3_i = 0x0;
    tb.dut->funct7_5_i = 0;
    tb.eval();
    CHECK_EQ(1, tb.dut->reg_we_o, "ADD: Reg Write Enable must be asserted (1)");
    CHECK_EQ(0, tb.dut->alu_src_b_o, "ADD: ALU Src B must select rs2 (0)");
    CHECK_EQ(0, tb.dut->alu_op_o, "ADD: ALU Op must be ADD (0)");

    tb.dut->opcode_i = 0x33;
    tb.dut->funct3_i = 0x0;
    tb.dut->funct7_5_i = 1;
    tb.eval();
    CHECK_EQ(8, tb.dut->alu_op_o, "SUB: ALU Op must be SUB (8)");

    // ====================================================
    // TEST 2: I-Type (Logic and Shifts)
    // ====================================================
    std::cout << "\n--- Test 2: I-Type (SRAI) ---" << std::endl;

    tb.dut->opcode_i = 0x13;
    tb.dut->funct3_i = 0x5;
    tb.dut->funct7_5_i = 1;
    tb.eval();
    CHECK_EQ(1, tb.dut->alu_src_b_o, "SRAI: ALU Src B must select Immediate (1)");
    CHECK_EQ(13, tb.dut->alu_op_o, "SRAI: ALU Op must be SRA (13)");

    // ====================================================
    // TEST 3: Memory Access (Loads & Stores)
    // ====================================================
    std::cout << "\n--- Test 3: Load (LW) and Store (SW) ---" << std::endl;

    tb.dut->opcode_i = 0x23;
    tb.dut->funct3_i = 0x2;
    tb.dut->funct7_5_i = 0;
    tb.eval();
    CHECK_EQ(0, tb.dut->reg_we_o, "SW: Must NOT write to register file (0)");
    CHECK_EQ(1, tb.dut->mem_we_o, "SW: Must ENABLE memory write (1)");
    CHECK_EQ(1, tb.dut->imm_type_o, "SW: Immediate format must be S-Type (1)");

    tb.dut->opcode_i = 0x03;
    tb.dut->funct3_i = 0x4;
    tb.dut->funct7_5_i = 0;
    tb.eval();
    CHECK_EQ(1, tb.dut->reg_we_o, "LBU: Must write to register file (1)");
    CHECK_EQ(0, tb.dut->mem_we_o, "LBU: Must NOT write to memory (0)");
    CHECK_EQ(0, tb.dut->mem_size_o, "LBU: Access size must be Byte (0)");
    CHECK_EQ(1, tb.dut->mem_unsigned_o, "LBU: Unsigned extension flag must be set (1)");

    // ====================================================
    // TEST 4: Jumps (JAL vs JALR)
    // ====================================================
    std::cout << "\n--- Test 4: Jumps and Links ---" << std::endl;

    tb.dut->opcode_i = 0x6F;
    tb.dut->funct3_i = 0x0;
    tb.eval();
    CHECK_EQ(1, tb.dut->is_jump_o, "JAL: is_jump signal must be asserted (1)");
    CHECK_EQ(0, tb.dut->jalr_sel_o, "JAL: PC-relative address selector (0)");
    CHECK_EQ(2, tb.dut->wb_src_o, "JAL: Write-back source must be PC+4 (2)");

    tb.dut->opcode_i = 0x67;
    tb.dut->funct3_i = 0x0;
    tb.eval();
    CHECK_EQ(1, tb.dut->is_jump_o, "JALR: is_jump signal must be asserted (1)");
    CHECK_EQ(1, tb.dut->jalr_sel_o, "JALR: rs1-based address selector (1)");

    // ====================================================
    // TEST 5: U-Type (AUIPC and LUI)
    // ====================================================
    std::cout << "\n--- Test 5: LUI and AUIPC ---" << std::endl;

    tb.dut->opcode_i = 0x37;
    tb.eval();
    CHECK_EQ(2, tb.dut->alu_src_a_o, "LUI: ALU In A must be Zero Constant (2)");

    tb.dut->opcode_i = 0x17;
    tb.eval();
    CHECK_EQ(1, tb.dut->alu_src_a_o, "AUIPC: ALU In A must be the PC (1)");

    // ====================================================
    // TEST 6: Branch Conditionals
    // ====================================================
    std::cout << "\n--- Test 6: Branches (Decode) ---" << std::endl;

    tb.dut->opcode_i = 0x63;
    tb.dut->funct3_i = 0x4;
    tb.eval();
    CHECK_EQ(1, tb.dut->is_branch_o, "Branches must trigger the is_branch_o flag");

    // ====================================================
    // TEST 7: Random Stress Testing (Fuzzing)
    // ====================================================
    std::cout << "\n--- Test 7: Fuzzing Engine (Safety Invariants) ---" << std::endl;

    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist_32(0, 0xFFFFFFFF);
    std::uniform_int_distribution<uint8_t> dist_7(0, 0x7F);

    int fuzz_iterations = 10000;
    int illegal_hits = 0;

    for (int i = 0; i < fuzz_iterations; i++)
    {
        uint8_t rand_op = dist_7(rng);
        tb.dut->opcode_i = rand_op;
        tb.dut->funct3_i = dist_32(rng) & 0x7;
        tb.dut->funct7_5_i = dist_32(rng) & 0x1;
        tb.eval();

        if (!is_valid_opcode(rand_op))
        {
            illegal_hits++;
            if (tb.dut->reg_we_o != 0 || tb.dut->mem_we_o != 0 || tb.dut->is_branch_o != 0 || tb.dut->is_jump_o != 0)
            {
                std::cerr << "❌ FUZZING FATAL ERROR: Illegal opcode triggered state modification!" << std::endl;
                return 1;
            }
        }
    }

    std::cout << "  ✓ Fuzzing completed safely for " << fuzz_iterations << " iterations.\n";
    std::cout << "\n✅ Controlpath High Coverage & Fuzzing test PASSED!\n"
              << std::endl;
    return 0;
}