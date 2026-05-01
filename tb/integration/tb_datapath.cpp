#include "../include/testbench.hpp"
#include "Vdatapath.h"
#include <cstdint>
#include <iostream>
#include <iomanip>

// ========================================================
// ENUM MIRRORS (must match RTL encoding)
// ========================================================
enum ImmType
{
    ImmI = 0,
    ImmS,
    ImmB,
    ImmU,
    ImmJ
};
enum AluSrcA
{
    AluSrcARs1 = 0,
    AluSrcAPc,
    AluSrcAZero
};
enum AluSrcB
{
    AluSrcBRs2 = 0,
    AluSrcBImm
};
enum AluOp
{
    AluAdd = 0,
    AluSll,
    AluSlt,
    AluSltu,
    AluXor,
    AluSrl,
    AluOr,
    AluAnd,
    AluSub
};
enum MemSize
{
    MemSizeByte = 0,
    MemSizeHalf,
    MemSizeWord
};
enum WbSrc
{
    WbSrcAlu = 0,
    WbSrcMem,
    WbSrcPc
};

// ========================================================
// Helper: apply ADDI
// ========================================================
void exec_addi(Testbench<Vdatapath> &tb, uint32_t instr)
{
    // 1. Injeta a instrução no barramento
    tb.dut->instr_rdata_i = instr;

    // 2. Define os sinais de controle (o que a Control Unit faria instantaneamente)
    tb.dut->stall_i = 0;
    tb.dut->branch_valid_i = 0;
    tb.dut->jalr_sel_i = 0;
    tb.dut->imm_type_i = ImmI;
    tb.dut->alu_src_a_i = AluSrcARs1;
    tb.dut->alu_src_b_i = AluSrcBImm;
    tb.dut->alu_op_i = AluAdd;
    tb.dut->mem_we_i = 0;
    tb.dut->mem_size_i = MemSizeWord;
    tb.dut->mem_unsigned_i = 0;
    tb.dut->wb_src_i = WbSrcAlu;
    tb.dut->reg_we_i = 1;

    // 3. PROPAGAÇÃO COMBINACIONAL
    // Permite que o Verilator resolva a ALU, extensões de sinal e os Muxes
    // antes de qualquer borda de clock.
    tb.eval();

    // 4. BORDA DE SUBIDA (Posedge)
    // É neste exato momento que o Banco de Registradores captura o Write-Back.
    tb.dut->clk_i = 1;
    tb.eval();

    // 5. BORDA DE DESCIDA (Negedge)
    // Prepara o clock para o próximo ciclo e garante que a próxima instrução
    // injetada pelo C++ comece com o clock zerado.
    tb.dut->clk_i = 0;
    tb.eval();
}

// ========================================================
// Helper: read register via rs1 exposure
// ========================================================
uint32_t read_reg(Testbench<Vdatapath> &tb, uint32_t rs1)
{
    uint32_t instr = (rs1 << 15) | 0x33; // fake R-type

    tb.dut->instr_rdata_i = instr;
    tb.dut->reg_we_i = 0;

    tb.eval();
    return tb.dut->rs1_data_o;
}

// ========================================================
// MAIN TEST
// ========================================================
int main(int argc, char **argv)
{
    Testbench<Vdatapath> tb("trace_datapath_extended.vcd");
    int failures = 0;

    std::cout << "\n==> Datapath Extended Verification\n"
              << std::endl;

    // ----------------------------------------------------
    // RESET
    // ----------------------------------------------------
    tb.dut->rst_ni = 0;
    tb.tick();
    tb.dut->rst_ni = 1;
    tb.eval();

    // ====================================================
    // TEST 1: Independent ADDI writes
    // ====================================================

    std::cout << "--- Independent Writes ---" << std::endl;

    exec_addi(tb, 0x00A00093); // x1 = 10

    exec_addi(tb, 0x01400113); // x2 = 20

    if (read_reg(tb, 1) != 10)
    {
        std::cerr << "❌ x1 mismatch\n";
        failures++;
    }
    else
        std::cout << "  ✓ x1 = 10\n";

    if (read_reg(tb, 2) != 20)
    {
        std::cerr << "❌ x2 mismatch\n";
        failures++;
    }
    else
        std::cout << "  ✓ x2 = 20\n";

    // ====================================================
    // TEST 2: Dependent chain (with hazard mitigation)
    // ====================================================

    std::cout << "\n--- Dependent Chain (with bubbles) ---" << std::endl;

    exec_addi(tb, 0x00500193); // x3 = 5

    // x4 = x3 + 10 → ADDI x4, x3, 10
    exec_addi(tb, (10 << 20) | (3 << 15) | (4 << 7) | 0x13);

    uint32_t x4 = read_reg(tb, 4);

    if (x4 != 15)
    {
        std::cerr << "❌ x4 mismatch. Expected 15 got " << x4 << "\n";
        failures++;
    }
    else
        std::cout << "  ✓ x4 = 15\n";

    // ====================================================
    // TEST 3: x0 immutability
    // ====================================================

    std::cout << "\n--- x0 Immutability ---" << std::endl;

    exec_addi(tb, 0x06300013); // ADDI x0, x0, 99

    if (read_reg(tb, 0) != 0)
    {
        std::cerr << "❌ x0 modified!\n";
        failures++;
    }
    else
        std::cout << "  ✓ x0 correctly immutable\n";

    // ====================================================
    // TEST 4: Stall behavior
    // ====================================================

    std::cout << "\n--- Stall Behavior ---" << std::endl;

    uint32_t pc_before = tb.dut->instr_addr_o;

    tb.dut->stall_i = 1;
    tb.tick();

    uint32_t pc_after = tb.dut->instr_addr_o;

    if (pc_before != pc_after)
    {
        std::cerr << "❌ PC changed during stall\n";
        failures++;
    }
    else
        std::cout << "  ✓ PC held during stall\n";

    tb.dut->stall_i = 0;

    // ====================================================
    // FINAL RESULT
    // ====================================================

    std::cout << "\n----------------------------------------" << std::endl;

    if (failures == 0)
    {
        std::cout << "✅ Datapath extended test PASSED" << std::endl;
    }
    else
    {
        std::cout << "❌ Datapath extended test FAILED (" << failures << " errors)" << std::endl;
    }

    std::cout << "----------------------------------------\n"
              << std::endl;

    return failures == 0 ? 0 : 1;
}