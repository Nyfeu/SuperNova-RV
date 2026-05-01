#include <iostream>
#include <iomanip>
#include <unordered_map>
#include "Vfetch_stage.h"
#include "../include/testbench.hpp"

// Helper function to initialize our virtual ROM in C++
void load_firmware(std::unordered_map<uint32_t, uint32_t> &mem)
{
    // Address -> Instruction (Fictitious Opcode for testing)
    mem[0x00000000] = 0x00000013; // NOP (addi x0, x0, 0)
    mem[0x00000004] = 0x00100093; // addi x1, x0, 1
    mem[0x00000008] = 0x00200113; // addi x2, x0, 2
    mem[0x0000000C] = 0x00300193; // addi x3, x0, 3
    mem[0x00001000] = 0xDEADBEEF; // Target of a distant jump
}

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Testbench<Vfetch_stage> tb("build/fetch_stage.vcd");

    std::cout << "\n==> Starting Instruction Fetch (IF) Verification...\n\n";

    // 1. Configure Virtual Memory (C++ acting as RAM/ROM)
    std::unordered_map<uint32_t, uint32_t> mock_rom;
    load_firmware(mock_rom);

    // 2. Initialize signals and apply Reset
    tb.dut->stall_i = 0;
    tb.dut->branch_valid_i = 0;
    tb.dut->branch_target_i = 0;
    tb.dut->instr_rdata_i = 0; // Memory has not yet responded

    // Asynchronous reset loads BOOT_ADDR (0x00000000)
    tb.reset();

    // ------------------------------------------------------------------------
    // TEST 1: Boot and Sequential Execution (PC + 4)
    // ------------------------------------------------------------------------
    std::cout << "--- Test 1: Sequential Fetch ---\n";
    for (int i = 0; i < 4; i++)
    {
        uint32_t current_pc = tb.dut->instr_addr_o;

        // C++ reads the address emitted by hardware and provides the instruction
        uint32_t fetched_instr = mock_rom.count(current_pc) ? mock_rom[current_pc] : 0xFFFFFFFF;
        tb.dut->instr_rdata_i = fetched_instr;
        tb.eval();

        // Checks if CPU passed the instruction correctly to Decode
        CHECK_EQ(fetched_instr, tb.dut->instr_o, "Fetch at PC: 0x" + std::to_string(current_pc));
        CHECK_EQ(current_pc + 4, tb.dut->pc_next_o, "PC+4 Calculation");

        tb.tick(); // Clock edge -> PC advances
    }

    // ------------------------------------------------------------------------
    // TEST 2: Pipeline Stall (Freezing)
    // ------------------------------------------------------------------------
    std::cout << "\n--- Test 2: Pipeline Stall ---\n";
    uint32_t pc_before_stall = tb.dut->pc_o;

    tb.dut->stall_i = 1; // Activates Stall
    tb.tick();
    tb.tick(); // Passes 2 clock cycles

    CHECK_EQ(pc_before_stall, tb.dut->pc_o, "PC did not advance during stall");

    tb.dut->stall_i = 0; // Releases Stall

    // ------------------------------------------------------------------------
    // TEST 3: Branch / Jump Execution
    // ------------------------------------------------------------------------
    std::cout << "\n--- Test 3: Branch Execution ---\n";

    // We simulate that the Branch Unit (BCU) ordered a jump to 0x1000
    tb.dut->branch_valid_i = 1;
    tb.dut->branch_target_i = 0x00001000;
    tb.tick(); // Clock edge -> PC loads branch target

    tb.dut->branch_valid_i = 0; // Deactivates branch to avoid jumping in loop
    tb.eval();

    // We feed memory with the instruction from the new address
    uint32_t new_pc = tb.dut->instr_addr_o;
    tb.dut->instr_rdata_i = mock_rom[new_pc];
    tb.eval();

    CHECK_EQ(0x00001000, new_pc, "PC jumped to target address");
    CHECK_EQ(0xDEADBEEF, tb.dut->instr_o, "Fetched instruction at Branch Target");

    std::cout << "\n✅  Instruction Fetch Stage PASSED all structural checks!\n\n";

    return 0;
}