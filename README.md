# 🌌 SuperNova-RV

**SuperNova-RV** é um projeto pessoal de design e implementação de um processador customizado, baseado na arquitetura RISC-V (RV32I). O foco do projeto é a construção de um núcleo (*core*) robusto e eficiente, explorando desde conceitos fundamentais de design digital até técnicas avançadas de microarquitetura de CPUs.

## 🚀 Sobre o Projeto

Este repositório documenta a minha jornada para levar meus conhecimentos em arquitetura de computadores do nível básico ao avançado. O objetivo não é apenas criar um processador que funcione, mas aplicar boas práticas de engenharia de hardware e software durante todo o ciclo de vida do projeto.

## 🔭 Roadmap 

A evolução deste projeto segue uma analogia cósmica: assim como estrelas passam por fases bem definidas de formação, estabilidade e colapso, este processador evolui por estágios incrementais de complexidade microarquitetural.

Cada **milestone** representa não apenas um estado funcional, mas uma **transição de paradigma arquitetural**.


- [x] ☁️ **Nebula** — Núcleo RV32I *single-cycle* funcional;  
- [ ] 🌟 **Protostar** — Pipeline básico (5 estágios) com tratamento de hazards;  
- [ ] ⭐ **Main Sequence** — Núcleo estável e utilizável;
- [ ] 🔴 **Red Giant** — Expansão arquitetural; 
- [ ] 🌀 **Binary Star** — Microarquitetura superscalar *in-order*;
- [ ] 🌌 **White Dwarf** — Consolidação em hardware;
- [ ] 💥 **SuperNova** — Exploração avançada.  

## ⚙️ Eixos Transversais

Os seguintes subsistemas evoluem ao longo de todas as fases:

- Arquitetura privilegiada (CSRs, modos, interrupções)
- Verificação (testes, co-simulação)
- Sistema de memória (latência, cache, políticas)
- Unidades funcionais (latências e paralelismo)
- Controle de fluxo (branch handling)
- Métricas de desempenho (CPI, IPC)

## 🛠️ Stack Tecnológica

A base deste projeto foi construída para ser moderna, reprodutível e livre de dores de cabeça com dependências locais:

- Hardware Description Language (HDL): **SystemVerilog**

- Simulação & Verificação: **Verilator** (C++ Testbenches)

- Linting & Formatação: **Verible**

- Build System: **GNU Make** (com targets dinâmicos e testes inteligentes)

- CI/CD & Qualidade: **GitHub Actions**, **Git Hooks** (**Conventional Commits**)

- Ambiente de Desenvolvimento: **Docker** / **VS Code DevContainers**

- Documentação: **MkDocs** (Material Theme)

## 🏁 Como Começar (Quickstart)

Para garantir que todos os compiladores, linters e simuladores funcionem perfeitamente, este projeto utiliza **DevContainers**.

1. Clone o repositório:
```bash
git clone https://github.com/Nyfeu/SuperNova-RV.git
cd supernova-rv
```

2. Abra o projeto no VS Code.
```
code .
```

3. Uma notificação aparecerá no canto inferior direito: "*Folder contains a Dev Container configuration file*". Clique em ***Reopen in Container***.
    - **Nota**: é necessário ter o Docker e a extensão "Dev Containers" instalados.

4. Aguarde a construção da imagem (apenas na primeira vez). O ambiente instalará tudo automaticamente.

## 📜 Fluxo de Contribuição e Qualidade

O repositório possui verificações automáticas estritas para manter o código limpo:

- **Linting RTL**: O código **SystemVerilog** é validado via **verible-verilog-lint**.

- **Smart Tests**: Um script em Python (`scripts/smart_test.py`) detecta quais os arquivos `.sv` que foram alterados e compila/roda apenas os testbenches C++ correspondentes antes de cada commit.

- **Conventional Commits**: O projeto exige mensagens de commit padronizadas (ex: `feat: add ALU module`, `fix(core): resolve pipeline stall`). O hook `commit-msg` fará essa validação localmente.

## 📚 Documentação

A documentação detalhada da arquitetura, módulos e decisões de design está disponível no MkDocs.
Para visualizar localmente dentro do DevContainer:

```bash
mkdocs serve -a 0.0.0.0:8000
```

Acesse o URL: http://localhost:8000 no seu navegador.

---