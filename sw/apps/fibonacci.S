.section .text
.global main

main:
    li   s0, 0              # F0
    li   s1, 1              # F1
    li   s2, 10             # Contador de iterações

fib_loop:
    mv   a0, s0             # Valor para imprimir
    jal  ra, print_int      # Chama conversor ASCII
    
    # Lógica de Fibonacci
    add  t1, s0, s1
    mv   s0, s1
    mv   s1, t1
    
    addi s2, s2, -1
    bnez s2, fib_loop

    # Finaliza com Sucesso
    li   t0, 0x10000000
    li   t1, 0xDEADBEEF
    sw   t1, 0(t0)          

# Rotina para imprimir um número de até 2 dígitos (0-99)
print_int:
    li   t0, 0x10000004     # Endereço da UART
    li   t1, 10             # Constante 10 para comparação

    blt  a0, t1, print_unit # Se a0 < 10, pula direto para imprimir a unidade

    # --- Lógica para Dezenas ---
    li   t2, 0              # t2 será o nosso contador de dezenas
find_tens:
    addi a0, a0, -10        # Subtrai 10 de a0
    addi t2, t2, 1          # Incrementa a dezena
    bge  a0, t1, find_tens  # Se o resto (a0) ainda for >= 10, continua o loop

    # Imprime a dezena
    addi t2, t2, 48         # Converte a dezena para ASCII
    sw   t2, 0(t0)          # Envia a dezena para a UART

    # --- Lógica para Unidades ---
print_unit:
    addi a0, a0, 48         # Converte a unidade (o que sobrou em a0) para ASCII
    sw   a0, 0(t0)          # Envia a unidade para a UART

    # Imprime um espaço no final
    li   a0, 32             # Espaço (ASCII 32)
    sw   a0, 0(t0)
    ret