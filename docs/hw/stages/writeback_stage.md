# Write-Back Stage

## Contexto
O estágio de **Write-Back** (Escrita de Retorno) é o quinto e último bloco lógico do Datapath do SuperNova-RV. Sua única responsabilidade é atuar como o roteador final do processador, abrigando o multiplexador que decide qual dado será enviado de volta ao Banco de Registradores (Register File) no estágio de Decode.

## Especificação da Interface

| Nome do Sinal  | Direção | Largura/Tipo  | Descrição |
| :---           | :---    | :---          | :---      |
| `alu_result_i` | Entrada | `logic [31:0]`| Resultado da operação aritmética/lógica vindo do estágio Execute. |
| `lsu_rdata_i`  | Entrada | `logic [31:0]`| Dado lido e formatado vindo da Memória (Load-Store Unit). |
| `pc_plus_4_i`  | Entrada | `logic [31:0]`| Endereço de retorno sequencial (`PC + 4`) vindo do estágio Fetch. Usado para salvar o retorno de funções (JAL/JALR). |
| `wb_src_i`     | Entrada | `wb_src_e`    | Sinal de controle (enum) que seleciona qual das três entradas será repassada para a saída. |
| `wb_data_o`    | Saída   | `logic [31:0]`| Dado final selecionado que será roteado de volta para a porta `rd_data_i` do Register File. |

## Arquitetura
Este módulo é **puramente combinacional**. Ele não possui clock (`clk_i`) ou reset (`rst_ni`). Ele atua apenas como uma "chave seletora" instantânea. A escrita real do dado só acontece quando esse sinal chega no `reg_file.sv` (dentro do estágio Decode) e encontra uma borda de subida do clock junto com o sinal `reg_we_i` ativado.