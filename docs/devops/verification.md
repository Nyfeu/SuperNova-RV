# Estratégia de Verificação e Compliance

O desenvolvimento da **SuperNova-RV** segue uma hierarquia de verificação em três níveis para garantir a correção do hardware: Testes Unitários, Testes de Integração e Testes de *Compliance* (End-to-End).

## 1. Níveis de Verificação

### Testes Unitários e de Integração
Escritos em C++ usando a biblioteca nativa do Verilator. Eles injetam estímulos diretamente nos sinais dos módulos individuais (ex: ALU, Branch Unit) e comparam com saídas esperadas. O script `smart_test.py` mapeia o `git diff` e executa automaticamente apenas os testbenches dos módulos modificados.

### Testes de Compliance E2E (RV32I Base Integer)
Para garantir que a SuperNova-RV é um núcleo RISC-V autêntico, utilizamos o *Compliance Test Suite* oficial da RISC-V International. A verificação E2E avalia todo o *Datapath* e *Controlpath* de forma integrada.

## 2. A Abordagem de Compliance

Optamos por uma arquitetura de testes "congelada" (*vendored*) em vez de gerar os testes dinamicamente a cada simulação. Isso maximiza a velocidade do CI/CD e garante reprodutibilidade.

O fluxo de verificação E2E funciona assim:

1. **Compilação Cruzada:** O GNU GCC (`gcc-riscv64-unknown-elf`) compila cada teste Assembly `.S` nativo do diretório `sw/compliance/rv32i_m/` usando um *linker script* customizado (`compliance.ld`). O resultado é extraído em um binário `.hex`.
2. **Verilating do Top-Level:** O Verilator converte o módulo `top_level.sv` em um executável C++ e injeta o firmware compilado na memória da CPU.
3. **Execução em Hardware:** A simulação avança *clock* por *clock* até que o programa atinja a diretiva de *HALT* ou acione um *Timeout*.
4. **Extração da Assinatura:** A macro oficial do teste grava o resultado das operações em posições de memória específicas (marcadas por `begin_signature` e `end_signature`). Após a execução, o *Testbench* extrai esses dados e cria um arquivo `.signature`.
5. **Comparação Estática:** O arquivo de assinatura extraído do nosso RTL é comparado bit a bit (usando `diff`) contra a *Golden Reference* estática do Spike. Um *Mismatch* reprova o teste imediatamente.

## 3. Pipeline de CI/CD (GitHub Actions)

Para evitar regressões, o repositório conta com um fluxo de Integração Contínua (CI) automatizado no GitHub Actions, acionado em *Pushes* e *Pull Requests* para a branch `main`.

A topologia do CI é baseada em "Fail-Fast":

* **Job 1 (Linting):** Instala o `verible` via cache, verifica as regras `.rules.verible_lint` e a nomenclatura do código.
* **Job 2 (Simulation):** Executado dentro de um container Docker do Verilator. Baixa o ambiente Python e executa o `smart_test.py` para validar módulos unitários e de integração.
* **Job 3 (Compliance Verification):** Se os *Jobs* anteriores passarem e houver alterações no hardware/firmware, o container do Verilator instala o compilador RISC-V via `apt` e executa o `make test-compliance-all`, varrendo todo o portfólio de testes contra as referências estáticas e disponibilizando logs (`traces/`) em caso de falha.