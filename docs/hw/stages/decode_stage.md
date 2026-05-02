# Estágio de Decodificação 

## 1. Contexto
O Estágio de Decodificação é o segundo bloco lógico do Caminho de Dados. Ele recebe a instrução bruta obtida da memória e extrai os operandos necessários para a execução. Ele encapsula o Arquivo de Registradores e o Gerador de Imediato, fornecendo uma interface limpa para o Estágio de Execução.

## 2. Interface

| Nome do Sinal    | Direção | Largura/Tipo    | Descrição |
| :---             | :---    | :---            | :---      |
| `clk_i`          | Entrada | 1 bit           | Clock do sistema principal (para escritas no Arquivo de Registradores). |
| `instr_i`        | Entrada | 32 bits         | A instrução bruta vinda do estágio de Busca. |
| `reg_we_i`       | Entrada | 1 bit           | Habilitação de Escrita para o Arquivo de Registradores. |
| `rd_data_i`      | Entrada | 32 bits         | Dados a serem escritos de volta no registrador de destino. |
| `imm_type_i`     | Entrada | `imm_type_e`    | Seletor de formato para o Gerador de Imediato. |
| `rs1_data_o`     | Saída   | 32 bits         | Dados lidos do Registrador Fonte 1. |
| `rs2_data_o`     | Saída   | 32 bits         | Dados lidos do Registrador Fonte 2. |
| `imm_o`          | Saída   | 32 bits         | Valor de imediato com extensão de sinal. |

### 2.1. Descrição dos Sinais

- **`clk_i`**: Clock principal do sistema. Usado para sincronizar as operações de escrita no Arquivo de Registradores (na borda de subida).

- **`instr_i`**: A instrução de 32 bits vinda do estágio de Busca (Fetch). Esta é a entrada principal que conduz todo o resto da decodificação.

- **`reg_we_i`**: Sinal de controle vindo da Unidade de Controle. Quando alto, habilita a escrita de dados (`rd_data_i`) no registrador de destino (`rd`) durante esta decodificação.

- **`rd_data_i`**: O valor a ser escrito no Arquivo de Registradores. Em um design de single-cycle, este é tipicamente o resultado do stage anterior (WB) que agora está sendo retroalimentado para escrita.

- **`imm_type_i`**: Sinal de controle que especifica qual tipo de imediato deve ser gerado (ImmI, ImmS, ImmB, ImmU, ImmJ). O Gerador de Imediato usará este sinal para saber como extrair e estender o imediato da instrução.

- **`rs1_data_o`, `rs2_data_o`**: As saídas dos dados lidos do Arquivo de Registradores. Estes são os valores brutos dos registradores Rs1 e Rs2 especificados pela instrução, que serão usados como operandos na ALU ou na Unidade de Desvios.

- **`imm_o`**: O valor de imediato de 32 bits (sign-extended) gerado pelo Gerador de Imediato. Será usado como segundo operando da ALU ou como deslocamento em instruções de memória.

## 3. Arquitetura

Este módulo é puramente um wrapper estrutural. Ele fatia a instrução de entrada `instr_i` para acionar as portas de endereço de leitura/escrita do `reg_file` instanciado e fornece os bits de dados úteis ao `imm_gen` instanciado. Atua como o fornecedor de dados para a ALU e a Unidade de Desvios no estágio de Execução subsequente.

### 3.1. Estrutura Interna

O estágio de decodificação é organizado em três componentes principais:

1. **Arquivo de Registradores (Register File)**
   - Contém 32 registradores de 32 bits (x0-x31)
   - Suporta duas leituras simultâneas (Rs1 e Rs2) e uma escrita (Rd)
   - A escrita é síncrona (borda de subida do clock)
   - As leituras são combinacionais (sem atraso)
   - x0 é sempre zero (escritas são ignoradas, leituras retornam 0)

2. **Gerador de Imediato (Immediate Generator)**
   - Extrai bits da instrução baseado no tipo de imediato
   - Realiza sign-extension para o tamanho de 32 bits
   - Suporta todos os 5 formatos RV32I: I, S, B, U, J

3. **Lógica de Seleção de Registrador**
   - Extrai Rs1 (bits [19:15]) e Rs2 (bits [24:20]) da instrução
   - Extrai Rd (bits [11:7]) para escrita
   - Fornece estes endereços ao Arquivo de Registradores

### 3.2. Considerações de Timing

- **Latência de leitura**: As leituras são combinacionais, então qualquer atraso é apenas propagação através da lógica de decodificação e do array de registradores (~1-2 ps em tecnologias modernas).
- **Latência de escrita**: A escrita é síncrona e acontece na borda de subida do clock. O novo dado está disponível para leitura no próximo ciclo.
- **Timing crítico**: Geralmente não é crítico, pois as leituras paralelas do RF são rápidas.

### 3.3. Casos Especiais

- **Leitura/Escrita de x0**: Rs1 ou Rs2 = x0 (operando zero é comum); Rd = x0 (escritas são ignoradas).
- **Forwarding**: Em pipelines com múltiplos estágios, se a instrução anterior escrever em um registrador que a instrução atual lê, pode haver atraso (data hazard). Técnicas de bypassing são usadas para evitar bolhas.
- **Escrita em Rd = instrução anterior**: Se a instrução i-1 escreve em Rs1 da instrução i, a leitura em i vê o valor antigo (antes da escrita em i-1 ser confirmada). Isso é resolvido com forwarding do estágio anterior.