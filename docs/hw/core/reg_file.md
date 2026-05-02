# Banco de Registradores (Register File)

O Banco de Registradores é uma das peças chaves para definição do estado do processador RISC-V. Diferente da ALU, que é puramente combinacional, este componente introduz a **lógica síncrona** no caminho de dados (***datapath***). Ele contém os 32 registradores de propósito geral (`x0` a `x31`) da arquitetura `RV32I`.

## 1. Especificação da Interface

O módulo SystemVerilog (`hw/core/reg_file.sv`) expõe as seguintes portas:

| Porta | Direção | Largura (Bits) | Descrição |
| :--- | :---: | :---: | :--- |
| `clk_i` | Entrada | 1 | Clock do sistema. Base para a escrita síncrona. |
| `we_i` | Entrada | 1 | Write Enable (RegWrite). Habilita a escrita quando em `1`. |
| `rs1_addr_i` | Entrada | 5 | Endereço do registrador para a 1ª porta de leitura (Source 1). |
| `rs2_addr_i` | Entrada | 5 | Endereço do registrador para a 2ª porta de leitura (Source 2). |
| `rd_addr_i` | Entrada | 5 | Endereço do registrador de destino (Destination) para escrita. |
| `rd_data_i` | Entrada | 32 | Dado a ser escrito no registrador de destino. |
| `rs1_data_o` | Saída | 32 | Dado lido da 1ª porta de leitura. |
| `rs2_data_o` | Saída | 32 | Dado lido da 2ª porta de leitura. |

## 2. Requisitos Comportamentais e Microarquitetura

### 2.1. Armazenamento Interno
O componente instancia um array interno contendo 32 posições de 32 bits cada, mapeando o estado arquitetural da CPU.

### 2.2. Comportamento de Leitura (Assíncrona)
O módulo possui duas portas de leitura independentes. A leitura opera de forma **combinacional**:
* Qualquer alteração nos endereços `rs1_addr_i` ou `rs2_addr_i` reflete-se imediatamente nas saídas `rs1_data_o` e `rs2_data_o`, sem depender das bordas de clock.
* **Proteção do Registrador Zero (x0):** Por definição da ISA RISC-V, o registrador `x0` está *hardwired* em zero. Se o endereço de leitura for `5'b00000`, a saída correspondente forçará `32'h00000000`, ignorando qualquer lixo de memória que pudesse existir nessa posição do array.

### 2.3. Comportamento de Escrita (Síncrona)
A escrita de novos valores no banco opera de forma estritamente **síncrona**:
* A atualização de um registrador ocorre exclusivamente na borda de subida do clock (`posedge clk_i`).
* O dado `rd_data_i` só é efetivamente gravado no endereço `rd_addr_i` se o sinal de controle `we_i` estiver ativo (`1'b1`).
* **Proteção de Escrita no x0:** Nenhuma operação de escrita pode alterar o registrador zero. A lógica interna descarta silenciosamente qualquer tentativa de escrita quando `rd_addr_i == 5'b00000`.