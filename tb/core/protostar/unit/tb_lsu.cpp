#include "../include/testbench.hpp"
#include "Vlsu.h"
#include <cstdint>
#include <iostream>
#include <random>

// ========================================================
// Memory access size encoding (must match DUT encoding)
// ========================================================
enum MemSize
{
    MemSizeByte = 0,
    MemSizeHalf = 1,
    MemSizeWord = 2
};

// ========================================================
// Golden Model for LSU
//
// This function models the *expected* behavior of the LSU.
// It is used as a reference to compare against the DUT.
//
// IMPORTANT:
// - Assumes NATURALLY ALIGNED accesses only
// - Misaligned accesses are NOT modeled (by design)
// ========================================================
void golden_lsu(uint32_t addr, uint32_t wdata, bool we, MemSize size, bool is_unsigned, uint32_t mem_rdata,
                uint32_t &exp_mem_wdata, uint8_t &exp_mem_be, uint32_t &exp_rdata)
{
    uint32_t offset = addr & 0x3; // Byte offset inside the 32-bit word

    // ---------------------------
    // STORE PATH (Write)
    // ---------------------------
    if (we)
    {
        if (size == MemSizeByte)
        {
            // Write only one byte at the correct position
            exp_mem_wdata = (wdata & 0xFF) << (offset * 8);
            exp_mem_be = 0b0001 << offset;
        }
        else if (size == MemSizeHalf)
        {
            // Write two bytes (aligned)
            exp_mem_wdata = (wdata & 0xFFFF) << (offset * 8);

            // Select lower or upper half depending on offset[1]
            exp_mem_be = (offset >= 2) ? 0b1100 : 0b0011;
        }
        else
        {
            // Full word write
            exp_mem_wdata = wdata;
            exp_mem_be = 0b1111;
        }
    }
    else
    {
        // No write
        exp_mem_wdata = 0;
        exp_mem_be = 0;
    }

    // ---------------------------
    // LOAD PATH (Read)
    // ---------------------------
    if (size == MemSizeByte)
    {
        uint8_t b = (mem_rdata >> (offset * 8)) & 0xFF;

        // Sign or zero extension
        exp_rdata = is_unsigned ? (uint32_t)b
                                : (uint32_t)((int32_t)((int8_t)b));
    }
    else if (size == MemSizeHalf)
    {
        uint16_t h = (mem_rdata >> (offset * 8)) & 0xFFFF;

        // Sign or zero extension
        exp_rdata = is_unsigned ? (uint32_t)h
                                : (uint32_t)((int32_t)((int16_t)h));
    }
    else
    {
        // Word load ignores offset (aligned access assumed)
        exp_rdata = mem_rdata;
    }
}

// ========================================================
// Directed Test Helper
//
// Applies a single deterministic test vector and checks
// DUT outputs against expected values.
// ========================================================
bool run_directed_test(Testbench<Vlsu> &tb, const std::string &desc, uint32_t addr, uint32_t wdata, bool we,
                       MemSize size, bool is_unsigned, uint32_t mem_rdata,
                       uint32_t exp_mem_wdata, uint8_t exp_mem_be, uint32_t exp_rdata)
{
    tb.dut->addr_i = addr;
    tb.dut->wdata_i = wdata;
    tb.dut->we_i = we;
    tb.dut->mem_size_i = size;
    tb.dut->mem_unsigned_i = is_unsigned;
    tb.dut->mem_rdata_i = mem_rdata;

    tb.eval();

    bool pass = true;

    if (tb.dut->mem_wdata_o != exp_mem_wdata)
        pass = false;
    if (tb.dut->mem_be_o != exp_mem_be)
        pass = false;
    if (tb.dut->rdata_o != exp_rdata)
        pass = false;

    if (pass)
    {
        std::cout << "  \033[1;32m✓\033[0m " << desc << std::endl;
        return true;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m " << desc << std::endl;
        return false;
    }
}

// ========================================================
// Main Verification Entry Point
// ========================================================
int main(int argc, char **argv)
{
    Testbench<Vlsu> tb("trace_lsu.vcd");
    int failures = 0;

    std::cout << "\n==> Starting LSU verification (Golden Model + Fuzzing)..." << std::endl;

    // ====================================================
    // DIRECTED TESTS (Corner Cases / Deterministic)
    // ====================================================
    std::cout << "\n--- Directed Tests: Store Alignment ---" << std::endl;

    if (!run_directed_test(tb, "SW (Word) @ 0x0000", 0x0000, 0xAABBCCDD, true, MemSizeWord, false, 0,
                           0xAABBCCDD, 0b1111, 0))
        failures++;

    if (!run_directed_test(tb, "SB (Byte) @ 0x0001", 0x0001, 0x000000FF, true, MemSizeByte, false, 0,
                           0x0000FF00, 0b0010, 0))
        failures++;

    if (!run_directed_test(tb, "SH (Half) @ 0x0002", 0x0002, 0x0000BEEF, true, MemSizeHalf, false, 0,
                           0xBEEF0000, 0b1100, 0))
        failures++;

    // Write disabled
    if (!run_directed_test(tb, "Write disabled (WE=0)", 0x0000, 0xFFFFFFFF, false, MemSizeWord, false, 0,
                           0x00000000, 0b0000, 0))
        failures++;

    std::cout << "\n--- Directed Tests: Load Sign Extension ---" << std::endl;

    if (!run_directed_test(tb, "LB signed (0x80 -> 0xFFFFFF80)", 0x0000, 0, false, MemSizeByte, false,
                           0x00000080, 0, 0b0000, 0xFFFFFF80))
        failures++;

    if (!run_directed_test(tb, "LBU unsigned (0x80 -> 0x00000080)", 0x0000, 0, false, MemSizeByte, true,
                           0x00000080, 0, 0b0000, 0x00000080))
        failures++;

    // ====================================================
    // FUZZ TEST (Constrained Random Verification)
    // ====================================================
    std::cout << "\n--- Fuzzing (Constrained Random) ---" << std::endl;

    std::mt19937 rng(42);

    std::uniform_int_distribution<uint32_t> dist_32(0, 0xFFFFFFFF);
    std::uniform_int_distribution<int> dist_size(0, 2);
    std::uniform_int_distribution<int> dist_bool(0, 1);

    const int NUM_FUZZ_TESTS = 10000;
    int fuzz_failures = 0;

    for (int i = 0; i < NUM_FUZZ_TESTS; i++)
    {
        uint32_t addr = dist_32(rng);
        uint32_t wdata = dist_32(rng);
        uint32_t mem_rdata = dist_32(rng);

        bool we = dist_bool(rng);
        MemSize size = static_cast<MemSize>(dist_size(rng));
        bool is_unsigned = dist_bool(rng);

        // ------------------------------------------------
        // CONSTRAINT: Enforce NATURAL ALIGNMENT
        //
        // This avoids generating undefined/misaligned accesses.
        // Equivalent to ISA-compliant behavior (RV32I).
        // ------------------------------------------------
        switch (size)
        {
        case MemSizeByte:
            // No constraint needed
            break;

        case MemSizeHalf:
            addr &= ~0x1; // align to 2 bytes
            break;

        case MemSizeWord:
            addr &= ~0x3; // align to 4 bytes
            break;
        }

        // Apply stimulus
        tb.dut->addr_i = addr;
        tb.dut->wdata_i = wdata;
        tb.dut->we_i = we;
        tb.dut->mem_size_i = size;
        tb.dut->mem_unsigned_i = is_unsigned;
        tb.dut->mem_rdata_i = mem_rdata;

        tb.eval();

        // Compute expected behavior
        uint32_t exp_mem_wdata, exp_rdata;
        uint8_t exp_mem_be;

        golden_lsu(addr, wdata, we, size, is_unsigned,
                   mem_rdata, exp_mem_wdata, exp_mem_be, exp_rdata);

        if (tb.dut->mem_wdata_o != exp_mem_wdata)
        {
            std::cerr << "Mismatch WDATA\n";
            std::cerr << "wdata=" << std::hex << wdata << "\n";
            std::cerr << "dut=" << tb.dut->mem_wdata_o << "\n";
            std::cerr << "exp=" << exp_mem_wdata << "\n";
        }

        // Compare DUT vs Golden Model
        if (tb.dut->mem_wdata_o != exp_mem_wdata ||
            tb.dut->mem_be_o != exp_mem_be ||
            tb.dut->rdata_o != exp_rdata)
        {
            std::cerr << "  \033[1;31m❌\033[0m Fuzz failure at iteration " << i << std::endl;

            fuzz_failures++;
            failures++;

            // Early exit to avoid flooding logs
            if (fuzz_failures > 5)
                break;
        }
    }

    if (fuzz_failures == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m "
                  << NUM_FUZZ_TESTS
                  << " constrained memory accesses passed!" << std::endl;
    }

    // ====================================================
    // FINAL RESULT
    // ====================================================
    std::cout << std::endl;

    if (failures == 0)
    {
        std::cout << "✅ LSU test PASSED.\n"
                  << std::endl;
    }
    else
    {
        std::cout << "❌ LSU test FAILED with "
                  << failures << " error(s).\n"
                  << std::endl;
    }

    return failures == 0 ? 0 : 1;
}