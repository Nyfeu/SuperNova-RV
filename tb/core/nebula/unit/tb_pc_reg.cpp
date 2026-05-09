#include <iostream>
#include <iomanip>
#include "Vpc_reg.h"
#include "../include/testbench.hpp"

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);

    Testbench<Vpc_reg> tb("build/dump_pc_reg.vcd");
    std::cout << "\n==> Iniciando validacao do modulo pc_reg...\n\n";

    // 1. Condições Iniciais
    tb.dut->stall_i = 0;
    tb.dut->pc_next_i = 0;

    // 2. Aplica o Reset
    tb.reset();
    CHECK_EQ(0x00000000, tb.dut->pc_o, "Inicializacao apos reset");

    // 3. Simula a contagem normal feita externamente (Datapath enviando PC+4)
    tb.dut->pc_next_i = 0x00000004;
    tb.tick();
    CHECK_EQ(0x00000004, tb.dut->pc_o, "Atualizacao sequencial (PC+4)");

    tb.dut->pc_next_i = 0x00000008;
    tb.tick();
    CHECK_EQ(0x00000008, tb.dut->pc_o, "Atualizacao sequencial (PC+8)");

    // 4. Simula um Salto calculado externamente
    tb.dut->pc_next_i = 0x00000040;
    tb.tick();
    CHECK_EQ(0x00000040, tb.dut->pc_o, "Salto condicional/incondicional");

    // 5. Testa o Stall (Congelamento)
    tb.dut->stall_i = 1;
    tb.dut->pc_next_i = 0x00000044; // Tenta forçar um novo valor
    tb.tick();
    CHECK_EQ(0x00000040, tb.dut->pc_o, "Manutencao de valor durante Stall");

    std::cout << "\n✅ Teste do PC Register concluido com sucesso!\n\n";
    return 0;
}