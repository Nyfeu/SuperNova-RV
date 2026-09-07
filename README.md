<div align="center">

# 🌌 SuperNova-RV

**Um processador RISC-V (RV32I) escrito do zero em SystemVerilog — do núcleo *single-cycle* ao pipeline de 5 estágios — com verificação de *compliance* oficial e CI de hardware.**

[![CI Pipeline](https://github.com/Nyfeu/SuperNova-RV/actions/workflows/ci.yml/badge.svg)](https://github.com/Nyfeu/SuperNova-RV/actions/workflows/ci.yml)
![ISA](https://img.shields.io/badge/ISA-RV32I-blue)
![HDL](https://img.shields.io/badge/HDL-SystemVerilog-orange)
![Simulator](https://img.shields.io/badge/Simulator-Verilator-green)

</div>

---

## 📑 Sumário

- [Sobre o Projeto](#-sobre-o-projeto)
- [Status Atual](#-status-atual)
- [Roadmap](#-roadmap)
- [Microarquitetura](#️-microarquitetura)
- [Estrutura do Repositório](#-estrutura-do-repositório)
- [Quickstart](#-quickstart)
- [Comandos de Build e Simulação](#-comandos-de-build-e-simulação)
- [Estratégia de Verificação](#-estratégia-de-verificação)
- [CI/CD e Qualidade](#-cicd-e-qualidade)
- [Stack Tecnológica](#️-stack-tecnológica)
- [Documentação](#-documentação)

---

## 🚀 Sobre o Projeto

**SuperNova-RV** é um projeto pessoal de design e implementação de um processador customizado baseado na
arquitetura aberta **RISC-V**, com foco no conjunto base de inteiros de 32 bits (**RV32I**).

O objetivo vai além de "fazer funcionar": cada milestone é uma **transição de paradigma microarquitetural**,
implementada sob práticas rigorosas de engenharia — código lintado, testado por níveis (unitário → integração →
end-to-end), validado contra o *Compliance Test Suite* oficial da RISC-V International e integrado a um pipeline
de CI/CD automatizado.

Uma característica central do repositório é que **as microarquiteturas coexistem**: cada milestone vive em sua
própria "bolha" (`hw/core/<arch>/` + `tb/core/<arch>/`), permitindo comparar implementações lado a lado e
executar a mesma suíte de verificação contra qualquer uma delas via `CORE_ARCH`.

## 📍 Status Atual

| Item | Estado |
| :--- | :--- |
| Microarquiteturas ativas | `nebula` (single-cycle) e `protostar` (pipeline de 5 estágios) |
| ISA suportada | RV32I Base Integer (32 registradores, endereçamento a byte) |
| Compliance oficial | **39/39 testes aprovados** em ambos os cores |
| Verificação | Até 9 testbenches unitários e 8 de integração por core, mais o fluxo E2E |
| Topologia de memória | Harvard (IMEM e DMEM externas ao core) |
| Core padrão do build | `protostar` (`make ... CORE_ARCH=nebula` para alternar) |

## 🔭 Roadmap

A evolução deste projeto segue uma analogia cósmica: assim como estrelas passam por fases bem definidas de
formação, estabilidade e colapso, este processador evolui por estágios incrementais de complexidade.

| Fase | Milestone | Status | Descrição |
| :-: | :--- | :-: | :--- |
| ☁️ | **Nebula** | ✅ Concluído | Núcleo RV32I *single-cycle* funcional e 100% validado. |
| 🌟 | **Protostar** | ✅ Concluído | Pipeline de 5 estágios com *forwarding*, detecção de *load-use hazard* e *flush* de desvios. |
| ⭐ | **Main Sequence** | 🔜 Próximo | Núcleo estável e utilizável: arquitetura privilegiada (CSRs), *traps* e interrupções. |
| 🔴 | **Red Giant** | ⏳ Planejado | Expansão arquitetural (extensões da ISA, sistema de memória com cache). |
| 🌀 | **Binary Star** | ⏳ Planejado | Microarquitetura superescalar *in-order*. |
| 🌌 | **White Dwarf** | ⏳ Planejado | Consolidação em hardware (síntese e prototipagem em FPGA). |
| 💥 | **SuperNova** | ⏳ Planejado | Exploração avançada (execução fora de ordem e além). |

### Eixos transversais

Estes subsistemas evoluem continuamente ao longo de **todas** as fases:

- Arquitetura privilegiada (CSRs, modos de execução, interrupções)
- Verificação (testes unitários, *compliance*, co-simulação)
- Sistema de memória (latência, cache, políticas de escrita)
- Unidades funcionais (latências e paralelismo)
- Controle de fluxo (*branch handling* e predição)
- Métricas de desempenho (CPI, IPC)

## 🏗️ Microarquitetura

O processador é exposto ao mundo externo como uma **caixa preta Harvard**: o `top_level` costura o `datapath`
("músculos") ao `controlpath` ("cérebro") e expõe apenas *clock*, *reset* e os barramentos de memória de
instruções (IMEM) e de dados (DMEM). O endereço de boot é parametrizável via `BOOT_ADDR`.

### `nebula` — single-cycle

Todos os estágios lógicos (Fetch → Decode → Execute → Memory → Write-Back) ocorrem no **mesmo tick de clock**.
Desvios são resolvidos combinacionalmente no mesmo ciclo do *fetch*, dispensando predição — CPI = 1, ao custo de
um caminho crítico longo. Serve como referência dourada de comportamento para as microarquiteturas seguintes.

### `protostar` — pipeline de 5 estágios

Os mesmos blocos funcionais, agora separados por quatro registradores de pipeline fortemente tipados
(`if_id_t`, `id_ex_t`, `ex_mem_t`, `mem_wb_t`, definidos em `supernova_pkg.sv`):

Tratamento de hazards implementado:

| Hazard | Unidade responsável | Resolução |
| :--- | :--- | :--- |
| **RAW (dados)** | `forwarding_unit` | *Bypass* dos operandos da ALU a partir de EX/MEM (prioridade 1) ou MEM/WB (prioridade 2), sem penalidade de ciclos. |
| **Load-Use** | `hazard_unit` | Detecta `LOAD` em EX cujo `rd` é consumido pela instrução em ID: congela IF/ID e injeta uma bolha (NOP) em ID/EX. |
| **Controle (desvios)** | `branch_unit` + `datapath` | Desvio resolvido em EX; ao ser tomado, IF/ID e ID/EX são zerados (*flush*) e o PC recebe o alvo (`PC+Imm` ou `(rs1+Imm) & ~1` para `JALR`). |

Todos os sinais de controle usam **tipos enumerados** (`alu_op_e`, `imm_type_e`, `alu_src_a_e`, `alu_src_b_e`,
`mem_size_e`, `wb_src_e`) empacotados em `ctrl_bus_t`, garantindo *type safety* na travessia do pipeline.

## 📁 Estrutura do Repositório

```
SuperNova-RV/
├── hw/core/
│   ├── nebula/            # RTL do core single-cycle (SystemVerilog)
│   └── protostar/         # RTL do core pipelined (+ hazard_unit, forwarding_unit)
├── tb/core/<arch>/
│   ├── unit/              # Testbenches C++ por módulo (ALU, LSU, RegFile, ...)
│   ├── integration/       # Testbenches por estágio e de datapath/controlpath
│   ├── e2e/               # Testbench end-to-end (carrega firmware .hex)
│   └── include/           # Infraestrutura comum de testbench (testbench.hpp)
├── sw/
│   ├── boot/              # crt0.S e linker script
│   ├── apps/              # Programas de exemplo (ex.: fibonacci.S)
│   └── compliance/        # Suíte oficial RISC-V (src, env, target, references)
├── scripts/
│   ├── gera_deps.py       # Gera supernova.yml varrendo instâncias/imports do RTL
│   ├── query_yaml.py      # Consulta as dependências de um target para o Makefile
│   └── smart_test.py      # Executa apenas os testes afetados pelo git diff
├── docs/                  # Documentação MkDocs (hardware + devops)
├── supernova.yml          # Grafo de dependências RTL ↔ testbench (gerado)
└── makefile               # Build system com pattern rules por CORE_ARCH
```

## 🏁 Quickstart

O ambiente é totalmente containerizado — **não é necessário instalar Verilator, Verible ou a toolchain RISC-V
localmente**. Requisitos: Docker + VS Code com a extensão *Dev Containers*.

```bash
git clone https://github.com/Nyfeu/SuperNova-RV.git
cd SuperNova-RV
code .
```

Ao abrir, o VS Code exibirá a notificação *"Folder contains a Dev Container configuration file"* — clique em
**Reopen in Container**. Na primeira execução, a imagem instalará `verilator`, `verible`,
`gcc-riscv64-unknown-elf`, `make`, Python 3 e as extensões necessárias, além de ativar os Git Hooks.

Para validar que tudo está funcionando, rode a suíte oficial de compliance:

```bash
make test-compliance-all
```

## 🔧 Comandos de Build e Simulação

O `makefile` usa *pattern rules*: novos módulos não exigem novas regras. Todos os alvos aceitam
`CORE_ARCH=<nebula|protostar>` (padrão: `protostar`).

| Comando | Descrição |
| :--- | :--- |
| `make test-unit-<módulo>` | Verila e executa o testbench unitário do módulo (ex.: `make test-unit-alu`). |
| `make test-int-<módulo>` | Executa o testbench de integração (ex.: `make test-int-datapath`). |
| `make test-app-<app>` | Compila `sw/apps/<app>.S` e executa no core (ex.: `make test-app-fibonacci`). |
| `make test-compliance-<teste>` | Executa um teste de compliance isolado (ex.: `make test-compliance-add-01`). |
| `make test-compliance-all` | Roda os 39 testes oficiais RV32I e imprime o relatório final. |
| `make lint` | Análise estática com `verible-verilog-lint` sobre o core selecionado. |
| `make clean` | Remove `build/` e `traces/`. |

```bash
# Exemplos
make test-unit-forwarding_unit                  # unitário no core protostar
make test-int-datapath CORE_ARCH=nebula         # integração no core single-cycle
make test-compliance-all CORE_ARCH=nebula       # compliance completo no single-cycle
```

Artefatos gerados ficam isolados por core em `build/<arch>/` e os logs, *waveforms* (VCD) e assinaturas em
`traces/<arch>/`.

## 🔬 Estratégia de Verificação

A verificação é organizada em **três níveis**, todos escritos em C++ sobre a API nativa do Verilator:

1. **Unitário** — estímulos diretos nos sinais de módulos individuais (ALU, `branch_unit`, `imm_gen`, `lsu`,
   `reg_file`, `pc_reg`, `hazard_unit`, `forwarding_unit`, `instr_decoder`).
2. **Integração** — validação por estágio (fetch/decode/execute/memory/writeback) e dos blocos `datapath`,
   `controlpath` e `top_level`.
3. **End-to-End / Compliance** — execução de firmware real no core completo, validado contra o
   *Compliance Test Suite* oficial da RISC-V International.

### Fluxo de compliance (E2E)

Os testes são *vendored* (congelados no repositório) em vez de gerados dinamicamente, o que maximiza a
reprodutibilidade e a velocidade do CI:

1. **Compilação cruzada** — `riscv64-unknown-elf-gcc` compila cada `.S` de `sw/compliance/rv32i_m/I/src/` com o
   linker script `compliance.ld`; `objcopy` extrai um `.hex`.
2. **Verilating** — o `top_level.sv` vira um executável C++ (com `BOOT_ADDR=0x80000000`) e o firmware é
   injetado na memória simulada.
3. **Execução** — a simulação avança ciclo a ciclo até o *HALT* do programa ou um *timeout*.
4. **Extração da assinatura** — o testbench lê a região entre `begin_signature` e `end_signature` e grava um
   arquivo `.signature`.
5. **Comparação estática** — `diff` bit a bit contra a *golden reference* do Spike. Qualquer divergência reprova
   o teste imediatamente e o log é preservado em `traces/`.

## 📜 CI/CD e Qualidade

### Pipeline local (Git Hooks)

Ativados automaticamente na criação do DevContainer, em `.githooks/`:

- **`pre-commit`** — roda `make lint` (Verible, conforme `.rules.verible_lint`: `always_comb`, `snake_case`,
  sufixos `_i`/`_o`) e, em seguida, `scripts/smart_test.py`, que lê o `git diff` *staged*, resolve o grafo de
  dependências em `supernova.yml` e **executa apenas os testbenches afetados**. Teste falhando bloqueia o commit.
- **`commit-msg`** — exige **Conventional Commits** no formato `<tipo>(<escopo>): <descrição>`
  (ex.: `feat(alu): add barrel shifter`, `fix(core): resolve pipeline stall`).

### Pipeline remoto (GitHub Actions)

Topologia *fail-fast* acionada em *pushes* e *pull requests* para `main`:

| Job | Responsabilidade |
| :--- | :--- |
| **0 · Detect Changes** | `paths-filter` identifica quais cores foram tocados e monta dinamicamente a matriz de execução. |
| **1 · Verible Lint** | Instala o Verible (com cache) e valida as regras de escrita do RTL. |
| **2 · Verilator Tests** | Dentro do container `verilator/verilator`, roda `smart_test.py` (unitários + integração). |
| **3 · RV32I Compliance** | Matriz por core: executa `make test-compliance-all` e publica os `traces/` como artefato em caso de falha. |

## 🛠️ Stack Tecnológica

| Camada | Ferramenta |
| :--- | :--- |
| Descrição de hardware | **SystemVerilog** (tipos enumerados e *structs* empacotados) |
| Simulação e verificação | **Verilator** com testbenches em **C++** e traces **VCD** |
| Análise estática | **Verible** (`verible-verilog-lint` / `verible-verilog-format`) |
| Toolchain de software | **GCC RISC-V** (`riscv64-unknown-elf`), linker scripts customizados |
| Build system | **GNU Make** com *pattern rules* e isolamento por `CORE_ARCH` |
| Automação auxiliar | **Python 3** (geração do grafo de dependências e testes incrementais) |
| CI/CD | **GitHub Actions** + **Git Hooks** + **Conventional Commits** |
| Ambiente | **Docker** / **VS Code DevContainers** |
| Documentação | **MkDocs** (Material Theme) |

## 📚 Documentação

A documentação detalha a arquitetura, cada módulo RTL e as decisões de design. Acesse pelo link: [Documentação](https://nyfeu.github.io/SuperNova-RV/)

