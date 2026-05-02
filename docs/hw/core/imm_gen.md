# Gerador de Imediatos (ImmGen)

## 1. Contexto
O Gerador de Imediatos é um módulo puramente combinacional responsável por extrair e estender o sinal dos valores imediatos codificados nas instruções RISC-V de 32 bits. Ele reconstrói o valor imediato baseado no formato de instrução ditado pela Unidade de Controle.

## 2. Interface
| Nome do Sinal | Direção | Largura | Descrição |
| :---          | :---    | :---    | :---      |
| `instr_i`     | Entrada | 28      | A instrução bruta de 32 bits sem o `op_code` buscada da memória. |
| `imm_type_i`  | Entrada | Enum    | Sinal de controle indicando o formato da instrução (I, S, B, U, J). |
| `imm_o`       | Saída   | 32      | O valor imediato reconstruído e estendido em sinal de 32 bits. |

## 3. Arquitetura
O módulo utiliza um único bloco `always_comb` com uma declaração `case` controlada por `imm_type_i`. Ele aproveita os operadores de concatenação `{}` e replicação `{N{}}` do SystemVerilog para realizar eficientemente o fatiamento de bits e extensão de sinal. Os imediatos dos tipos B e J têm um bit menos significativo implícito de 0, que é codificado na concatenação.

## 4. Formatos de Instruções RV32I

Para acomodar diferentes necessidades (operações matemáticas, acessos à memória e saltos), as instruções são divididas em 6 formatos básicos:

- **Tipo-R (Register):** Usado para operações aritméticas e lógicas onde todos os operandos são registradores (ex: `ADD`, `SUB`, `AND`).

- **Tipo-I (Immediate):** Usado para operações com imediatos curtos (12 bits), instruções de Load (leitura de memória) e o salto `JALR`.

- **Tipo-S (Store):** Usado exclusivamente para escrever na memória (ex: `SW`, `SB`). O imediato de 12 bits é dividido em duas partes para manter `rs1` e `rs2` em seus lugares fixos.

- **Tipo-B (Branch):** Usado para saltos condicionais (`BEQ`, `BNE`). Muito semelhante ao Tipo-S, mas o imediato codifica múltiplos de 2 (visto que as instruções são alinhadas na memória).

- **Tipo-U (Upper Immediate):** Usado para carregar imediatos longos (20 bits) nos bits mais significativos de um registrador (`LUI`, `AUIPC`).

- **Tipo-J (Jump):** Usado para saltos incondicionais longos (`JAL`).

![Formatos de Instruções](../../assets/instruction_format.png){ .hero-img }

## Processo de Extração

O ImmGen executa os seguintes passos para cada formato:

1. **Fatiamento**: Extrai os bits relevantes da instrução de entrada (`instr_i`)
2. **Recombinação**: Reorganiza os bits conforme o layout específico do formato
3. **Extensão de Sinal**: Replica o bit de sinal (MSB) para os bits superiores
4. **Saída**: Fornece o imediato de 32 bits estendido em sinal via `imm_o`
