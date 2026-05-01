# Instruction Fetch Stage (IF)

O estágio de **Instruction Fetch** (Busca de Instruções) é o primeiro estágio do pipeline do SuperNova-RV. Ele é responsável por manter o controle do fluxo do programa, calcular o próximo endereço a ser executado e buscar a instrução correspondente na memória.

## Decisão Microarquitetural: Interface Harvard
Para manter o núcleo da CPU altamente modular e compatível com futuras expansões (como a adição de Caches L1 ou barramentos AXI), a memória de instruções (ROM/RAM) **não** é instanciada internamente. Em vez disso, este estágio expõe uma interface de memória: ele emite o endereço atual (`instr_addr_o`) e espera receber a instrução (`instr_rdata_i`) do sistema de memória externo.

## Especificação da Interface

O módulo SystemVerilog (`hw/core/fetch_stage.sv`) expõe as seguintes portas:

| Porta | Direção | Largura | Descrição |
| :--- | :---: | :---: | :--- |
| `clk_i` | Entrada | 1 | Clock do sistema. |
| `rst_ni` | Entrada | 1 | Reset Assíncrono (Ativo em Baixo). Fundamental para carregar o `BOOT_ADDR` no PC. |
| `stall_i` | Entrada | 1 | Congela o PC no endereço atual (útil para Hazards de dados no pipeline). |
| `branch_valid_i`| Entrada | 1 | Sinal da Unidade de Branch indicando que um salto deve ocorrer. |
| `branch_target_i`| Entrada | 32 | Endereço de destino do salto (calculado pela ALU/BCU). |
| `instr_addr_o` | Saída | 32 | Endereço enviado à memória externa para leitura. |
| `instr_rdata_i` | Entrada | 32 | Instrução crua de 32-bits retornada pela memória externa. |
| `pc_o` | Saída | 32 | Valor atual do Program Counter (repassado ao estágio Decode). |
| `pc_next_o` | Saída | 32 | Endereço sequencial (`PC + 4`). Usado para salvar o retorno de funções (`JAL`/`JALR`). |
| `instr_o` | Saída | 32 | Instrução buscada (repassada ao estágio Decode). |

## Comportamento do Program Counter (PC)
1. **Boot:** Durante o `rst_ni = 0`, o PC é forçado de forma assíncrona para o parâmetro `BOOT_ADDR` (padrão `0x00000000`). Diferente dos registradores de uso geral, **o PC precisa de reset**.
2. **Execução Sequencial:** A cada ciclo de clock, se `stall_i` estiver inativo, o PC avança 4 bytes (`PC + 4`), pois o RISC-V é endereçado a byte e cada instrução RV32I possui 32 bits (4 bytes).
3. **Desvios (Branches/Jumps):** Se `branch_valid_i` for ativado no ciclo atual, o PC descartará a execução sequencial e carregará o `branch_target_i` na próxima borda de subida.