# Estágio de Aceesso à Memória 

## Contexto

O estágio de acesso à memória é o quarto bloco lógico do caminho de dados (*datapath*). Ele age como uma ponte entre o núcleo de execução do processador e acesso à memória de dados (via DMem - barramento de dados). Ele encapsula a Load-Store Unit (LSU), lidando com alinhamento, byte-enables, e sign-extension para leituras e escritas na memória. 

## Interface
| Signal Name      | Direction | Width/Type      | Description |
| :---             | :---      | :---            | :---        |
| `alu_result_i`   | Input     | 32-bit logic    | Memory address computed by the ALU. |
| `rs2_data_i`     | Input     | 32-bit logic    | Raw data to be written (from Decode). |
| `mem_we_i`       | Input     | 1-bit logic     | Write Enable from Control Unit. |
| `mem_size_i`     | Input     | `mem_size_e`    | Memory access size (Byte, Half, Word). |
| `mem_unsigned_i` | Input     | 1-bit logic     | Zero-extension flag for Loads. |
| `dmem_rdata_i`   | Input     | 32-bit logic    | Raw data read from DMEM. |
| `dmem_addr_o`    | Output    | 32-bit logic    | Physical address sent to DMEM. |
| `dmem_wdata_o`   | Output    | 32-bit logic    | Formatted data to write to DMEM. |
| `dmem_be_o`      | Output    | 4-bit logic     | Byte Enables for DMEM writes. |
| `lsu_rdata_o`    | Output    | 32-bit logic    | Formatted, sign-extended data read from memory. |

## Architecture

É um componente puramente estrutural. Instancia o módulo `lsu.sv` e traduz requisições internas em trasanções externas válidas.