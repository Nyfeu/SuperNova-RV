# Unidade de Desvios (BU - Branch Unit)

## Contexto
A Unidade de Desvios (Branch Unit) avalia desvios condicionais no processador SuperNova-RV. Como a ALU principal é utilizada para computar o endereço alvo do desvio (`PC + Imediato`) durante uma instrução de desvio, a Unidade de Desvios funciona como um comparador independente. Ela recebe os dados brutos do Arquivo de Registradores (`rs1` e `rs2`), avalia a condição especificada por `Funct3`, e asserti um único sinal `branch_taken_o` se a condição for satisfeita.

A arquitetura de projeto plano (flat design) do SuperNova-RV separa a resolução de desvios em dois componentes:
1. **Instruction Decoder**: Identifica instruções de desvio e fornece `is_branch_i` e `funct3_i`
2. **Branch Unit**: Avalia a condição de forma independente e paralela à execução de outras operações

Essa separação permite que o processador resolva desvios rapidamente sem bloquear a ALU.

## Interface

### Sinais de Entrada/Saída

| Nome do Sinal     | Direção | Largura/Tipo  | Descrição |
| :---              | :---    | :---          | :---      |
| `rs1_data_i`      | Entrada | 32 bits       | Dados do Registrador Fonte 1 (Rs1). |
| `rs2_data_i`      | Entrada | 32 bits       | Dados do Registrador Fonte 2 (Rs2). |
| `is_branch_i`     | Entrada | 1 bit         | Sinal de habilitação vindo do Decodificador de Instruções. |
| `funct3_i`        | Entrada | 3 bits        | Seletor de condição vindo da instrução. |
| `branch_taken_o`  | Saída   | 1 bit         | Alto (1) se a condição de desvio for satisfeita. |

### Descrição dos Sinais

- **`rs1_data_i` e `rs2_data_i`**: Valores brutos lidos do Arquivo de Registradores para os operandos do desvio. Não há atraso de forwarding aqui; os dados vêm diretamente do RF.
- **`is_branch_i`**: Sinal de controle vindo do Decodificador de Instruções. Quando alto, indica que a instrução atual é um desvio condicional e a Unidade de Desvios deve avaliar a condição.
- **`funct3_i`**: Campo de 3 bits da instrução (bits [14:12]) que codifica qual condição deve ser avaliada (EQ, NE, LT, GE, LTU, GEU).
- **`branch_taken_o`**: Saída única de 1 bit. Alto se a condição for satisfeita, baixo caso contrário. Este sinal é usado pelo controlador de PC para decidir se o salto deve ser feito.

## Condições Suportadas (Baseado em Funct3)

A Unidade de Desvios implementa seis comparadores paralelos para suportar todas as condições de desvio RV32I:

| Funct3 | Mnemônico | Condição | Descrição |
| :---   | :---      | :---     | :---      |
| `000`  | BEQ       | `rs1 == rs2` | Desvio se Igual |
| `001`  | BNE       | `rs1 != rs2` | Desvio se Diferente |
| `100`  | BLT       | `$signed(rs1) < $signed(rs2)` | Desvio se Menor (com sinal) |
| `101`  | BGE       | `$signed(rs1) >= $signed(rs2)` | Desvio se Maior ou Igual (com sinal) |
| `110`  | BLTU      | `rs1 < rs2` (sem sinal) | Desvio se Menor (sem sinal) |
| `111`  | BGEU      | `rs1 >= rs2` (sem sinal) | Desvio se Maior ou Igual (sem sinal) |

### Detalhes das Operações de Comparação

- **BEQ (Branch if Equal, `000`)**: Executa `rs1 == rs2`. Resultado é um bit que indica igualdade.
- **BNE (Branch if Not Equal, `001`)**: Executa `rs1 != rs2`. Resultado é um bit que indica desigualdade (inverso de BEQ).
- **BLT (Branch if Less Than Signed, `100`)**: Compara `rs1` e `rs2` como inteiros com sinal (two's complement). True se rs1 < rs2 em aritmética com sinal.
- **BGE (Branch if Greater or Equal Signed, `101`)**: Compara `rs1` e `rs2` como inteiros com sinal. True se rs1 >= rs2. Inverso de BLT.
- **BLTU (Branch if Less Than Unsigned, `110`)**: Compara `rs1` e `rs2` como inteiros sem sinal. True se rs1 < rs2 em aritmética sem sinal.
- **BGEU (Branch if Greater or Equal Unsigned, `111`)**: Compara `rs1` e `rs2` como inteiros sem sinal. True se rs1 >= rs2. Inverso de BLTU.

## Arquitetura de Projeto e Implementação

### Design Combinacional
A Unidade de Desvios é um módulo puramente combinacional que não contém estado sequencial. Todos os comparadores funcionam em paralelo, e o sinal de saída `branch_taken_o` é derivado combinacionalmente dos sinais de entrada.

Este design oferece:
- **Latência baixa**: Uma única etapa combinacional (típica da tecnologia CMOS moderno).
- **Throughput**: Novo resultado a cada ciclo de clock (já que não há dependência de ciclo anterior).
- **Simplicidade**: Nenhuma sincronização complexa ou arbitragem.

### Habilitação por `is_branch_i`
O sinal `is_branch_i` atua como um habilitador geral. Quando baixo, a saída `branch_taken_o` é sempre zero, mesmo que uma condição fosse teoricamente verdadeira. Isso garante que a Unidade de Desvios não interfira com instruções não-branch.

## Exemplos de Instruções

### Formato das Instruções de Desvio (B-Type)
```
imm[12|10:5] | rs2[4:0] | rs1[4:0] | funct3[2:0] | imm[4:1|11] | opcode[6:0]
```

Opcode para todas as instruções de desvio: `0x63` (bits 6:0 = `1100011`)

### Exemplos Práticos

- **BEQ x1, x2, 16**
  - Salta 16 bytes adiante se `x1 == x2`
  - `funct3 = 000`
  - Imediato (offset) = 16 bytes (branch offset é sempre um múltiplo de 2)

- **BNE x5, x6, -8**
  - Salta 8 bytes atrás se `x5 != x6`
  - `funct3 = 001`
  - Imediato (offset) = -8 (signed, two's complement)

- **BLT x10, x11, 32**
  - Salta 32 bytes adiante se `$signed(x10) < $signed(x11)`
  - `funct3 = 100`
  - Comparação com sinal

- **BLTU x3, x4, 24**
  - Salta 24 bytes adiante se `x3 < x4` (sem sinal)
  - `funct3 = 110`
  - Comparação sem sinal

## Considerações de Design e Otimização

### Seleção de Arquitetura
A escolha entre implementações (comparadores paralelos vs. mux hierárquico) depende de:
- **Velocidade crítica**: Se a latência de desvio for crítica, comparadores paralelos com mux rápido é preferível.
- **Área de silício**: Múltiplos comparadores podem aumentar a área; um mux sequencial a reduz.
- **Consumo de energia**: Comparadores sempre ativos consomem potência dinâmica mesmo quando não usados.

### Forwarding de Dados
Se Rs1 ou Rs2 for escrito pela instrução anterior, técnicas de forwarding são necessárias para evitar atraso:
- Em pipelines simples, a Unidade de Desvios lê diretamente do RF (sem forwarding).
- Em pipelines mais profundos, lógica de bypassing permite usar dados do stage anterior.

### Tratamento de Casos Especiais
- **Desvio para si mesmo**: `BEQ x1, x1, 0` é sempre verdadeiro (mesmo registrador sempre é igual a si mesmo).
- **Comparação com x0**: Útil para testar se um registrador é zero (`BEQ x1, x0, label` salta se x1 == 0).
- **Desvios de loop**: Desvios retroativos (backward branches) são comuns em loops e podem se beneficiar de previsão.
