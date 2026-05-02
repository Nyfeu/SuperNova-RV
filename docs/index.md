# SuperNova-RV

**SuperNova-RV** é um projeto de design e implementação de um processador customizado baseado na arquitetura **RISC-V** (RV32I). O objetivo é construir um núcleo (core) robusto, explorando desde conceitos fundamentais de design digital até técnicas avançadas de microarquitetura, sempre guiado por práticas rigorosas de verificação e engenharia de software.

## 1. Status Atual: Milestone Nebula

Atualmente, o projeto atingiu a sua primeira fase estável, denominada **Nebula**. Nesta etapa, o processador opera como um núcleo **single-cycle funcional**, servindo como a fundação lógica para todas as expansões futuras.  

- **Compatibilidade**: 100% aprovado no Compliance Test Suite oficial da RISC-V International para a extensão RV32I.

- **Pipeline**: Single-cycle (1 instrução por ciclo de clock).

- **Verificação**: Sistema de End-to-End Testing integrado com Verilator e assinaturas de referência estáticas.

## 2. Roadmap Cósmico

A evolução do SuperNova-RV segue uma analogia cósmica, onde cada estágio representa uma transição de paradigma microarquitetural:

|Fase|Milestone|Status|Descrição|
|:-:|:-|:-:|:-|
|☁️|**Nebula**|✅ Finalizado|Núcleo RV32I single-cycle funcional e 100% validado.|
|🌟|**Protostar**|🏗️ Em Breve|Implementação de Pipeline básico (5 estágios) com tratamento de hazards.|
|⭐|**Main Sequence**|⏳ Planejado|Núcleo estável e utilizável com suporte a CSRs e interrupções.|
|🌀|**Binary Star**|⏳ Planejado|Microarquitetura Superscalar In-Order.|
|💥|**SuperNova**|⏳ Planejado|Exploração de técnicas avançadas (Out-of-Order+).|

## 3. Stack Tecnológica

O projeto utiliza um ecossistema moderno de ferramentas de EDA (*Electronic Design Automation*) para garantir um ambiente reprodutível e livre de conflitos de dependências:

- **HDL**: SystemVerilog.  

- **Simulação & Verificação**: Verilator (C++ Testbenches).  

- **Linting**: Verible.  

- **Build System**: GNU Make (com regras dinâmicas e cores customizadas).

- **CI/CD**: GitHub Actions com integração de Linter e Compliance E2E.

- **Ambiente**: Docker via VS Code DevContainers.

