# ==============================================================================
# PROJECT: SuperNova-RV
# FILE: rv32i_test.S
# DESCRIPTION: Fixed E2E test with corrected immediate ranges.
# ==============================================================================

.section .text
.global main

main:
    # --------------------------------------------------------------------------
    # 1. TEST: Integer Arithmetic (ADDI, ADD, SUB)
    # --------------------------------------------------------------------------
    addi x1, x0, 100        # x1 = 100
    addi x2, x0, 50         # x2 = 50
    add  x3, x1, x2         # x3 = 150
    sub  x4, x3, x1         # x4 = 50
    
    addi x5, x0, 50
    bne  x4, x5, fail

    # --------------------------------------------------------------------------
    # 2. TEST: Logical Operations (Using valid 12-bit signed immediates)
    # --------------------------------------------------------------------------
    addi x1, x0, -1         # x1 = 0xFFFFFFFF
    andi x2, x1, 0x0FF      # x2 = 0x000000FF (Fits: 255 < 2047)
    
    # 0xF00 (3840) is too big for ori, let's use a register for the mask
    li   x7, 0xF00          # Assembler expands this to lui + addi
    or   x3, x2, x7         # x3 = 0x00000FFF
    
    li   x4, 4095           # 4095 = 0x00000FFF
    bne  x3, x4, fail

    # --------------------------------------------------------------------------
    # 3. TEST: Shifts (SLLI, SRLI, SRAI)
    # --------------------------------------------------------------------------
    addi x1, x0, 1          
    slli x1, x1, 4          # x1 = 16
    srli x2, x1, 2          # x2 = 4
    
    addi x3, x0, -16        
    srai x4, x3, 2          # x4 = -4
    
    addi x5, x0, -4
    bne  x4, x5, fail

    # --------------------------------------------------------------------------
    # 4. TEST: Comparison
    # --------------------------------------------------------------------------
    addi x1, x0, -1         
    slti x2, x1, 1          # Signed: -1 < 1 -> x2 = 1
    sltiu x3, x1, 1         # Unsigned: MAX_INT < 1 -> x3 = 0
    
    bne  x2, x1, check_u    
    j    fail               
check_u:
    bne  x3, x0, fail       

    # --------------------------------------------------------------------------
    # 5. TEST: Memory Access
    # --------------------------------------------------------------------------
    addi x1, x0, 64         
    addi x2, x0, 1234       
    sw   x2, 0(x1)          
    lw   x3, 0(x1)          
    
    bne  x3, x2, fail       

    # --------------------------------------------------------------------------
    # 6. TEST: Upper Immediates (LUI)
    # --------------------------------------------------------------------------
    lui   x1, 0x12345       
    li    x2, 0x12345000    
    bne   x1, x2, fail

    # --------------------------------------------------------------------------
    # SUCCESS: Signal via MMIO (0x10000000)
    # --------------------------------------------------------------------------
success:
    li   x10, 0x10000000    # Debug Port
    li   x11, 0xDEADBEEF    # Success Code
    sw   x11, 0(x10)
    
wait_success:
    jal x0, wait_success

fail:
    li   x10, 0x10000000
    li   x11, 0xBAD0BAD0    # Failure Code
    sw   x11, 0(x10)
    
wait_fail:
    jal x0, wait_fail