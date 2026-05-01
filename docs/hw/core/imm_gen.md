# Gerador de Imediatos (ImmGen)

## Contexto
O Gerador de Imediatos é um módulo puramente combinacional responsável por extrair e estender o sinal dos valores imediatos codificados nas instruções RISC-V de 32 bits. Ele reconstrói o valor imediato baseado no formato de instrução ditado pela Unidade de Controle.

## Interface
| Nome do Sinal | Direção | Largura | Descrição |
| :---          | :---    | :---    | :---      |
| `instr_i`     | Entrada | 32      | A instrução bruta de 32 bits buscada da memória. |
| `imm_type_i`  | Entrada | Enum    | Sinal de controle indicando o formato da instrução (I, S, B, U, J). |
| `imm_o`       | Saída   | 32      | O valor imediato reconstruído e estendido em sinal de 32 bits. |

## Arquitetura
O módulo utiliza um único bloco `always_comb` com uma declaração `case` controlada por `imm_type_i`. Ele aproveita os operadores de concatenação `{}` e replicação `{N{}}` do SystemVerilog para realizar eficientemente o fatiamento de bits e extensão de sinal. Os imediatos dos tipos B e J têm um bit menos significativo implícito de 0, que é codificado na concatenação.

## Formatos de Instruções RV32I

O conjunto de instruções RISC-V RV32I utiliza cinco formatos principais de instruções, cada um com um layout diferente para codificar o valor imediato. O ImmGen deve extrair corretamente o imediato de cada formato:

### Formato I (I-type)
Utilizado por instruções como `ADDI`, `LW`, `JALR`.

```
┌─────────────────────────────────────────────────────┐
│ imm[11:0] │ rs1[4:0] │ funct3[2:0] │ rd[4:0] │ opcode │
│   [31:20] │ [19:15]  │  [14:12]    │ [11:7]  │ [6:0]  │
└─────────────────────────────────────────────────────┘
```

- **Imediato**: bits [31:20] (12 bits), estendido em sinal para 32 bits
- **Intervalo**: -2048 a +2047
- **Exemplo**: `addi x1, x2, 100` → imm = 0x064 (com extensão de sinal)

### Formato S (S-type)
Utilizado por instruções de armazenamento como `SW`, `SH`, `SB`.

```
┌───────────┬─────────────────────────────────────┬────────────────┐
│ imm[11:5] │ rs2[4:0] │ rs1[4:0] │ funct3[2:0] │ imm[4:0] │ op │
│ [31:25]   │ [24:20]  │ [19:15]  │  [14:12]    │ [11:7]   │[6:0]
└───────────┴─────────────────────────────────────┴────────────────┘
```

- **Imediato**: bits [31:25] + bits [11:7], recombinados como imm[11:0]
- **Intervalo**: -2048 a +2047
- **Exemplo**: `sw x2, 100(x1)` → imm = 0x064

### Formato B (B-type)
Utilizado por instruções de branch como `BEQ`, `BNE`, `BLT`.

```
┌──────────┬──────────────────────────────────┬──────────┬────────┐
│imm[12|10:5]│ rs2[4:0] │ rs1[4:0] │ funct3 │ imm[4:1] │ opcode │
│ [31]|[30:25]│ [24:20]  │ [19:15]  │ [14:12]│ [11:8]   │ [6:0]  │
└──────────┴──────────────────────────────────┴──────────┴────────┘
```

- **Imediato**: bits [31], [7], [30:25], [11:8], recombinados com LSB implícito = 0
- **Intervalo**: -4096 a +4094 (múltiplos de 2)
- **Exemplo**: `beq x1, x2, offset` → imm é estendido em sinal com bit 0 = 0

### Formato U (U-type)
Utilizado por instruções que carregam constantes grandes como `LUI`, `AUIPC`.

```
┌───────────────────────────────┬──────────┬────────┐
│     imm[31:12]                │ rd[4:0]  │ opcode │
│     [31:12]                   │ [11:7]   │ [6:0]  │
└───────────────────────────────┴──────────┴────────┘
```

- **Imediato**: bits [31:12] (20 bits), deslocados 12 bits para a esquerda
- **Intervalo**: -524288 a +524287 (em passos de 4096)
- **Exemplo**: `lui x1, 0x12345` → imm = 0x12345000

### Formato J (J-type)
Utilizado por instruções de salto incondicional como `JAL`.

```
┌──────────┬───────────┬──────┬──────────────────┬────────┐
│imm[20|10:1|11|19:12]│ rd[4:0]│     opcode      │
│[31]|[19:12]|[20]|[30:21]│ [11:7]│     [6:0]      │
└──────────┴───────────┴──────┴──────────────────┴────────┘
```

- **Imediato**: bits [31], [19:12], [20], [30:21], recombinados com LSB implícito = 0
- **Intervalo**: -1048576 a +1048574 (múltiplos de 2)
- **Exemplo**: `jal x1, label` → imm é estendido em sinal com bit 0 = 0

## Processo de Extração

O ImmGen executa os seguintes passos para cada formato:

1. **Fatiamento**: Extrai os bits relevantes da instrução de entrada (`instr_i`)
2. **Recombinação**: Reorganiza os bits conforme o layout específico do formato
3. **Extensão de Sinal**: Replica o bit de sinal (MSB) para os bits superiores
4. **Saída**: Fornece o imediato de 32 bits estendido em sinal via `imm_o`

## Exemplo de Implementação

```systemverilog
always_comb begin
  case (imm_type_i)
    I_TYPE: imm_o = {{20{instr_i[31]}}, instr_i[31:20]};
    S_TYPE: imm_o = {{20{instr_i[31]}}, instr_i[31:25], instr_i[11:7]};
    B_TYPE: imm_o = {{20{instr_i[31]}}, instr_i[31], instr_i[7], 
                      instr_i[30:25], instr_i[11:8], 1'b0};
    U_TYPE: imm_o = {instr_i[31:12], 12'b0};
    J_TYPE: imm_o = {{12{instr_i[31]}}, instr_i[31], instr_i[19:12],
                      instr_i[20], instr_i[30:21], 1'b0};
    default: imm_o = 32'b0;
  endcase
end
```

## Notas Importantes

- **Extensão de Sinal**: Todos os formatos, exceto o U-type, utilizam extensão de sinal para manter a representação correta de números negativos.
- **Bits Implícitos**: Os formatos B-type e J-type possuem um bit menos significativo implícito de 0, que é automaticamente concatenado durante a extração.
- **Operação Combinacional**: O módulo é puramente combinacional, sem lógica sequencial, permitindo que a saída seja imediatamente disponível após a entrada.