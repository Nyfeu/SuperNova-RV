#include "../include/testbench.hpp"
#include "Vwriteback_stage.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <random>

// Enum mapping for wb_src_e
enum WbSrc
{
    WbSrcAlu = 0,
    WbSrcMem = 1,
    WbSrcPc = 2
};

// --------------------------------------------------------
// Golden Model: Write-Back Mux
// --------------------------------------------------------
uint32_t golden_wb(uint32_t alu_res, uint32_t mem_data, uint32_t pc_plus_4, WbSrc src)
{
    switch (src)
    {
    case WbSrcAlu:
        return alu_res;
    case WbSrcMem:
        return mem_data;
    case WbSrcPc:
        return pc_plus_4;
    default:
        return alu_res;
    }
}

int main(int argc, char **argv)
{
    Testbench<Vwriteback_stage> tb("trace_writeback_stage.vcd");
    int failures = 0;

    std::cout << "\n==> Starting Write-Back Stage verification..." << std::endl;

    // Valores fixos e muito distintos para facilitar o debug visual
    uint32_t test_alu_res = 0xAAAAAAAA;
    uint32_t test_mem_data = 0xBBBBBBBB;
    uint32_t test_pc_plus_4 = 0xCCCCCCCC;

    tb.dut->alu_result_i = test_alu_res;
    tb.dut->lsu_rdata_i = test_mem_data;
    tb.dut->pc_plus_4_i = test_pc_plus_4;

    // --------------------------------------------------------
    // Directed Test 1: ALU Source (e.g., ADD, SUB, ADDI)
    // --------------------------------------------------------
    tb.dut->wb_src_i = WbSrcAlu;
    tb.eval();

    if (tb.dut->wb_data_o == test_alu_res)
    {
        std::cout << "  \033[1;32m✓\033[0m WbSrcAlu routed correctly (0xAAAAAAAA)." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m WbSrcAlu failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Directed Test 2: Memory Source (e.g., LW, LB)
    // --------------------------------------------------------
    tb.dut->wb_src_i = WbSrcMem;
    tb.eval();

    if (tb.dut->wb_data_o == test_mem_data)
    {
        std::cout << "  \033[1;32m✓\033[0m WbSrcMem routed correctly (0xBBBBBBBB)." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m WbSrcMem failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Directed Test 3: PC+4 Source (e.g., JAL, JALR)
    // --------------------------------------------------------
    tb.dut->wb_src_i = WbSrcPc;
    tb.eval();

    if (tb.dut->wb_data_o == test_pc_plus_4)
    {
        std::cout << "  \033[1;32m✓\033[0m WbSrcPc routed correctly (0xCCCCCCCC)." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m WbSrcPc failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // FUZZING
    // --------------------------------------------------------
    std::cout << "\n--- Fuzzing (Stress Test: Multiplexer) ---" << std::endl;
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist_32(0, 0xFFFFFFFF);
    std::uniform_int_distribution<int> dist_src(0, 2);

    const int NUM_FUZZ_TESTS = 10000;
    int fuzz_failures = 0;

    for (int i = 0; i < NUM_FUZZ_TESTS; i++)
    {
        uint32_t rand_alu = dist_32(rng);
        uint32_t rand_mem = dist_32(rng);
        uint32_t rand_pc = dist_32(rng);
        WbSrc rand_src = static_cast<WbSrc>(dist_src(rng));

        tb.dut->alu_result_i = rand_alu;
        tb.dut->lsu_rdata_i = rand_mem;
        tb.dut->pc_plus_4_i = rand_pc;
        tb.dut->wb_src_i = rand_src;

        tb.eval();

        uint32_t expected = golden_wb(rand_alu, rand_mem, rand_pc, rand_src);

        if (tb.dut->wb_data_o != expected)
        {
            std::cerr << "  \033[1;31m❌\033[0m Fuzzing failed at iteration " << i << std::endl;
            fuzz_failures++;
        }

        if (fuzz_failures > 5)
            break;
    }

    if (fuzz_failures == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m " << NUM_FUZZ_TESTS << " random write-back routes verified successfully!" << std::endl;
    }
    else
    {
        failures += fuzz_failures;
    }

    std::cout << std::endl;
    if (failures == 0)
    {
        std::cout << "✅ Write-Back Stage tests PASSED.\n"
                  << std::endl;
    }
    else
    {
        std::cout << "❌ Write-Back Stage tests FAILED with " << failures << " error(s).\n"
                  << std::endl;
    }

    return failures == 0 ? 0 : 1;
}