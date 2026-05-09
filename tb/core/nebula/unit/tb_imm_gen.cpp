#include "../include/testbench.hpp"
#include "Vimm_gen.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <random>
#include <string>

// Enum mapping para bater com supernova_pkg
enum ImmType
{
    IMM_I = 0,
    IMM_S = 1,
    IMM_B = 2,
    IMM_U = 3,
    IMM_J = 4
};

// --------------------------------------------------------
// Golden Model: C++ implementation of RISC-V Imm decoding
// --------------------------------------------------------
uint32_t golden_imm_gen(uint32_t instr, ImmType type)
{
    uint32_t imm = 0;
    uint32_t sign = (instr >> 31) & 0x1;
    uint32_t sign_ext = sign ? 0xFFFFF000 : 0x00000000;

    switch (type)
    {
    case IMM_I:
        imm = sign_ext | ((instr >> 20) & 0xFFF);
        break;
    case IMM_S:
        imm = sign_ext | (((instr >> 25) & 0x7F) << 5) | ((instr >> 7) & 0x1F);
        break;
    case IMM_B:
        sign_ext = sign ? 0xFFFFF000 : 0x00000000;
        imm = sign_ext | (((instr >> 7) & 0x1) << 11) | (((instr >> 25) & 0x3F) << 5) | (((instr >> 8) & 0xF) << 1);
        break;
    case IMM_U:
        imm = instr & 0xFFFFF000;
        break;
    case IMM_J:
        sign_ext = sign ? 0xFFF00000 : 0x00000000;
        imm = sign_ext | (((instr >> 12) & 0xFF) << 12) | (((instr >> 20) & 0x1) << 11) | (((instr >> 21) & 0x3FF) << 1);
        break;
    default:
        imm = 0;
        break;
    }
    return imm;
}

// --------------------------------------------------------
// Função Auxiliar para Testes Dirigidos (Formatação Visual)
// --------------------------------------------------------
bool run_directed_test(Testbench<Vimm_gen> &tb, const std::string &description, uint32_t instr, ImmType type, uint32_t expected)
{

    // O hardware espera os bits 31:7 (arquitetura limpa, sem opcode)
    tb.dut->instr_i = (instr >> 7);
    tb.dut->imm_type_i = type;
    tb.eval();

    uint32_t result = tb.dut->imm_o;

    if (result == expected)
    {
        std::cout << "  \033[1;32m✓\033[0m " << description << " (0x" << std::hex << result << std::dec << ")" << std::endl;
        return true;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m " << description << " | Esperado: 0x" << std::hex << expected
                  << " Obtido: 0x" << result << std::dec << std::endl;
        return false;
    }
}

// --------------------------------------------------------
// Main Testbench
// --------------------------------------------------------
int main(int argc, char **argv)
{
    Testbench<Vimm_gen> tb("trace_imm_gen.vcd");
    int failures = 0;

    std::cout << "\n==> Iniciando verificacao do Immediate Generator (Golden Model & Fuzzing)..." << std::endl;

    // --- TESTES DIRIGIDOS (INSTRUÇÕES REAIS DO RISC-V) ---

    std::cout << "\n--- Testando Formatos de Instrucao (Casos Reais) ---" << std::endl;

    // I-Type: addi x1, x0, -15 (Imm = -15 -> 0xFFFFFFF1)
    if (!run_directed_test(tb, "I-Type: addi x1, x0, -15", 0xFF100093, IMM_I, 0xFFFFFFF1))
        failures++;

    // S-Type: sw x2, 16(x3) (Imm = 16 -> 0x00000010)
    if (!run_directed_test(tb, "S-Type: sw x2, 16(x3)", 0x0021A823, IMM_S, 0x00000010))
        failures++;

    // B-Type: beq x4, x5, -8 (Imm = -8 -> 0xFFFFFFF8)
    if (!run_directed_test(tb, "B-Type: beq x4, x5, -8", 0xFE520CE3, IMM_B, 0xFFFFFFF8))
        failures++;

    // U-Type: lui x6, 0x12345 (Imm = 0x12345000)
    if (!run_directed_test(tb, "U-Type: lui x6, 0x12345", 0x12345337, IMM_U, 0x12345000))
        failures++;

    // J-Type: jal x7, 4096 (Imm = 0x00001000)
    if (!run_directed_test(tb, "J-Type: jal x7, 4096", 0x000013EF, IMM_J, 0x00001000))
        failures++;

    // --- TESTE DE ESTRESSE (FUZZING) ---

    std::cout << "\n--- Iniciando Teste de Estresse (Fuzzing) ---" << std::endl;

    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist_instr(0, 0xFFFFFFFF);
    std::uniform_int_distribution<int> dist_type(0, 4);

    const int NUM_FUZZ_TESTS = 10000;
    int fuzz_failures = 0;

    for (int i = 0; i < NUM_FUZZ_TESTS; i++)
    {
        uint32_t random_instr = dist_instr(rng);
        ImmType random_type = static_cast<ImmType>(dist_type(rng));

        tb.dut->instr_i = (random_instr >> 7);
        tb.dut->imm_type_i = random_type;
        tb.eval();

        uint32_t expected = golden_imm_gen(random_instr, random_type);

        if (tb.dut->imm_o != expected)
        {
            std::cerr << "  \033[1;31m❌\033[0m Falha no Fuzzing iteracao " << i << std::endl;
            std::cerr << "     Instr: 0x" << std::hex << random_instr << " Type: " << random_type << std::endl;
            std::cerr << "     Esperado: 0x" << expected << " Obtido: 0x" << tb.dut->imm_o << std::dec << std::endl;
            fuzz_failures++;
            failures++;
            if (fuzz_failures > 5)
                break;
        }
    }

    if (fuzz_failures == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m " << NUM_FUZZ_TESTS << " vetores aleatorios verificados com sucesso!" << std::endl;
    }

    std::cout << std::endl;
    if (failures == 0)
    {
        std::cout << "✅ Teste do ImmGen concluido com sucesso.\n"
                  << std::endl;
    }
    else
    {
        std::cout << "❌ Teste do ImmGen falhou com " << failures << " erro(s).\n"
                  << std::endl;
    }

    return failures == 0 ? 0 : 1;
}