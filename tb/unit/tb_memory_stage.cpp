#include "../include/testbench.hpp"
#include "Vmemory_stage.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <random>

// Enum mapping for mem_size_e
enum MemSize
{
    MemSizeByte = 0,
    MemSizeHalf = 1,
    MemSizeWord = 2
};

// --------------------------------------------------------
// Golden Model: Load-Store Unit Behavior (ALIGNED ONLY)
// --------------------------------------------------------
struct LsuExpected
{
    uint32_t dmem_addr;
    uint32_t dmem_wdata;
    uint8_t dmem_be;
    uint32_t lsu_rdata;
};

LsuExpected golden_lsu(uint32_t addr, uint32_t wdata, bool we, MemSize size, bool is_unsigned, uint32_t rdata)
{
    LsuExpected exp;
    uint32_t offset = addr & 0x3;

    // Memory is word-aligned externally
    exp.dmem_addr = addr & ~0x3;

    if (we)
    {
        exp.lsu_rdata = 0;

        if (size == MemSizeByte)
        {
            exp.dmem_be = 1 << offset;
            exp.dmem_wdata = (wdata & 0xFF) << (offset * 8);
        }
        else if (size == MemSizeHalf)
        {
            // Only NATURAL Alignment (offset 0 or 2)
            exp.dmem_be = (offset & 0x2) ? 0b1100 : 0b0011;
            exp.dmem_wdata = (wdata & 0xFFFF) << (offset * 8);
        }
        else
        { // Word
            exp.dmem_be = 0b1111;
            exp.dmem_wdata = wdata;
        }
    }
    else
    {
        exp.dmem_be = 0;
        exp.dmem_wdata = 0;

        uint32_t shifted_rdata = rdata >> (offset * 8);

        if (size == MemSizeByte)
        {
            uint8_t byte_val = shifted_rdata & 0xFF;
            exp.lsu_rdata = is_unsigned ? byte_val
                                        : (uint32_t)((int32_t)((int8_t)byte_val));
        }
        else if (size == MemSizeHalf)
        {
            uint16_t half_val = shifted_rdata & 0xFFFF;
            exp.lsu_rdata = is_unsigned ? half_val
                                        : (uint32_t)((int32_t)((int16_t)half_val));
        }
        else
        {
            exp.lsu_rdata = rdata;
        }
    }

    return exp;
}

int main(int argc, char **argv)
{
    Testbench<Vmemory_stage> tb("trace_memory_stage.vcd");
    int failures = 0;

    std::cout << "\n==> Starting Memory Stage verification..." << std::endl;

    // --------------------------------------------------------
    // Directed Test 1: Store Word (SW)
    // --------------------------------------------------------
    std::cout << "\n--- Directed Tests: Memory Writes (Stores) ---" << std::endl;
    tb.dut->alu_result_i = 0x1000;
    tb.dut->rs2_data_i = 0xDEADBEEF;
    tb.dut->mem_we_i = 1;
    tb.dut->mem_size_i = MemSizeWord;
    tb.eval();

    if (tb.dut->dmem_be_o == 0xF && tb.dut->dmem_wdata_o == 0xDEADBEEF)
    {
        std::cout << "  \033[1;32m✓\033[0m Store Word correctly sets BE to 4'b1111." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Store Word failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Directed Test 2: Store Byte (SB)
    // --------------------------------------------------------
    tb.dut->alu_result_i = 0x1002;
    tb.dut->rs2_data_i = 0x000000AA;
    tb.dut->mem_we_i = 1;
    tb.dut->mem_size_i = MemSizeByte;
    tb.eval();

    if (tb.dut->dmem_be_o == 0x4 &&
        (tb.dut->dmem_wdata_o & 0x00FF0000) == 0x00AA0000)
    {
        std::cout << "  \033[1;32m✓\033[0m Store Byte (Offset 2) correct." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Store Byte failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Directed Test 3: Load Half Signed (LH)
    // --------------------------------------------------------
    std::cout << "\n--- Directed Tests: Memory Reads (Loads) ---" << std::endl;
    tb.dut->alu_result_i = 0x1000;
    tb.dut->mem_we_i = 0;
    tb.dut->mem_size_i = MemSizeHalf;
    tb.dut->mem_unsigned_i = 0;
    tb.dut->dmem_rdata_i = 0x0000F000;
    tb.eval();

    if (tb.dut->lsu_rdata_o == 0xFFFFF000)
    {
        std::cout << "  \033[1;32m✓\033[0m Load Half Signed correct." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Load Half Signed failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Directed Test 4: Load Byte Unsigned (LBU)
    // --------------------------------------------------------
    tb.dut->alu_result_i = 0x1003;
    tb.dut->mem_size_i = MemSizeByte;
    tb.dut->mem_unsigned_i = 1;
    tb.dut->dmem_rdata_i = 0xAB000000;
    tb.eval();

    if (tb.dut->lsu_rdata_o == 0x000000AB)
    {
        std::cout << "  \033[1;32m✓\033[0m Load Byte Unsigned correct." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Load Byte Unsigned failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // FUZZING (CONSTRAINED)
    // --------------------------------------------------------
    std::cout << "\n--- Fuzzing (Aligned Only) ---" << std::endl;

    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist_32(0, 0xFFFFFFFF);
    std::uniform_int_distribution<int> dist_size(0, 2);
    std::uniform_int_distribution<int> dist_bool(0, 1);

    const int NUM_FUZZ_TESTS = 10000;
    int fuzz_failures = 0;
    int executed = 0;

    for (int i = 0; i < NUM_FUZZ_TESTS; i++)
    {
        uint32_t addr = dist_32(rng);
        uint32_t wdata = dist_32(rng);
        uint32_t rdata = dist_32(rng);

        bool we = dist_bool(rng);
        bool is_unsign = dist_bool(rng);
        MemSize size = static_cast<MemSize>(dist_size(rng));

        // ✅ CONSTRAINT: alinhamento natural
        if (size == MemSizeHalf)
            addr &= ~0x1;
        if (size == MemSizeWord)
            addr &= ~0x3;

        executed++;

        tb.dut->alu_result_i = addr;
        tb.dut->rs2_data_i = wdata;
        tb.dut->dmem_rdata_i = rdata;
        tb.dut->mem_we_i = we;
        tb.dut->mem_unsigned_i = is_unsign;
        tb.dut->mem_size_i = size;
        tb.eval();

        LsuExpected exp = golden_lsu(addr, wdata, we, size, is_unsign, rdata);

        if (we)
        {
            uint32_t be_mask = 0;
            if (exp.dmem_be & 1)
                be_mask |= 0x000000FF;
            if (exp.dmem_be & 2)
                be_mask |= 0x0000FF00;
            if (exp.dmem_be & 4)
                be_mask |= 0x00FF0000;
            if (exp.dmem_be & 8)
                be_mask |= 0xFF000000;

            if (tb.dut->dmem_be_o != exp.dmem_be ||
                (tb.dut->dmem_wdata_o & be_mask) != (exp.dmem_wdata & be_mask))
            {
                std::cerr << "  ❌ Fuzz Write fail @ " << i << std::endl;
                fuzz_failures++;
            }
        }
        else
        {
            if (tb.dut->lsu_rdata_o != exp.lsu_rdata)
            {
                std::cerr << "  ❌ Fuzz Read fail @ " << i << std::endl;
                fuzz_failures++;
            }
        }

        if (fuzz_failures > 5)
            break;
    }

    if (fuzz_failures == 0)
    {
        std::cout << "  ✓ " << executed << " aligned transactions passed!" << std::endl;
    }
    else
    {
        failures += fuzz_failures;
    }

    std::cout << std::endl;

    if (failures == 0)
        std::cout << "✅ Memory Stage tests PASSED.\n"
                  << std::endl;
    else
        std::cout << "❌ Memory Stage tests FAILED with " << failures << " error(s).\n"
                  << std::endl;

    return failures == 0 ? 0 : 1;
}