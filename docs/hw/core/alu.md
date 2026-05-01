# Unidade Lógica e Aritmética (ALU)

A ALU é o componente responsável por realizar todas as operações matemáticas e lógicas necessárias para a execução das instruções da arquitetura RV32I. Diferente do Banco de Registradores, a ALU é um circuito **puramente combinacional**; as suas saídas reagem instantaneamente (ignorando atrasos de propagação físicos) às mudanças nas suas entradas, sem depender de um sinal de clock.

## Especificação da Interface

O módulo SystemVerilog (`hw/core/alu.sv`) expõe as seguintes portas:

| Porta | Direção | Largura (Bits) | Descrição |
| :--- | :---: | :---: | :--- |
| `a_i` | Entrada | 32 | Primeiro operando (geralmente vindo de rs1). |
| `b_i` | Entrada | 32 | Segundo operando (geralmente vindo de rs2 ou de um Imediato). |
| `alu_op_i` | Entrada | 4 | Código de controle da operação a ser executada. |
| `result_o` | Saída | 32 | Resultado da operação computada. |

## Mapeamento de Operações (ALU Opcodes)

Para facilitar a decodificação futura, o sinal `alu_op_i` foi desenhado com 4 bits, permitindo um mapeamento direto com os campos `funct3` e `funct7` das instruções RISC-V. O bit mais significativo (bit 3) é utilizado para diferenciar operações correlatas (como ADD/SUB e SRL/SRA).

| Opcode (Bin) | Mnemônico | Operação Matemática | Notas |
| :---: | :---: | :--- | :--- |
| `0000` | ADD | `result_o = a_i + b_i` | Soma padrão (ignora overflow). |
| `1000` | SUB | `result_o = a_i - b_i` | Subtração padrão. |
| `0001` | SLL | `result_o = a_i << b_i[4:0]` | Shift Left Logical. |
| `0010` | SLT | `result_o = (a_i < b_i) ? 1 : 0` | Set Less Than (Comparação com sinal). |
| `0011` | SLTU | `result_o = (a_i < b_i) ? 1 : 0` | Set Less Than Unsigned (Sem sinal). |
| `0100` | XOR | `result_o = a_i ^ b_i` | Ou Exclusivo bit a bit. |
| `0101` | SRL | `result_o = a_i >> b_i[4:0]` | Shift Right Logical (Preenche com zeros). |
| `1101` | SRA | `result_o = a_i >>> b_i[4:0]` | Shift Right Arithmetic (Mantém o bit de sinal). |
| `0110` | OR | `result_o = a_i | b_i` | Ou bit a bit. |
| `0111` | AND | `result_o = a_i & b_i` | E bit a bit. |

**Nota sobre Shifts:** Na arquitetura RV32I, os deslocamentos observam apenas os 5 bits menos significativos do operando `b_i` (`b_i[4:0]`), limitando o deslocamento máximo a 31 posições.