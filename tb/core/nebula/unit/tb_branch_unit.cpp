#include "../include/testbench.hpp"
#include "Vbranch_unit.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <random>

// --------------------------------------------------------
// Golden Model (C++)
// --------------------------------------------------------
bool golden_branch(uint32_t rs1, uint32_t rs2, bool is_branch, uint8_t funct3)
{
    if (!is_branch)
        return false;

    int32_t s_rs1 = static_cast<int32_t>(rs1);
    int32_t s_rs2 = static_cast<int32_t>(rs2);

    switch (funct3)
    {
    case 0:
        return (rs1 == rs2); // BEQ
    case 1:
        return (rs1 != rs2); // BNE
    case 4:
        return (s_rs1 < s_rs2); // BLT
    case 5:
        return (s_rs1 >= s_rs2); // BGE
    case 6:
        return (rs1 < rs2); // BLTU
    case 7:
        return (rs1 >= rs2); // BGEU
    default:
        return false;
    }
}

// --------------------------------------------------------
// Test Helper
// --------------------------------------------------------
bool run_directed_test(Testbench<Vbranch_unit> &tb, const std::string &desc,
                       uint32_t rs1, uint32_t rs2, bool is_branch, uint8_t funct3, bool expected)
{
    tb.dut->rs1_data_i = rs1;
    tb.dut->rs2_data_i = rs2;
    tb.dut->is_branch_i = is_branch;
    tb.dut->funct3_i = funct3;
    tb.eval();

    if (tb.dut->branch_taken_o == expected)
    {
        std::cout << "  \033[1;32m✓\033[0m " << desc << std::endl;
        return true;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m " << desc << " | Expected: " << expected << " Got: " << (int)tb.dut->branch_taken_o << std::endl;
        return false;
    }
}

int main(int argc, char **argv)
{
    Testbench<Vbranch_unit> tb("trace_branch_unit.vcd");
    int failures = 0;

    std::cout << "\n==> Starting Branch Unit verification..." << std::endl;

    std::cout << "\n--- Directed Tests: Equality ---" << std::endl;
    if (!run_directed_test(tb, "BEQ: 10 == 10 (Take)", 10, 10, true, 0, true))
        failures++;
    if (!run_directed_test(tb, "BEQ: 10 == 20 (Skip)", 10, 20, true, 0, false))
        failures++;
    if (!run_directed_test(tb, "BNE: 10 != 20 (Take)", 10, 20, true, 1, true))
        failures++;

    std::cout << "\n--- Directed Tests: Signed vs Unsigned ---" << std::endl;
    // 0xFFFFFFFF is -1 in Signed, but MAX_UINT in Unsigned
    if (!run_directed_test(tb, "BLT (Signed): -1 < 1 (Take)", 0xFFFFFFFF, 1, true, 4, true))
        failures++;
    if (!run_directed_test(tb, "BLTU (Unsigned): MAX_UINT < 1 (Skip)", 0xFFFFFFFF, 1, true, 6, false))
        failures++;

    if (!run_directed_test(tb, "BGE (Signed): -1 >= 1 (Skip)", 0xFFFFFFFF, 1, true, 5, false))
        failures++;
    if (!run_directed_test(tb, "BGEU (Unsigned): MAX_UINT >= 1 (Take)", 0xFFFFFFFF, 1, true, 7, true))
        failures++;

    std::cout << "\n--- Directed Tests: Enable Flag ---" << std::endl;
    if (!run_directed_test(tb, "Disabled Branch (is_branch = 0)", 10, 10, false, 0, false))
        failures++;

    // --- FUZZING ---
    std::cout << "\n--- Fuzzing (Stress Test) ---" << std::endl;
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist_val(0, 0xFFFFFFFF);
    std::uniform_int_distribution<int> dist_funct3(0, 7);
    std::uniform_int_distribution<int> dist_bool(0, 1);

    const int NUM_FUZZ_TESTS = 10000;
    int fuzz_failures = 0;

    for (int i = 0; i < NUM_FUZZ_TESTS; i++)
    {
        uint32_t rs1 = dist_val(rng);
        uint32_t rs2 = dist_val(rng);
        bool is_branch = dist_bool(rng);
        uint8_t funct3 = dist_funct3(rng);

        tb.dut->rs1_data_i = rs1;
        tb.dut->rs2_data_i = rs2;
        tb.dut->is_branch_i = is_branch;
        tb.dut->funct3_i = funct3;
        tb.eval();

        bool exp = golden_branch(rs1, rs2, is_branch, funct3);

        if (tb.dut->branch_taken_o != exp)
        {
            std::cerr << "  \033[1;31m❌\033[0m Fuzzing failed at iteration " << i << std::endl;
            fuzz_failures++;
            failures++;
            if (fuzz_failures > 5)
                break;
        }
    }

    if (fuzz_failures == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m " << NUM_FUZZ_TESTS << " random branches evaluated successfully!" << std::endl;
    }

    std::cout << std::endl;
    if (failures == 0)
    {
        std::cout << "✅ Branch Unit tests PASSED.\n"
                  << std::endl;
    }
    else
    {
        std::cout << "❌ Branch Unit tests FAILED with " << failures << " error(s).\n"
                  << std::endl;
    }

    return failures == 0 ? 0 : 1;
}