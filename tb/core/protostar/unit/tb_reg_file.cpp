#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include "Vreg_file.h"
#include "../include/testbench.hpp"

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);

    // Instancia o testbench. A macro CHECK_EQ exige que a variável se chame 'tb'.
    // Habilitamos a geração do trace VCD para facilitar o debug em caso de falha.
    Testbench<Vreg_file> tb("build/reg_file.vcd");

    // Inicializa as entradas
    tb.dut->we_i = 0;
    tb.dut->rs1_addr_i = 0;
    tb.dut->rs2_addr_i = 0;
    tb.dut->rd_addr_i = 0;
    tb.dut->rd_data_i = 0;
    tb.eval();

    std::cout << "\n==>Iniciando validacao do modulo reg_file...\n\n";

    // Configuração de números aleatórios para fuzzing
    // Seed fixa para reprodutibilidade
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

    // -------------------------------------------------------------------------
    // TESTE 1: Proteção do Registrador Zero (x0)
    // -------------------------------------------------------------------------

    tb.dut->we_i = 1;
    tb.dut->rd_addr_i = 0;
    tb.dut->rd_data_i = 0xDEADBEEF;
    tb.tick();

    tb.dut->rs1_addr_i = 0;
    tb.dut->rs2_addr_i = 0;
    tb.eval();

    CHECK_EQ(0x00000000, tb.dut->rs1_data_o, "Leitura de x0 na porta rs1 deve retornar 0");
    CHECK_EQ(0x00000000, tb.dut->rs2_data_o, "Leitura de x0 na porta rs2 deve retornar 0");

    // -------------------------------------------------------------------------
    // TESTE 2: Escrita e Leitura Básica (x1 a x31)
    // -------------------------------------------------------------------------

    std::vector<uint32_t> expected_state(32, 0);
    tb.dut->we_i = 1;

    for (int i = 1; i < 32; i++)
    {
        uint32_t random_val = dist(rng);
        expected_state[i] = random_val;

        tb.dut->rd_addr_i = i;
        tb.dut->rd_data_i = random_val;
        tb.tick();
    }

    // Desabilita escrita para não poluir o estado durante as leituras
    tb.dut->we_i = 0;

    for (int i = 1; i < 32; i++)
    {
        tb.dut->rs1_addr_i = i;

        // Lê um índice diferente na porta 2 para testar paralelismo
        tb.dut->rs2_addr_i = (i < 31) ? i + 1 : 1;

        tb.eval();

        CHECK_EQ(expected_state[i], tb.dut->rs1_data_o, "Leitura dos dados gravados em rs1");
        CHECK_EQ(expected_state[tb.dut->rs2_addr_i], tb.dut->rs2_data_o, "Leitura dos dados gravados em rs2");
    }

    // -------------------------------------------------------------------------
    // TESTE 3: Verificação de Write Enable (we_i)
    // -------------------------------------------------------------------------

    tb.dut->we_i = 0;
    tb.dut->rd_addr_i = 5;
    tb.dut->rd_data_i = 0xCAFEBABE;
    tb.tick();

    tb.dut->rs1_addr_i = 5;
    tb.eval();

    CHECK_EQ(expected_state[5], tb.dut->rs1_data_o, "Write Enable em 0 nao deve permitir sobrescrita");

    // -------------------------------------------------------------------------
    // TESTE 4: Transparência e Forwarding (Leitura Combinacional)
    // -------------------------------------------------------------------------

    tb.dut->rs1_addr_i = 10;
    tb.eval();
    uint32_t val1 = tb.dut->rs1_data_o;

    tb.dut->rs1_addr_i = 15;
    tb.eval();
    uint32_t val2 = tb.dut->rs1_data_o;

    CHECK_EQ(expected_state[10], val1, "Checagem do estado inicial combinacional (addr 10)");
    CHECK_EQ(expected_state[15], val2, "Checagem da atualizacao combinacional sem tick do clock (addr 15)");

    std::cout << "\n✅ Teste do reg_file concluido com sucesso!\n\n";

    return 0;
}