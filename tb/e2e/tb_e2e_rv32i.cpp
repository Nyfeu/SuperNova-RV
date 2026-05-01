#include "../include/testbench.hpp"
#include "Vtop_level.h"
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <iomanip>
#include <sstream>

// ====================================================================
// Configurações de MMIO e Cores
// ====================================================================
const uint32_t DEBUG_PORT_ADDR = 0x10000000;
const uint32_t UART_PORT_ADDR = 0x10000004;
const uint32_t SIGN_SUCCESS = 0xDEADBEEF;
const uint32_t SIGN_FAILURE = 0xBAD0BAD0;

#define RESET "\033[0m"
#define BOLDCYAN "\033[1m\033[36m"
#define BOLDYELLOW "\033[1m\033[33m"
#define BOLDWHITE "\033[1m\033[37m"
#define BOLDRED "\033[1m\033[31m"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Uso: " << argv[0] << " <caminho_para_arquivo.hex> [flags...]\n";
        std::cerr << "Flags suportadas:\n";
        std::cerr << "  --isa | --app | --compliance\n";
        std::cerr << "  --trace\n";
        std::cerr << "  --sig-start <addr_hex>\n";
        std::cerr << "  --sig-end <addr_hex>\n";
        std::cerr << "  --sig-file <caminho_saida>\n";
        return 1;
    }

    std::string hex_file_path = argv[1];
    std::string test_mode = "ISA"; // Padrão
    bool trace_mode = false;

    // Variáveis para extração de Assinatura (Compliance)
    uint32_t sig_start = 0;
    uint32_t sig_end = 0;
    std::string sig_file = "";

    // Parse dos argumentos de linha de comando
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--app")
            test_mode = "APP";
        else if (arg == "--compliance")
            test_mode = "COMPLIANCE";
        else if (arg == "--trace")
            trace_mode = true;
        else if (arg == "--sig-start" && i + 1 < argc)
            sig_start = std::stoul(argv[++i], nullptr, 16);
        else if (arg == "--sig-end" && i + 1 < argc)
            sig_end = std::stoul(argv[++i], nullptr, 16);
        else if (arg == "--sig-file" && i + 1 < argc)
            sig_file = argv[++i];
    }

    Testbench<Vtop_level> tb("traces/trace_e2e_" + test_mode + ".vcd");

    std::cout << BOLDCYAN << "\n==> SuperNova-RV E2E " << test_mode << " Verification Engine" << RESET << std::endl;
    std::cout << "--- Loading Binary: " << hex_file_path << std::endl;
    if (trace_mode)
    {
        std::cout << "--- Trace Mode: ENABLED" << std::endl;
    }

    // ====================================================
    // 1. Parser de Memória (Suporte a @ADDR e Multi-Word)
    // ====================================================
    std::map<uint32_t, uint32_t> memory;
    std::ifstream file(hex_file_path);

    if (!file.is_open())
    {
        std::cerr << BOLDRED << "Erro: Arquivo " << hex_file_path << " não encontrado." << RESET << std::endl;
        return 1;
    }

    std::string line;
    uint32_t current_addr = 0;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '/')
            continue;
        if (line[0] == '@')
        {
            // Multiplica por 4 para converter Word Address de volta para Byte Address
            current_addr = std::stoul(line.substr(1), nullptr, 16) * 4;
            continue;
        }
        std::stringstream ss(line);
        std::string hex_val;
        while (ss >> hex_val)
        {
            try
            {
                memory[current_addr] = std::stoul(hex_val, nullptr, 16);
                current_addr += 4;
            }
            catch (...)
            {
                continue;
            }
        }
    }
    std::cout << "--- Memory Map: " << std::dec << memory.size() << " Words Loaded\n"
              << std::endl;

    // ====================================================
    // 2. Loop de Simulação
    // ====================================================
    tb.reset();

    int max_cycles = 500000; // Aumentado para testes de compliance maiores
    bool test_passed = false;
    bool test_failed = false;
    std::string uart_buffer = "";

    for (int cycle = 0; cycle < max_cycles; cycle++)
    {
        // ESTÁGIO 1: IMEM
        uint32_t pc = tb.dut->imem_addr_o & 0xFFFFFFFC;
        tb.dut->imem_rdata_i = (memory.count(pc)) ? memory[pc] : 0x00000013; // NOP

        // IMPRESSÃO DO TRACE
        if (trace_mode)
        {
            std::cout << "[Cycle " << std::dec << std::setw(4) << std::setfill(' ') << cycle << "] "
                      << "PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << pc
                      << " | IR: 0x" << std::setw(8) << tb.dut->imem_rdata_i << std::endl;
        }

        tb.eval();

        // ESTÁGIO 2: DMEM Load
        uint32_t daddr = tb.dut->dmem_addr_o & 0xFFFFFFFC;
        if (!tb.dut->dmem_we_o)
        {
            tb.dut->dmem_rdata_i = (memory.count(daddr)) ? memory[daddr] : 0;
        }
        tb.eval();

        // ESTÁGIO 3: DMEM Store & MMIO
        if (tb.dut->dmem_we_o)
        {
            uint32_t ddata = tb.dut->dmem_wdata_o;

            if (daddr == UART_PORT_ADDR)
            {
                char c = (char)(ddata & 0xFF);
                if (c == '\n')
                {
                    std::cout << BOLDYELLOW << "[CONSOLE] " << uart_buffer << RESET << std::endl;
                    uart_buffer = "";
                }
                else
                {
                    uart_buffer += c;
                }
            }
            else if (daddr == DEBUG_PORT_ADDR)
            {
                if (ddata == SIGN_SUCCESS)
                {
                    test_passed = true;
                    if (!uart_buffer.empty())
                        std::cout << BOLDYELLOW << "[CONSOLE] " << uart_buffer << RESET << std::endl;
                    std::cout << BOLDWHITE << "\n>>> SUCCESS AT CYCLE " << std::dec << cycle << RESET << std::endl;
                    break;
                }
                else if (ddata == SIGN_FAILURE)
                {
                    test_failed = true;
                    std::cout << BOLDRED << "\n>>> FAILURE AT CYCLE " << std::dec << cycle
                              << " | PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << pc
                              << RESET << std::endl;
                    break;
                }
            }
            else
            {
                // Lógica de Escrita Parcial (Read-Modify-Write)
                uint32_t current_val = (memory.count(daddr)) ? memory[daddr] : 0;
                uint32_t mask = 0;
                uint8_t be = tb.dut->dmem_be_o; // O sinal que vem da sua lsu.sv

                // Transforma os bits de Byte Enable em máscara de 32 bits
                if (be & 0x1)
                    mask |= 0x000000FF;
                if (be & 0x2)
                    mask |= 0x0000FF00;
                if (be & 0x4)
                    mask |= 0x00FF0000;
                if (be & 0x8)
                    mask |= 0xFF000000;

                // Aplica a máscara: mantém o que já estava lá e atualiza só os bytes habilitados
                memory[daddr] = (current_val & ~mask) | (ddata & mask);
            }
        }
        tb.tick();
    }

    // ====================================================
    // 3. Resultado Final & Signature Dump
    // ====================================================
    if (test_passed)
    {
        // Geração do Arquivo de Assinatura (Compliance Oficial)
        if (test_mode == "COMPLIANCE" && !sig_file.empty())
        {
            std::ofstream sf(sig_file);
            if (!sf.is_open())
            {
                std::cerr << BOLDRED << "Erro: Nao foi possivel criar arquivo de assinatura: " << sig_file << RESET << std::endl;
                return 1;
            }
            sig_start &= 0xFFFFFFF0; // Garante alinhamento em blocos de 16 bytes (4 words)

            // O Spike imprime 4 words por linha, da word mais significativa (addr+12) para a menos (addr)
            for (uint32_t addr = sig_start; addr < sig_end; addr += 16)
            {
                uint32_t val0 = memory.count(addr + 0) ? memory[addr + 0] : 0;
                uint32_t val1 = memory.count(addr + 4) ? memory[addr + 4] : 0;
                uint32_t val2 = memory.count(addr + 8) ? memory[addr + 8] : 0;
                uint32_t val3 = memory.count(addr + 12) ? memory[addr + 12] : 0;

                sf << std::hex << std::setw(8) << std::setfill('0') << val3
                   << std::setw(8) << std::setfill('0') << val2
                   << std::setw(8) << std::setfill('0') << val1
                   << std::setw(8) << std::setfill('0') << val0 << "\n";
            }
            sf.close();
            std::cout << "--- Signature Dumped to: " << sig_file << " (" << std::dec << ((sig_end - sig_start) / 4) << " words)\n";
        }

        std::cout << BOLDCYAN << "\n✅ E2E " << test_mode << " TEST: PASSED" << RESET << "\n"
                  << std::endl;
        return 0;
    }
    else
    {
        std::cerr << BOLDRED << "\n❌ E2E " << test_mode << " TEST: FAILED / TIMEOUT" << RESET << "\n"
                  << std::endl;
        return 1;
    }
}