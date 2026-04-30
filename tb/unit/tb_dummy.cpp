#include <iostream>
#include <memory>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vdummy.h" // Cabeçalho gerado pelo Verilator

int main(int argc, char **argv)
{
    // Inicializa o Verilator
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    // Instancia o módulo e o tracer VCD
    auto dut = std::make_unique<Vdummy>();
    auto trace = std::make_unique<VerilatedVcdC>();

    dut->trace(trace.get(), 99);
    trace->open("dump.vcd");

    // Condições Iniciais
    dut->clk = 0;
    dut->rst_n = 0;
    dut->data_in = 0xAAAA5555;

    uint64_t time = 0;

    // Simulação simples (20 ciclos de clock)
    while (time < 200)
    {
        // Toggle do Clock a cada 5 unidades de tempo
        if ((time % 5) == 0)
        {
            dut->clk = !dut->clk;
        }

        // Libera o reset no tempo 15
        if (time == 15)
            dut->rst_n = 1;

        // Avalia o modelo e grava no log
        dut->eval();
        trace->dump(time);
        time++;
    }

    trace->close();
    std::cout << "Simulação Dummy concluída. Arquivo dump.vcd gerado." << std::endl;

    return 0;
}