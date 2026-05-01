# Load Store Unit (LSU)

## Context
The Load Store Unit sits between the CPU Datapath (Register File / ALU) and the Data Memory. It is a purely combinational datapath module. It is responsible for aligning data to be written to memory based on the address's lower bits and generating the correct byte-enable masks. Conversely, it extracts, aligns, and sign/zero-extends data read from memory back to the CPU.

## Interface
| Signal Name      | Direction | Width | Description |
| :---             | :---      | :---  | :---        |
| `addr_i`         | Input     | 32    | The memory address calculated by the ALU. |
| `wdata_i`        | Input     | 32    | Data to be written (from Register File rs2). |
| `mem_size_i`     | Input     | Enum  | Access size (Byte, Half-word, Word). |
| `mem_unsigned_i` | Input     | 1     | 1 for unsigned loads (lbu, lhu), 0 for signed (lb, lh, lw). |
| `mem_rdata_i`    | Input     | 32    | Raw data read from the aligned memory address. |
| `mem_addr_o`     | Output    | 32    | Word-aligned address sent to Data Memory. |
| `mem_wdata_o`    | Output    | 32    | Shifted/Formatted data sent to Data Memory. |
| `mem_be_o`       | Output    | 4     | Byte Enables (mask) indicating which bytes to write. |
| `rdata_o`        | Output    | 32    | Extracted and sign/zero-extended data back to Register File. |

## Architecture
The LSU aligns memory operations. Since RISC-V RV32I uses byte-addressing but reads/writes occur in 32-bit memory blocks, the lower 2 bits of `addr_i` determine the offset.
- **Store:** The 8 or 16 bits of `wdata_i` are shifted left to match the address offset. `mem_be_o` is asserted accordingly.
- **Load:** The relevant 8 or 16 bits from `mem_rdata_i` are shifted right and conditionally sign-extended.