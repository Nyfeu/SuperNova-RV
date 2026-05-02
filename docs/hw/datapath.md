# Caminho de Dados (*Datapath*)

## 1. Contexto
O Caminho de Dados é o núcleo estrutural do processador SuperNova-RV. Ele instancia e interconecta todas as unidades operacionais: Contador de Programa (PC), Arquivo de Registradores, Gerador de Imediato, ALU e Unidade de Carregamento/Armazenamento (LSU). Ele executa operações estritamente baseadas nos sinais de controle fortemente tipados fornecidos pela Unidade de Controle. Também faz interface direta com a Memória de Instruções (IMEM) e a Memória de Dados (DMEM).

![Diagrama de blocos](../../assets/single_cycle_datapath.png){ .hero-img }

## 2. Interface

### 2.1. Sinais de Entrada/Saída

| Nome do Sinal      | Direção | Largura/Tipo    | Descrição |
| :---               | :---    | :---            | :---      |
| **Sinais Globais** |         |                 |           |
| `clk_i`            | Entrada | 1 bit           | Clock do sistema principal. |
| `rst_ni`           | Entrada | 1 bit           | Reset assíncrono ativo-baixo. |
| **Interface IMEM** |         |                 |           |
| `imem_addr_o`      | Saída   | 32 bits         | Endereço da instrução (PC atual). |
| `imem_data_i`      | Entrada | 32 bits         | Instrução obtida da IMEM. |
| **Interface DMEM** |         |                 |           |
| `dmem_addr_o`      | Saída   | 32 bits         | Endereço físico da Memória de Dados (da LSU). |
| `dmem_wdata_o`     | Saída   | 32 bits         | Dados formatados para escrita em memória. |
| `dmem_rdata_i`     | Entrada | 32 bits         | Dados brutos lidos da memória. |
| `dmem_be_o`        | Saída   | 4 bits          | Habilitadores de Byte para escritas em memória. |
| **Entradas de Controle** |   |                 |           |
| `reg_we_i`         | Entrada | 1 bit           | Habilitação de Escrita do Arquivo de Registradores. |
| `imm_type_i`       | Entrada | `imm_type_e`    | Seletor de formato de imediato. |
| `alu_src_a_i`      | Entrada | `alu_src_a_e`   | Seletor do Multiplexador A da ALU (Rs1, PC, Zero). |
| `alu_src_b_i`      | Entrada | `alu_src_b_e`   | Seletor do Multiplexador B da ALU (Rs2, Imm). |
| `alu_op_i`         | Entrada | `alu_op_e`      | Código de operação da ALU. |
| `mem_we_i`         | Entrada | 1 bit           | Habilitação de Escrita Global da LSU. |
| `mem_size_i`       | Entrada | `mem_size_e`    | Tamanho de acesso à memória de dados. |
| `mem_unsigned_i`   | Entrada | 1 bit           | Sinalizador de extensão com zeros para Carregamentos. |
| `wb_src_i`         | Entrada | `wb_src_e`      | Seletor do Multiplexador Write-back (ALU, Mem, PC+4). |
| `pc_src_i`         | Entrada | `pc_src_e`      | Seletor do Multiplexador do Próximo PC (PC+4, Desvio/JAL, JALR). |
| **Saídas de Controle** |     |                 |           |
| `instr_o`          | Saída   | 32 bits         | Repassa a instrução obtida para a Unidade de Controle. |
| `rs1_data_o`       | Saída   | 32 bits         | Repassa dados de Rs1 para a Unidade de Desvios. |
| `rs2_data_o`       | Saída   | 32 bits         | Repassa dados de Rs2 para a Unidade de Desvios. |

## 3. Fluxo de Dados Interno 

1. **Busca (Fetch):** O registrador PC contém o endereço da instrução atual. Um somador dedicado computa `PC + 4`. 
2. **Decodificação (Decode):** A instrução alimenta as portas `ReadAddr` do Arquivo de Registradores e do `ImmGen`.
3. **Execução (Execute):** 
      - Multiplexador A seleciona entre `rs1`, `PC` ou `0`.
      - Multiplexador B seleciona entre `rs2` ou o `Imediato`.
      - Um Somador de Alvo dedicado computa `PC + Imediato` para desvios e JAL.
4. **Memória (Memory):** A LSU alinha leituras e escritas baseado no endereço de saída da ALU.
5. **Escrita de Retorno (Write-Back):** O Multiplexador WBSrc seleciona dados da ALU, da LSU, ou `PC + 4` para escrever em `rd`.
6. **Próximo PC (Next PC):** O Multiplexador PCSrc seleciona entre `PC + 4`, `Somador de Alvo`, ou o `Resultado da ALU` (para JALR), e o realimenta ao registrador PC.


!!! note "Instruction Cycle"
      A implementação da microarquitetura é de ciclo único (`single-cycle`), portanto todos os estágios acontecem no mesmo tick do `clk`.