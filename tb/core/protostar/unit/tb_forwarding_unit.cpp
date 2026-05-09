#include "../include/testbench.hpp"
#include "Vforwarding_unit.h"
#include <cstdint>
#include <iostream>

int main(int argc, char **argv)
{
    Testbench<Vforwarding_unit> tb("trace_forwarding_unit.vcd");
    int failures = 0;

    std::cout << "\n==> Starting Forwarding Unit Verification..." << std::endl;

    // Inicialização segura
    tb.dut->id_ex_rs1_addr_i = 0;
    tb.dut->id_ex_rs2_addr_i = 0;
    tb.dut->ex_mem_rd_addr_i = 0;
    tb.dut->ex_mem_reg_we_i = 0;
    tb.dut->mem_wb_rd_addr_i = 0;
    tb.dut->mem_wb_reg_we_i = 0;
    tb.eval();

    // --------------------------------------------------------
    // Teste 1: Sem Hazards (Baseline)
    // --------------------------------------------------------
    tb.dut->id_ex_rs1_addr_i = 5;
    tb.dut->id_ex_rs2_addr_i = 6;
    tb.eval();

    if (tb.dut->forward_a_o == 0 && tb.dut->forward_b_o == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m Test 1: No Hazards correctly yielded 00." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Test 1 Failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Teste 2: Hazard de EX (Prioridade Máxima)
    // --------------------------------------------------------
    // Instrução em EX/MEM vai escrever no x5. A instrução atual quer ler o x5 no rs1.
    tb.dut->ex_mem_rd_addr_i = 5;
    tb.dut->ex_mem_reg_we_i = 1;
    tb.eval();

    if (tb.dut->forward_a_o == 2 && tb.dut->forward_b_o == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m Test 2: EX Hazard (rs1) correctly yielded 10." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Test 2 Failed." << std::endl;
        failures++;
    }

    // Resetar EX
    tb.dut->ex_mem_reg_we_i = 0;

    // --------------------------------------------------------
    // Teste 3: Hazard de MEM
    // --------------------------------------------------------
    // Instrução em MEM/WB vai escrever no x6. A instrução atual quer ler o x6 no rs2.
    tb.dut->mem_wb_rd_addr_i = 6;
    tb.dut->mem_wb_reg_we_i = 1;
    tb.eval();

    if (tb.dut->forward_a_o == 0 && tb.dut->forward_b_o == 1)
    {
        std::cout << "  \033[1;32m✓\033[0m Test 3: MEM Hazard (rs2) correctly yielded 01." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Test 3 Failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Teste 4: Hazard Duplo (Verificação de Prioridade)
    // --------------------------------------------------------
    // A instrução atual quer ler o x7 na porta A e B.
    // MAS ambas as instruções anteriores vão escrever no x7!
    // add x7, x1, x2  (Está em MEM/WB - dado mais antigo)
    // add x7, x3, x4  (Está em EX/MEM - dado mais recente)
    // sub x9, x7, x7  (Está em ID/EX  - instrução atual)

    tb.dut->id_ex_rs1_addr_i = 7;
    tb.dut->id_ex_rs2_addr_i = 7;

    tb.dut->ex_mem_rd_addr_i = 7;
    tb.dut->ex_mem_reg_we_i = 1;

    tb.dut->mem_wb_rd_addr_i = 7;
    tb.dut->mem_wb_reg_we_i = 1;
    tb.eval();

    if (tb.dut->forward_a_o == 2 && tb.dut->forward_b_o == 2)
    {
        std::cout << "  \033[1;32m✓\033[0m Test 4: Double Hazard correctly prioritized EX (10)." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Test 4 Failed. Priority logic is wrong." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Teste 5: Proteção do Registo Zero (x0)
    // --------------------------------------------------------
    // Mesmo que o software tente escrever no x0 de forma inútil,
    // nunca devemos fazer forwarding desse valor fantasma.
    tb.dut->id_ex_rs1_addr_i = 0;
    tb.dut->id_ex_rs2_addr_i = 0;

    tb.dut->ex_mem_rd_addr_i = 0;
    tb.dut->mem_wb_rd_addr_i = 0;
    tb.eval();

    if (tb.dut->forward_a_o == 0 && tb.dut->forward_b_o == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m Test 5: x0 Register bypass correctly ignored." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m Test 5 Failed. x0 is being forwarded!" << std::endl;
        failures++;
    }

    std::cout << std::endl;
    if (failures == 0)
    {
        std::cout << "✅ Forwarding Unit tests PASSED.\n"
                  << std::endl;
    }
    else
    {
        std::cout << "❌ Forwarding Unit tests FAILED with " << failures << " error(s).\n"
                  << std::endl;
    }

    return failures == 0 ? 0 : 1;
}