# Load Store Unit

## Contexto
A Unidade de Carregamento/Armazenamento fica entre o Caminho de Dados da CPU (Arquivo de Registradores / ALU) e a Memória de Dados. É um módulo de caminho de dados puramente combinacional. É responsável por alinhar dados a serem escritos em memória baseado nos bits inferiores do endereço e gerar as máscaras corretas de habilitação de byte. Inversamente, ela extrai, alinha e faz sign/zero-extend de dados lidos da memória de volta para a CPU.

## Interface

| Nome do Sinal    | Direção | Largura | Descrição |
| :---             | :---    | :---    | :---      |
| `addr_i`         | Entrada | 32      | O endereço de memória calculado pela ALU. |
| `wdata_i`        | Entrada | 32      | Dados a serem escritos (do registrador Rs2 do Arquivo de Registradores). |
| `mem_size_i`     | Entrada | Enum    | Tamanho de acesso (Byte, Half-word, Word). |
| `mem_unsigned_i` | Entrada | 1       | 1 para carregamentos sem sinal (lbu, lhu), 0 para com sinal (lb, lh, lw). |
| `mem_rdata_i`    | Entrada | 32      | Dados brutos lidos do endereço de memória alinhado. |
| `mem_addr_o`     | Saída   | 32      | Endereço alinhado à palavra enviado à Memória de Dados. |
| `mem_wdata_o`    | Saída   | 32      | Dados deslocados/Formatados enviados à Memória de Dados. |
| `mem_be_o`       | Saída   | 4       | Habilitadores de Byte (máscara) indicando quais bytes devem ser escritos. |
| `rdata_o`        | Saída   | 32      | Dados extraídos e com sign/zero-extended de volta para o Arquivo de Registradores. |

### Descrição dos Sinais

- **`addr_i`**: O endereço de memória de 32 bits calculado pela ALU. Os bits inferiores (bits [1:0]) determinam o offset dentro da palavra de 32 bits.

- **`wdata_i`**: Os dados de 32 bits a serem escritos, que vêm do Arquivo de Registradores (registrador Rs2). A LSU extrairá os bits relevantes (8 ou 16 bits) baseado em `mem_size_i`.

- **`mem_size_i`**: Enum que especifica o tamanho da operação (8 bits para byte, 16 bits para half-word, 32 bits para word).

- **`mem_unsigned_i`**: Sinalizador que indica se a carga é sem sinal (zero-extend) ou com sinal (sign-extend). Irrelevante para armazenamentos.

- **`mem_rdata_i`**: Dados brutos de 32 bits lidos da memória. A LSU extrairá e processará os bits relevantes.

- **`mem_addr_o`**: Endereço enviado à memória. Tipicamente alinhado em palavras (bits [1:0] = 0), permitindo leituras/escritas de 32 bits.

- **`mem_wdata_o`**: Dados formatados para escrita. Contém o valor a escrever, deslocado para o offset correto dentro da palavra de 32 bits.

- **`mem_be_o`**: Habilitadores de byte (4 bits, um para cada byte da palavra). Indica quais bytes devem ser escritos (1 = escrever, 0 = não escrever).

- **`rdata_o`**: Dados retornados ao Arquivo de Registradores após leitura de memória. Os bits irrelevantes foram extraídos e o valor foi estendido (sign-extended ou zero-extended conforme necessário).

## Arquitetura

A LSU alinha operações de memória. Como RISC-V RV32I usa endereçamento em bytes, mas leituras/escritas ocorrem em blocos de memória de 32 bits, os 2 bits inferiores de `addr_i` determinam o offset.

### Operações de Armazenamento (Store)

- Os 8 ou 16 bits de `wdata_i` são deslocados à esquerda para corresponder ao offset do endereço.
- `mem_be_o` é assertado conforme necessário para indicar quais bytes devem ser escritos.

#### Exemplo de Armazenamento de Byte (SB)

Se `addr_i[1:0] = 2'b01` (offset 1 byte), e desejamos escrever um byte:
```
wdata_i = 0x000000AB
mem_addr_o = addr_i & 32'hFFFFFFFC  (alinha para palavra)
mem_wdata_o = 0x0000AB00  (byte deslocado para posição correta)
mem_be_o = 4'b0010  (habilita apenas o segundo byte)
```

#### Exemplo de Armazenamento de Half-word (SH)

Se `addr_i[1:0] = 2'b10` (offset 2 bytes), e desejamos escrever um half-word:
```
wdata_i = 0x0000ABCD
mem_addr_o = addr_i & 32'hFFFFFFFC
mem_wdata_o = 0xABCD0000  (half-word deslocado para posição correta)
mem_be_o = 4'b1100  (habilita os dois bytes superiores)
```

#### Exemplo de Armazenamento de Word (SW)

O endereço já deve estar alinhado em palavras (bits [1:0] = 0):
```
wdata_i = 0x12345678
mem_addr_o = addr_i & 32'hFFFFFFFC
mem_wdata_o = 0x12345678  (sem deslocamento necessário)
mem_be_o = 4'b1111  (todos os 4 bytes habilitados)
```

### Operações de Carregamento (Load)

- Os bits relevantes de `mem_rdata_i` são deslocados à direita e opcionalmente sign-extended.
- O resultado é uma palavra completa de 32 bits, com extensão apropriada.

#### Exemplo de Carregamento de Byte com Sinal (LB)

Se `addr_i[1:0] = 2'b01` (offset 1 byte), e `mem_rdata_i = 0x12AB34CD`:
```
mem_rdata_i = 0x12AB34CD
byte_value = 0xAB (segundo byte)
sign_extended = (0xAB[7] == 1) ? 0xFFFFFFAB : 0x000000AB
rdata_o = 0xFFFFFFAB  (se sinal negativo, 0xAB = -85)
```

#### Exemplo de Carregamento de Byte sem Sinal (LBU)

Mesmo cenário, mas com zero-extend:
```
mem_rdata_i = 0x12AB34CD
byte_value = 0xAB
zero_extended = 0x000000AB
rdata_o = 0x000000AB  (sempre positivo)
```

#### Exemplo de Carregamento de Half-word com Sinal (LH)

Se `addr_i[1:0] = 2'b10` (offset 2 bytes), e `mem_rdata_i = 0xFFFF1234`:
```
mem_rdata_i = 0xFFFF1234
halfword_value = 0xFF12 (dois bytes superiores)
sign_extended = (0xFF12[15] == 1) ? 0xFFFFFF12 : 0x0000FF12
rdata_o = 0xFFFFFF12  (se sinal negativo)
```

#### Exemplo de Carregamento de Word (LW)

O endereço deve estar alinhado em palavras:
```
mem_rdata_i = 0x12345678
rdata_o = 0x12345678  (sem processamento necessário)
```

## Considerações de Design

### Capacidade de Memória
- A LSU assume que a memória de dados já é alinhada em palavras (word-aligned).
- A memória fornece uma palavra completa de 32 bits em cada leitura, independentemente do tamanho solicitado.
- Para escritas, o controlador de memória usa `mem_be_o` para escrever apenas os bytes desejados.

### Casos Especiais

- **Endereços não alinhados**: RV32I permite cargas/armazenamentos de bytes e half-words não alinhados. A LSU trata isso corretamente com deslocamentos.
- **Boundary conditions**: Se um half-word começar em um byte limite (offset 3), ainda funciona, mas pode cruzar uma palavra (não suportado em algumas implementações; RV32I permite).
- **Big-endian vs Little-endian**: O design acima assume **little-endian**. Para big-endian, o cálculo de offset seria invertido.
