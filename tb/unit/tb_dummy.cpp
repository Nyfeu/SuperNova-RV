#include <iostream>
#include <memory>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vdummy.h"

int main(int argc, char **argv)
{
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);

    auto dut = std::make_unique<Vdummy>();
    auto trace = std::make_unique<VerilatedVcdC>();

    dut->trace(trace.get(), 99);
    trace->open("dump.vcd");

    // Condições Iniciais com os novos nomes (_i, _ni)
    dut->clk_i = 0;
    dut->rst_ni = 0;
    dut->data_i = 0xAAAA5555;

    uint64_t time = 0;

    while (time < 200)
    {
        if ((time % 5) == 0)
        {
            dut->clk_i = !dut->clk_i;
        }

        if (time == 15)
            dut->rst_ni = 1;

        dut->eval();
        trace->dump(time);
        time++;
    }

    trace->close();
    std::cout << "Simulação Dummy concluída. Arquivo dump.vcd gerado." << std::endl;

    return 0;
}