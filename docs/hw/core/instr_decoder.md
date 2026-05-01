# Decodificador de Instruções (ID)

## Contexto
O Decodificador de Instruções é o núcleo do Caminho de Controle (Control Path) na arquitetura SuperNova-RV. É um módulo puramente combinacional que implementa uma estratégia de decodificação em "projeto plano" (flat design). Em vez de depender de múltiplos decodificadores hierárquicos (como um decodificador principal seguido por um decodificador de ALU), ele lê os campos `Opcode`, `Funct3` e `Funct7` simultaneamente para gerar todos os sinais de controle fortemente tipados em um único estágio. Essa abordagem achata o caminho crítico e reduz a latência.

A arquitetura segue o conjunto de instruções base RV32I (RISC-V 32-bit Integer) conforme especificado na ISA RISC-V, garantindo compatibilidade com software padrão dessa plataforma.

## Interface

### Sinais de Entrada/Saída

| Nome do Sinal     | Direção | Largura/Tipo  | Descrição |
| :---              | :---    | :---          | :---      |
| `instr_i`         | Entrada | 32 bits       | A instrução bruta de 32 bits obtida da memória. |
| `reg_we_o`        | Saída   | 1 bit         | Habilitação de Escrita no Arquivo de Registradores (Register File Write Enable). |
| `imm_type_o`      | Saída   | `imm_type_e`  | Seletor de formato para o Gerador de Imediato (Immediate Generator). |
| `alu_src_a_o`     | Saída   | `alu_src_a_e` | Seleciona o primeiro operando da ALU (Rs1, PC ou Zero). |
| `alu_src_b_o`     | Saída   | `alu_src_b_e` | Seleciona o segundo operando da ALU (Rs2 ou Imediato). |
| `alu_op_o`        | Saída   | `alu_op_e`    | Código de operação específica para a ALU. |
| `mem_we_o`        | Saída   | 1 bit         | Habilitação de Escrita na Memória de Dados (Store). |
| `mem_size_o`      | Saída   | `mem_size_e`  | Tamanho de acesso à Memória de Dados (Byte, Half-word, Word). |
| `mem_unsigned_o`  | Saída   | 1 bit         | Sinalizador de extensão com zeros para Carregamentos (Loads). |
| `wb_src_o`        | Saída   | `wb_src_e`    | Seleciona a fonte de dados para escrita no Arquivo de Registradores (Write-back). |
| `is_branch_o`     | Saída   | 1 bit         | Indica se a instrução é um desvio condicional (B-Type). |
| `is_jump_o`       | Saída   | 1 bit         | Indica se a instrução é um salto incondicional (JAL / JALR). |

### Descrição dos Sinais de Controle

- **`reg_we_o`**: Habilita a escrita do resultado no registrador de destino (Rd).
- **`imm_type_o`**: Define o tipo de imediato que será gerado (I, S, B, U ou J) para estar disponível na próxima etapa.
- **`alu_src_a_o` / `alu_src_b_o`**: Controlam os multiplexadores de entrada da ALU, selecionando a origem de cada operando.
- **`alu_op_o`**: Especifica a operação aritmética ou lógica a ser realizada (Add, Sub, AND, OR, XOR, SLL, SRL, SRA, etc.).
- **`mem_we_o`**: Indica que uma operação de escrita em memória (Store) será executada.
- **`mem_size_o`**: Define se o acesso é de 8 bits (byte), 16 bits (half-word) ou 32 bits (word).
- **`mem_unsigned_o`**: Quando alto, carregamentos são zero-extended; quando baixo, são sign-extended.
- **`wb_src_o`**: Seleciona a origem do dado a ser escrito: resultado da ALU, dado da memória, ou PC+4.
- **`is_branch_o` / `is_jump_o`**: Sinalizam instruções de controle de fluxo para que o núcleo execute a resolução de desvios apropriada.

## Estratégia de Decodificação e Tabela de Verdade

O decodificador mapeia a instrução para os sinais de controle baseado nos opcodes base do RV32I.

| Categoria de Instrução | Opcode    | `reg_we` | `imm_type` | `alu_src_a` | `alu_src_b` | `alu_op`      | `mem_we` | `wb_src`   |
| :---                   | :---      | :---     | :---       | :---        | :---        | :---          | :---     | :---       |
| **R-Type**             | `0110011` | 1        | X          | `Rs1`       | `Rs2`       | *Decodificado*| 0        | `Alu`      |
| **I-Type (ALU)**       | `0010011` | 1        | `ImmI`     | `Rs1`       | `Imm`       | *Decodificado*| 0        | `Alu`      |
| **Load**               | `0000011` | 1        | `ImmI`     | `Rs1`       | `Imm`       | `AluAdd`      | 0        | `Mem`      |
| **Store**              | `0100011` | 0        | `ImmS`     | `Rs1`       | `Imm`       | `AluAdd`      | 1        | X          |
| **Branch**             | `1100011` | 0        | `ImmB`     | `Rs1`       | `Rs2`       | `AluAdd` (*)  | 0        | X          |
| **JAL**                | `1101111` | 1        | `ImmJ`     | `Pc`        | `Imm`       | `AluAdd`      | 0        | `Pc` (**)  |
| **JALR**               | `1100111` | 1        | `ImmI`     | `Rs1`       | `Imm`       | `AluAdd`      | 0        | `Pc` (**)  |
| **LUI**                | `0110111` | 1        | `ImmU`     | `Zero`      | `Imm`       | `AluAdd`      | 0        | `Alu`      |
| **AUIPC**              | `0010111` | 1        | `ImmU`     | `Pc`        | `Imm`       | `AluAdd`      | 0        | `Alu`      |

*(X indica um valor "Don't Care", normalmente fixado em valores padrão para evitar latches).*  
*( * ) Desvios utilizam uma Unidade de Desvios dedicada no controle de nível superior; a ALU pode calcular endereços alvo ou permanecer inativa.*  
*( ** ) Saltos escrevem PC+4 no registrador de destino, portanto `wb_src` roteia o registrador PC do pipeline.*

### Detalhes dos Tipos de Instrução

- **R-Type**: Operações entre registradores (ex: ADD, SUB, AND, OR). Todos os operandos vêm de registradores.
- **I-Type**: Operações imediatas e carregamentos (ex: ADDI, LW, LH). Um operando é um registrador, o outro é um imediato de 12 bits.
- **S-Type**: Armazenamentos em memória (ex: SW, SH, SB). Os operandos são um registrador e um imediato para offset.
- **B-Type**: Desvios condicionais (ex: BEQ, BNE, BLT). Comparam dois registradores e fazem salto baseado em imediato.
- **U-Type**: Instruções com imediato de 20 bits (ex: LUI, AUIPC). Carregam valor imediato em bits superiores.
- **J-Type**: Saltos incondicionais (JAL). Carregam o endereço de retorno em um registrador.

## Operações Decodificadas da ALU

Para instruções `R-Type` e `I-Type`, o `alu_op_o` é derivado analisando `Funct3` (bits 14:12) e, quando necessário, `Funct7` (bit 30, que distingue ADD/SUB e SRL/SRA). Para todas as instruções de memória, PC-relativas e de salto, a ALU é forçada para `AluAdd` para calcular endereços de memória ou PCs alvo.

### Tabela de Operações ALU (para R-Type e I-Type)

| Funct3 | Funct7 (R-Type) | Operação | Descrição |
| :---   | :---            | :---     | :---      |
| `000`  | `0000000`       | ADD      | Adição (Rs1 + Rs2 / Rs1 + Imm) |
| `000`  | `0100000`       | SUB      | Subtração (Rs1 - Rs2, apenas R-Type) |
| `001`  | `0000000`       | SLL      | Deslocamento lógico à esquerda (Rs1 << Rs2[4:0] / Rs1 << Imm[4:0]) |
| `010`  | `0000000`       | SLT      | Set Less Than com sinal |
| `011`  | `0000000`       | SLTU     | Set Less Than sem sinal |
| `100`  | `0000000`       | XOR      | Operação XOR lógica |
| `101`  | `0000000`       | SRL      | Deslocamento lógico à direita |
| `101`  | `0100000`       | SRA      | Deslocamento aritmético à direita |
| `110`  | `0000000`       | OR       | Operação OR lógica |
| `111`  | `0000000`       | AND      | Operação AND lógica |

## Implementação e Considerações de Design

### Projeto Combinacional Plano
O decodificador implementa toda a lógica de decodificação em um único nível combinacional, evitando a necessidade de múltiplos estágios hierárquicos. Isso resulta em:
- **Menor latência**: Nenhum atraso de propagação entre decodificadores intermediários.
- **Previsibilidade**: O tempo crítico é bem definido e fácil de otimizar.
- **Facilidade de depuração**: A lógica é centralizada e transparente.

### Tratamento de Valores "Don't Care"
Sinais com valor "X" devem ser sempre fixados em valores padrão conhecidos para evitar comportamentos não determinísticos que possam levar a latches indesejados.

### Sinalização de Erros
O decodificador atualmente não sinaliza instruções inválidas explicitamente. Um módulo de detecção de exceções de nível superior é responsável por identificar opcodes não implementados ou instruções malformadas.