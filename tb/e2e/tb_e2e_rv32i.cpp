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
        std::cerr << "Uso: " << argv[0] << " <caminho_para_arquivo.hex>\n";
        return 1;
    }

    std::string hex_file_path = argv[1];
    Testbench<Vtop_level> tb("traces/trace_e2e_compliance.vcd");

    std::cout << BOLDCYAN << "\n==> SuperNova-RV E2E Verification Engine" << RESET << std::endl;
    std::cout << "--- Loading Binary: " << hex_file_path << std::endl;

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
            current_addr = std::stoul(line.substr(1), nullptr, 16);
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
    // 2. Loop de Simulação com Buffer de UART
    // ====================================================
    tb.reset();

    int max_cycles = 10000;
    bool test_passed = false;
    bool test_failed = false;
    std::string uart_buffer = "";

    for (int cycle = 0; cycle < max_cycles; cycle++)
    {
        // --- ESTÁGIO 1: Busca de Instrução (IMEM) ---
        uint32_t pc = tb.dut->imem_addr_o & 0xFFFFFFFC;
        tb.dut->imem_rdata_i = (memory.count(pc)) ? memory[pc] : 0x00000013;
        tb.eval();

        // --- ESTÁGIO 2: Leitura de Dados (DMEM Load) ---
        uint32_t daddr = tb.dut->dmem_addr_o & 0xFFFFFFFC;
        if (!tb.dut->dmem_we_o)
        {
            tb.dut->dmem_rdata_i = (memory.count(daddr)) ? memory[daddr] : 0;
        }
        tb.eval();

        // --- ESTÁGIO 3: Escrita e MMIO (DMEM Store) ---
        if (tb.dut->dmem_we_o)
        {
            uint32_t ddata = tb.dut->dmem_wdata_o;

            // Porta UART-Lite (Bufferizada)
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
            // Porta de Debug (Status do Teste)
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
                    std::cout << BOLDRED << "\n>>> FAILURE AT CYCLE " << std::dec << cycle << RESET << std::endl;
                    break;
                }
            }
            else
            {
                memory[daddr] = ddata;
            }
        }

        // Heartbeat silencioso no stderr para não quebrar a UART
        if (cycle % 500 == 0)
        {
            std::cerr << BOLDCYAN << "." << RESET << std::flush;
        }

        tb.tick();
    }

    // ====================================================
    // 3. Resultado Final
    // ====================================================
    if (test_passed)
    {
        std::cout << BOLDCYAN << "✅ E2E Compliance: PASSED" << RESET << "\n"
                  << std::endl;
        return 0;
    }
    else
    {
        std::cerr << BOLDRED << "❌ E2E Compliance: FAILED / TIMEOUT" << RESET << "\n"
                  << std::endl;
        return 1;
    }
}