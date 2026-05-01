#pragma once

#include <verilated.h>
#include <verilated_vcd_c.h>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>

// Macro de verificação de igualdade com mensagens de erro detalhadas
#define CHECK_EQ(expected, actual, msg)                                                                                 \
    if ((expected) != (actual))                                                                                         \
    {                                                                                                                   \
        std::cerr << "\n❌ ERRO FATAL: " << msg << "\n"                                                                 \
                  << "   Esperado: 0x" << std::hex << std::setw(8) << std::setfill('0') << (expected) << "\n"           \
                  << "   Obtido:   0x" << std::hex << std::setw(8) << std::setfill('0') << (actual) << std::dec << "\n" \
                  << "   Tempo:    " << tb.main_time << " ticks\n\n";                                                   \
        return 1;                                                                                                       \
    }                                                                                                                   \
    else                                                                                                                \
    {                                                                                                                   \
        std::cout << "  ✓ " << msg << " (0x" << std::hex << (actual) << std::dec << ")\n";                              \
    }

template <class Module>
class Testbench
{
public:
    std::unique_ptr<Module> dut; // Design Under Test (DUT)
    std::unique_ptr<VerilatedVcdC> trace;
    uint64_t main_time;

    // Construtor: Inicializa o Verilator e abre o arquivo VCD
    Testbench(const std::string &vcd_name = "")
    {
        dut = std::make_unique<Module>();
        main_time = 0;

        Verilated::traceEverOn(true);
        if (!vcd_name.empty())
        {
            // Cria a pasta traces/ caso ela não exista
            std::filesystem::create_directories("traces");

            // Força o caminho para dentro de traces/
            std::string vcd_path = "traces/" + vcd_name;

            trace = std::make_unique<VerilatedVcdC>();
            dut->trace(trace.get(), 99);
            trace->open(vcd_path.c_str());
        }
    }

    // Destrutor: Garante que o arquivo VCD seja salvo e fechado com segurança
    virtual ~Testbench()
    {
        if (trace)
        {
            trace->close();
        }
    }

    // Avalia os sinais combinacionais e salva o momento no log
    void eval()
    {
        dut->eval();
        if (trace)
        {
            trace->dump(main_time);
        }
        main_time += 5; // Avança meia unidade de tempo
    }

    // Gera um ciclo de clock completo (borda de descida -> borda de subida)
    void tick()
    {
        dut->clk_i = 0;
        eval(); // settle low phase

        dut->clk_i = 1;
        eval(); // posedge (posedge triggered logic)

        dut->clk_i = 0;
        eval(); // settle after edge
    }

    // Aplica a rotina padrão de Reset assíncrono (ativo em baixo)
    void reset()
    {
        dut->rst_ni = 0;
        tick();
        dut->rst_ni = 1;
        eval();
    }
};