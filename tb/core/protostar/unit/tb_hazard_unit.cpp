#include "../include/testbench.hpp"
#include "Vhazard_unit.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <random>

int main(int argc, char **argv)
{
    Testbench<Vhazard_unit> tb("trace_hazard_unit.vcd");
    int failures = 0;

    std::cout << "\n==> Hazard Detection Unit (HDU) Verification\n"
              << std::endl;

    // Helper lambda to set inputs, evaluate and check the result
    auto check = [&](uint8_t mem_read, uint8_t rd, uint8_t rs1, uint8_t rs2, uint8_t expected, const char *msg)
    {
        tb.dut->id_ex_mem_read_i = mem_read;
        tb.dut->id_ex_rd_addr_i = rd;
        tb.dut->if_id_rs1_addr_i = rs1;
        tb.dut->if_id_rs2_addr_i = rs2;
        tb.eval();

        if (tb.dut->load_use_hazard_o != expected)
        {
            std::cerr << "❌ FAILED: " << msg
                      << " | Expected: " << (int)expected
                      << " Got: " << (int)tb.dut->load_use_hazard_o << "\n";
            failures++;
        }
        else
        {
            std::cout << "  \033[1;32m✓\033[0m " << msg << "\n";
        }
    };

    std::cout << "--- Directed Tests (Corner Cases) ---\n";

    // Test 1: No hazard (Not a load)
    check(0, 5, 5, 0, 0, "No hazard when ID_EX is not a LOAD (even if addresses match)");

    // Test 2: Hazard on RS1
    check(1, 10, 10, 2, 1, "Hazard detected correctly on RS1 match");

    // Test 3: Hazard on RS2
    check(1, 15, 3, 15, 1, "Hazard detected correctly on RS2 match");

    // Test 4: Both match
    check(1, 20, 20, 20, 1, "Hazard detected correctly when both RS1 and RS2 match");

    // Test 5: No match
    check(1, 10, 11, 12, 0, "No hazard when addresses do not match");

    // Test 6: Load to x0 (Immutable, should be ignored)
    check(1, 0, 0, 0, 0, "No hazard when Load target is x0 (Bypass condition)");

    std::cout << "\n--- Fuzzing (Stress Test) ---\n";
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist_reg(0, 31);
    std::uniform_int_distribution<int> dist_bool(0, 1);

    const int NUM_FUZZ = 10000;
    int fuzz_failures = 0;

    for (int i = 0; i < NUM_FUZZ; i++)
    {
        uint8_t mem_read = dist_bool(rng);
        uint8_t rd = dist_reg(rng);
        uint8_t rs1 = dist_reg(rng);
        uint8_t rs2 = dist_reg(rng);

        tb.dut->id_ex_mem_read_i = mem_read;
        tb.dut->id_ex_rd_addr_i = rd;
        tb.dut->if_id_rs1_addr_i = rs1;
        tb.dut->if_id_rs2_addr_i = rs2;
        tb.eval();

        // Golden model logic
        uint8_t expected = 0;
        if (mem_read && rd != 0 && (rd == rs1 || rd == rs2))
        {
            expected = 1;
        }

        if (tb.dut->load_use_hazard_o != expected)
        {
            if (fuzz_failures < 5)
            {
                std::cerr << "  \033[1;31m❌\033[0m Fuzzing mismatch at iter " << i
                          << " | mem_read:" << (int)mem_read
                          << " rd:" << (int)rd
                          << " rs1:" << (int)rs1
                          << " rs2:" << (int)rs2
                          << " | Expected:" << (int)expected
                          << " Got:" << (int)tb.dut->load_use_hazard_o << "\n";
            }
            fuzz_failures++;
        }
    }

    if (fuzz_failures == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m " << NUM_FUZZ << " fuzzing iterations passed smoothly.\n";
    }
    else
    {
        failures += fuzz_failures;
        std::cerr << "  \033[1;31m❌\033[0m Fuzzing failed with " << fuzz_failures << " errors.\n";
    }

    std::cout << "\n----------------------------------------\n";
    if (failures == 0)
    {
        std::cout << "✅ Hazard Unit test PASSED\n";
    }
    else
    {
        std::cout << "❌ Hazard Unit test FAILED (" << failures << " errors)\n";
    }
    std::cout << "----------------------------------------\n\n";

    return failures == 0 ? 0 : 1;
}