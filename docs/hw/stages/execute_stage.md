# Estágio de Execução (Execute Stage)

## Contexto
O Estágio de Execução é o terceiro bloco lógico do Caminho de Dados. É o coração computacional do processador. Ele encapsula a Unidade Lógico-Aritmética (ALU) principal, os multiplexadores de operandos (Mux A e Mux B), e um Somador de Alvo dedicado para cálculos de endereços de desvios e saltos.

## Interface

| Nome do Sinal    | Direção | Largura/Tipo    | Descrição |
| :---             | :---    | :---            | :---      |
| `pc_i`           | Entrada | 32 bits         | Contador de Programa atual (vindo da Busca). |
| `rs1_data_i`     | Entrada | 32 bits         | Dados do Registrador Fonte 1 (vindo da Decodificação). |
| `rs2_data_i`     | Entrada | 32 bits         | Dados do Registrador Fonte 2 (vindo da Decodificação). |
| `imm_i`          | Entrada | 32 bits         | Imediato com extensão de sinal (vindo da Decodificação). |
| `alu_src_a_i`    | Entrada | `alu_src_a_e`   | Seletor do Multiplexador A (`Rs1`, `Pc` ou `Zero`). |
| `alu_src_b_i`    | Entrada | `alu_src_b_e`   | Seletor do Multiplexador B (`Rs2` ou `Imm`). |
| `alu_op_i`       | Entrada | `alu_op_e`      | Código de operação para a ALU. |
| `alu_result_o`   | Saída   | 32 bits         | Saída principal da ALU. |
| `target_addr_o`  | Saída   | 32 bits         | Endereço alvo computado (`PC + Imm`). |
| `jalr_addr_o`    | Saída   | 32 bits         | Endereço JALR computado (`(Rs1 + Imm) & ~1`). |

### Descrição dos Sinais

- **`pc_i`**: O Contador de Programa atual, que chegou do estágio de Busca. Usado pelo Somador de Alvo para calcular endereços PC-relativos (para branches e JAL).

- **`rs1_data_i`, `rs2_data_i`**: Dados brutos dos registradores Rs1 e Rs2, vindo do Arquivo de Registradores no estágio de Decodificação. Estes são os operandos primários para a ALU e a Unidade de Desvios.

- **`imm_i`**: O valor de imediato sign-extended de 32 bits, vindo do Gerador de Imediato. Usado como segundo operando da ALU ou como offset de endereço.

- **`alu_src_a_i`, `alu_src_b_i`**: Sinais de controle que selecionam qual operando (Rs1, PC, Zero para A; Rs2, Imm para B) deve ser alimentado à ALU.

- **`alu_op_i`**: Código de operação que especifica qual operação a ALU deve realizar (ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU).

- **`alu_result_o`**: O resultado principal da operação da ALU. Este é um resultado de propósito geral usado em quase todas as instruções: aritmética, lógica, carregamentos, armazenamentos, e até instruções de desvio (para calcular o endereço alvo).

- **`target_addr_o`**: O resultado do Somador de Alvo, que computa `PC + Imm`. Usado exclusivamente para resolver desvios condicionais (B-type) e saltos incondicionais (JAL).

- **`jalr_addr_o`**: O endereço JALR computado, que é o resultado da ALU com o bit menos significativo zerado: `(Rs1 + Imm) & ~1`. Seguindo a especificação RISC-V, o endereço de salto JALR sempre deve ser alinhado em 2 bytes (bit 0 = 0).

## Arquitetura

Este módulo é puramente combinacional. Não contém elementos sequenciais, apenas lógica combinacional que processa entradas em paralelo.

### Componentes Principais

1. **Somador de Alvo (Target Adder)**
      - Operação fixa: `target_addr = pc_i + imm_i`
      - Implementado como um somador de 32 bits
      - Funciona continuamente (sempre pronto)
      - Saída: `target_addr_o`
      - Usado para: Branches, JAL


2. **Multiplexador A (Mux A)**
      - Seleciona o primeiro operando da ALU baseado em `alu_src_a_i`
      - Opções: `rs1_data_i`, `pc_i`, ou constante `0`
      - Saída alimenta a entrada A da ALU


3. **Multiplexador B (Mux B)**
      - Seleciona o segundo operando da ALU baseado em `alu_src_b_i`
      - Opções: `rs2_data_i` ou `imm_i`
      - Saída alimenta a entrada B da ALU


4. **Unidade Lógico-Aritmética (ALU)**
      - Executa a operação especificada por `alu_op_i`
      - Operações suportadas: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU
      - Saída: `alu_result_o` (32 bits)
      - A ALU é uma unidade combinacional pura


5. **Lógica de Mascaramento JALR**
      - Deriva `jalr_addr_o` diretamente de `alu_result_o`
      - Operação: `jalr_addr_o = alu_result_o & 32'hFFFFFFFE`
      - Garante que o endereço JALR sempre tenha o bit 0 = 0 (alinhamento de 2 bytes)
      - Implementado como uma porta AND simples (negligenciável em latência)

### Operações da ALU

| Código | Mnemônico | Operação | Descrição |
| :---   | :---      | :---     | :---      |
| 0      | ADD       | A + B    | Adição |
| 1      | SLL       | A << B[4:0] | Deslocamento à esquerda lógico |
| 2      | SLT       | (A < B) ? 1 : 0 (com sinal) | Set Less Than (com sinal) |
| 3      | SLTU      | (A < B) ? 1 : 0 (sem sinal) | Set Less Than (sem sinal) |
| 4      | XOR       | A ^ B    | Operação XOR lógica |
| 5      | SRL       | A >> B[4:0] | Deslocamento à direita lógico |
| 6      | OR        | A \| B   | Operação OR lógica |
| 7      | AND       | A & B    | Operação AND lógica |
| 8      | SUB       | A - B    | Subtração |
| 13     | SRA       | A >>> B[4:0] | Deslocamento à direita aritmético |

## Integração com o Pipeline

### Uso dos Outputs

- **`alu_result_o`**: Utilizado para:
  - Armazenar resultado em registrador (instrução R-type, I-type)
  - Calcular endereço de memória (Load/Store)
  - Fornecido como segundo argumento para seleção de PC (JALR)

- **`target_addr_o`**: Utilizado para:
  - Fornecer endereço alvo para desvios (branches)
  - Fornecer endereço alvo para JAL

- **`jalr_addr_o`**: Utilizado para:
  - Resolver JALR com o endereço alvo garantidamente alinhado em 2 bytes
  