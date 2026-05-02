# Top Level - IP

## 1. Visão Geral
O `top_level` é o contêiner estrutural máximo do processador SuperNova-RV. Ele materializa a arquitetura final "costurando" os Músculos (`datapath`) e o Cérebro (`controlpath`).

A principal função deste módulo é abstrair toda a complexidade interna de roteamento, fatiamento de instruções e sinais de controle. Para o mundo externo, o processador atua como uma "caixa preta" que segue estritamente a topologia Harvard, expondo apenas os pinos de *Clock*, *Reset* e os barramentos das Memórias de Instrução (IMEM) e Dados (DMEM).

## 2. Parâmetros
| Parâmetro | Tipo | Padrão | Descrição |
| :--- | :--- | :--- | :--- |
| `BOOT_ADDR` | `logic [31:0]` | `32'h0000_0000` | Endereço do *Program Counter* (PC) onde a CPU inicia a execução após o reset. |

## 3. Interface de Hardware (Ports)

### 3.1. Sinais Básicos
| Sinal | Direção | Descrição |
| :--- | :--- | :--- |
| `clk_i` | Input | Clock principal do sistema (Borda de subida sensível). |
| `rst_ni` | Input | Reset global do processador, assíncrono e ativo em nível baixo (ativo quando `0`). |

### 3.2. Interface de Memória de Instruções (IMEM - ROM)
*Responsável pela busca (Fetch) do código.*

| Sinal | Largura | Direção | Descrição |
| :--- | :---: | :--- | :--- |
| `imem_addr_o` | 32 bits | Output | Endereço da instrução a ser buscada (igual ao PC atual). |
| `imem_rdata_i`| 32 bits | Input | Instrução crua retornada pela memória. |

### 3.3. Interface de Memória de Dados (DMEM - RAM)
*Responsável pelas operações de Load e Store.*

| Sinal | Largura | Direção | Descrição |
| :--- | :---: | :--- | :--- |
| `dmem_addr_o` | 32 bits | Output | Endereço físico do dado na RAM (calculado pela ALU). |
| `dmem_wdata_o`| 32 bits | Output | Dado a ser escrito na memória (Store). |
| `dmem_be_o` | 4 bits | Output | *Byte Enables*. Máscara que indica quais bytes dentro da Word de 32 bits serão alterados. |
| `dmem_we_o` | 1 bit | Output | *Write Enable*. Quando `1`, grava `dmem_wdata_o` na RAM no próximo ciclo de clock. |
| `dmem_rdata_i`| 32 bits | Input | Dado bruto lido da memória (Load). |

## 3.4. Diagrama de Blocos Lógico

O `top_level` gerencia o roteamento em malha fechada entre seus dois submódulos:

1. A Instrução (`imem_rdata_i`) entra no **Datapath**.
2. O Datapath fatia a instrução e envia apenas `opcode`, `funct3`, `funct7_5` e os valores literais de `rs1` e `rs2` para o **Controlpath**.
3. O **Controlpath** processa esses dados e devolve um barramento largo de controle (Multiplexadores, *Write Enables*, configurações de ALU).
4. O **Datapath** aplica esses controles para rotear o dado pela ALU ou pela Interface DMEM, salvando o resultado de volta em seus registradores síncronos na subida do clock.

## 3.5. Considerações de Design (Linting)
Este módulo implementa o fatiamento da instrução na borda da instanciação do `controlpath` (`.opcode_i(instr[6:0])`). Essa abordagem purista foi adotada deliberadamente para evitar o roteamento de fios ociosos entre os módulos, atendendo aos requisitos estritos de análise estática e verificação formal do Verilator e do Verible Linter.