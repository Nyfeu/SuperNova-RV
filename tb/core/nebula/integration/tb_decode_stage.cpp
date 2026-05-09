#include "../include/testbench.hpp"
#include "Vdecode_stage.h"
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <random>
#include <vector>

// Enum mapping for imm_type_e
enum ImmType
{
    ImmI = 0,
    ImmS = 1,
    ImmB = 2,
    ImmU = 3,
    ImmJ = 4
};

// --------------------------------------------------------
// Golden Model: Immediate Generator
// --------------------------------------------------------
uint32_t golden_imm_gen(uint32_t instr, ImmType type)
{
    uint32_t imm = 0;
    uint32_t bit31 = (instr >> 31) & 1;
    uint32_t sign_ext = bit31 ? 0xFFFFF000 : 0x00000000;

    switch (type)
    {
    case ImmI:
        imm = (instr >> 20) & 0xFFF;
        return imm | sign_ext;
    case ImmS:
        imm = (((instr >> 25) & 0x7F) << 5) | ((instr >> 7) & 0x1F);
        return imm | sign_ext;
    case ImmB:
        imm = (((instr >> 31) & 1) << 12) | (((instr >> 7) & 1) << 11) |
              (((instr >> 25) & 0x3F) << 5) | (((instr >> 8) & 0xF) << 1);
        return imm | sign_ext;
    case ImmU:
        return instr & 0xFFFFF000;
    case ImmJ:
        imm = (((instr >> 31) & 1) << 20) | (((instr >> 12) & 0xFF) << 12) |
              (((instr >> 20) & 1) << 11) | (((instr >> 21) & 0x3FF) << 1);
        sign_ext = bit31 ? 0xFFF00000 : 0x00000000;
        return imm | sign_ext;
    default:
        return 0;
    }
}

int main(int argc, char **argv)
{
    Testbench<Vdecode_stage> tb("trace_decode_stage.vcd");
    int failures = 0;

    // Golden Model para o Register File (32 registradores, x0 sempre 0)
    std::vector<uint32_t> golden_reg_file(32, 0);

    std::cout << "\n==> Starting Decode Stage verification..." << std::endl;

    // Inicialização segura
    tb.dut->clk_i = 0;
    tb.dut->reg_we_i = 0;
    tb.dut->instr_payload_i = 0;
    tb.dut->rd_data_i = 0;
    tb.eval();

    // --------------------------------------------------------
    // Teste 1: Geração de Imediatos (Lógica Combinacional)
    // --------------------------------------------------------
    std::cout << "\n--- Directed Tests: Immediate Extraction ---" << std::endl;

    // Instrução Fictícia para teste de extração: 0x80508093 (ADDI x1, x1, -2043)
    uint32_t test_instr = 0x80508093;
    tb.dut->instr_payload_i = test_instr >> 7; // CORREÇÃO AQUI
    tb.dut->imm_type_i = ImmI;
    tb.eval();

    if (tb.dut->imm_o == golden_imm_gen(test_instr, ImmI))
    {
        std::cout << "  \033[1;32m✓\033[0m I-Type Immediate extracted correctly." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m I-Type Immediate failed." << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // Teste 2: Escrita e Leitura de Registradores (Lógica Sequencial)
    // --------------------------------------------------------
    std::cout << "\n--- Directed Tests: RegFile Integration ---" << std::endl;

    // Queremos escrever 0xDEADBEEF no registrador x5 (rd = 5)
    // instr[11:7] = 5 -> (5 << 7) = 0x00000280
    tb.dut->instr_payload_i = 0x00000280 >> 7; // CORREÇÃO AQUI
    tb.dut->rd_data_i = 0xDEADBEEF;
    tb.dut->reg_we_i = 1;
    tb.tick(); // Borda de subida do clock! Escrita ocorre aqui.

    // Atualiza o Golden Model
    golden_reg_file[5] = 0xDEADBEEF;

    // Agora vamos ler o x5 colocando-o na porta rs1 (instr[19:15] = 5 -> 5 << 15 = 0x00028000)
    tb.dut->reg_we_i = 0;                      // Desliga a escrita
    tb.dut->instr_payload_i = 0x00028000 >> 7; // CORREÇÃO AQUI
    tb.eval();                                 // Combinacional, o dado deve aparecer na saída imediatamente

    if (tb.dut->rs1_data_o == golden_reg_file[5])
    {
        std::cout << "  \033[1;32m✓\033[0m Successfully wrote to and read from x5." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m RegFile Data routing failed." << std::endl;
        failures++;
    }

    // Testando a proteção do Registrador Zero (x0)
    tb.dut->instr_payload_i = 0x00000000 >> 7; // CORREÇÃO AQUI
    tb.dut->rd_data_i = 0xFFFFFFFF;
    tb.dut->reg_we_i = 1;
    tb.tick();

    tb.dut->reg_we_i = 0;
    tb.eval();

    if (tb.dut->rs1_data_o == 0 && tb.dut->rs2_data_o == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m Zero Register (x0) write protection is intact." << std::endl;
    }
    else
    {
        std::cerr << "  \033[1;31m❌\033[0m x0 was overwritten!" << std::endl;
        failures++;
    }

    // --------------------------------------------------------
    // FUZZING: Teste de Estresse Sequencial
    // --------------------------------------------------------
    std::cout << "\n--- Fuzzing (Stress Test: RegFile & ImmGen) ---" << std::endl;
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist_32(0, 0xFFFFFFFF);
    std::uniform_int_distribution<int> dist_type(0, 4);
    std::uniform_int_distribution<int> dist_bool(0, 1);

    const int NUM_FUZZ_TESTS = 10000;
    int fuzz_failures = 0;

    for (int i = 0; i < NUM_FUZZ_TESTS; i++)
    {
        uint32_t rand_instr = dist_32(rng);
        uint32_t rand_data = dist_32(rng);
        bool we = dist_bool(rng);
        ImmType type = static_cast<ImmType>(dist_type(rng));

        // 1. Aplica as entradas
        tb.dut->instr_payload_i = rand_instr >> 7; // CORREÇÃO AQUI
        tb.dut->rd_data_i = rand_data;
        tb.dut->reg_we_i = we;
        tb.dut->imm_type_i = type;

        // 2. Extrai os endereços para o C++ saber o que verificar
        uint32_t rs1_addr = (rand_instr >> 15) & 0x1F;
        uint32_t rs2_addr = (rand_instr >> 20) & 0x1F;
        uint32_t rd_addr = (rand_instr >> 7) & 0x1F;

        // 3. Avalia as leituras combinacionais (Ocorre antes da borda do clock)
        tb.eval();

        // Checa ImmGen
        uint32_t exp_imm = golden_imm_gen(rand_instr, type);
        if (tb.dut->imm_o != exp_imm)
        {
            std::cerr << "  \033[1;31m❌\033[0m Fuzzing failed: Immediate mismatch at iter " << i << std::endl;
            fuzz_failures++;
        }

        // Checa RegFile Read
        if (tb.dut->rs1_data_o != golden_reg_file[rs1_addr] || tb.dut->rs2_data_o != golden_reg_file[rs2_addr])
        {
            std::cerr << "  \033[1;31m❌\033[0m Fuzzing failed: RegFile Read mismatch at iter " << i << std::endl;
            fuzz_failures++;
        }

        // 4. Executa a escrita (Tick no Clock)
        tb.tick();

        // Atualiza o Golden Model se 'we' for verdadeiro e não for o registrador x0
        if (we && rd_addr != 0)
        {
            golden_reg_file[rd_addr] = rand_data;
        }

        if (fuzz_failures > 5)
            break;
    }

    if (fuzz_failures == 0)
    {
        std::cout << "  \033[1;32m✓\033[0m " << NUM_FUZZ_TESTS << " random cycles (Read/Write/Decode) evaluated successfully!" << std::endl;
    }
    else
    {
        failures += fuzz_failures;
    }

    std::cout << std::endl;
    if (failures == 0)
    {
        std::cout << "✅ Decode Stage tests PASSED.\n"
                  << std::endl;
    }
    else
    {
        std::cout << "❌ Decode Stage tests FAILED with " << failures << " error(s).\n"
                  << std::endl;
    }

    return failures == 0 ? 0 : 1;
}