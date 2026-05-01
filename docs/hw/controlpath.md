# Controlpath 

## Visão Geral
O `controlpath` atua como o "cérebro" do processador SuperNova-RV. É um módulo puramente combinacional responsável por receber as fatias críticas da instrução atual, decodificá-la e gerar todos os sinais de controle necessários para governar a execução no `datapath`. Além disso, ele encapsula a lógica de resolução de saltos condicionais e incondicionais.

Para evitar *warnings* de sinais ociosos em linters rigorosos (como o Verilator), o módulo foi projetado para receber a instrução já fatiada, não consumindo o barramento de 32 bits inteiro.

## Arquitetura Interna
O módulo atua como um *wrapper* combinacional integrando dois submódulos principais:
1. **`instr_decoder`**: Decodificador plano (*flat decoder*) que traduz o `opcode` e os campos `funct` nos sinais de multiplexação e habilitação (Write Enables).
2. **`branch_unit`**: Unidade de salto que recebe os dados dos registradores `rs1` e `rs2` diretamente do Datapath e avalia condições aritméticas (ex: `==`, `<`, `>=`) operando de forma paralela à ALU.

## Interface de Hardware (Ports)

### Entradas (Inputs)
| Sinal | Largura | Descrição |
| :--- | :---: | :--- |
| `opcode_i` | 7 bits | Bits `[6:0]` da instrução. Define a categoria principal da operação. |
| `funct3_i` | 3 bits | Bits `[14:12]` da instrução. Usado para sub-operações e condições de branch. |
| `funct7_5_i` | 1 bit | Bit `[30]` da instrução. Essencial para distinguir `ADD`/`SUB` e `SRL`/`SRA`. |
| `rs1_data_i` | 32 bits | Dado lido da porta 1 do Banco de Registradores (usado pela Branch Unit). |
| `rs2_data_i` | 32 bits | Dado lido da porta 2 do Banco de Registradores (usado pela Branch Unit). |

### Saídas de Controle (Outputs)
*Nota: Muitos sinais utilizam tipos enumerados (enums) definidos no pacote `supernova_pkg` para segurança de tipo (Type Safety).*

| Sinal | Tipo | Descrição |
| :--- | :--- | :--- |
| `stall_o` | `logic` | Sinal de *stall* da arquitetura. Como o processador é *single-cycle*, é cravado em `0`. |
| `branch_valid_o` | `logic` | Sinaliza ao Fetch Stage se o salto (Condicional, JAL ou JALR) deve ser tomado. |
| `jalr_sel_o` | `logic` | Seleciona se o endereço alvo do salto vem do cálculo relativo ao PC ou da ALU (JALR). |
| `reg_we_o` | `logic` | Habilita a escrita no Banco de Registradores. |
| `imm_type_o` | `imm_type_e` | Informa ao *Immediate Generator* o formato de extração (I, S, B, U, J). |
| `alu_src_a_o` | `alu_src_a_e` | Seleciona o primeiro operando da ALU (RS1, PC ou Zero). |
| `alu_src_b_o` | `alu_src_b_e` | Seleciona o segundo operando da ALU (RS2 ou Imediato). |
| `alu_op_o` | `alu_op_e` | Define a operação matemática/lógica que a ALU deve executar. |
| `mem_we_o` | `logic` | Habilita a escrita na Memória de Dados (Store). |
| `mem_size_o` | `mem_size_e` | Define a granularidade do acesso à memória (Byte, Half-Word, Word). |
| `mem_unsigned_o`| `logic` | Define o comportamento de extensão de sinal para leituras sub-word (LB vs LBU). |
| `wb_src_o` | `wb_src_e` | Seleciona qual dado será escrito no Banco de Registradores (ALU, Memória ou PC+4). |

## Considerações de Design
* **Isolamento Lógico:** Nenhuma operação aritmética do datapath passa por aqui. O fluxo de dados principal fica restrito aos "músculos", garantindo caminhos críticos mais curtos e organizados.
* **Resolução de Salto em Um Ciclo:** A avaliação de branches condicionais acontece no mesmo ciclo de clock do *Fetch* e *Decode*, permitindo redirecionamento imediato do `PC` sem necessidade de predição de desvios.