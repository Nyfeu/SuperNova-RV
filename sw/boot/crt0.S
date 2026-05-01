.section .text.init
.global _start

_start:
    # 1. Configurar o Stack Pointer (opcional para testes simples em Assembly)
    # la sp, _end + 0x1000 
    
    # 2. Saltar para a função principal (main em C ou sua label de teste)
    jal x0, main