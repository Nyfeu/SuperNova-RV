# Ambiente de Desenvolvimento

Para projetar o **SuperNova-RV**, foi tomada a decisão de não depender da instalação local de pacotes no sistema operacional do desenvolvedor. Ferramentas de EDA (*Electronic Design Automation*) e toolchains cruzadas (*cross-compilers*) costumam ser difíceis de configurar e podem variar entre distribuições Linux, Windows e macOS.

Para resolver isso, utilizamos um **DevContainer**.

## 1. O que é o DevContainer?

O **DevContainer** (*Development Container*) é um ambiente **Docker** configurado especificamente para atuar como um ambiente de desenvolvimento completo e integrado ao *Visual Studio Code*.

Ao abrir o projeto no *VS Code* e aceitar reabrir no container, o Docker baixa uma imagem *Ubuntu* (distro *Linux*), instala todas as dependências do hardware, mapeia o seu código-fonte para dentro do container e injeta as extensões do *VS Code* necessárias.

## 2. Ferramentas Pré-instaladas

Nossa imagem base (`.devcontainer/Dockerfile`) instala automaticamente:

- **Build Tools**: `make`, `gcc`, `build-essential`.

- **SystemVerilog & Linting**: `verible` (baixado dinamicamente na versão mais recente para garantir regras modernas de formatação e *linting*).

- **Simulação**: `verilator` (para converter SystemVerilog em modelos C++ de alta performance).

- **Toolchain RISC-V**: `gcc-riscv64-unknown-elf` (para compilar nossos futuros programas em C/Assembly para rodar no nosso processador).

- **Python 3 & Venv**: Para rodar scripts auxiliares (como o nosso testador inteligente) e o MkDocs.

Além disso, o arquivo `devcontainer.json` configura extensões do VS Code para SystemVerilog, C/C++ e suporte a sintaxe Assembly RISC-V.

## 3. Pipeline Local (Git Hooks)

Para evitar que código quebrado chegue ao repositório ou ao **GitHub Actions**, foram configurados dois **Git Hooks** principais na pasta `.githooks/` (que são ativados automaticamente na criação do DevContainer):

1. `pre-commit` (Smart Testing & Linting)

Toda vez que você tenta fazer um commit, este hook entra em ação:

- **Linting**: Roda `make lint`. O Verible verifica se o código SystemVerilog segue as regras definidas em `.rules.verible_lint` (ex: uso de `always_comb`, nomenclatura em `snake_case`, sufixos `_i` e `_o`).

- **Smart Test**: Roda `python3 scripts/smart_test.py`. Este script olha para o `git diff`. Se você alterou, por exemplo, o arquivo `hw/core/pc_reg.sv`, ele descobrirá isso, encontrará o testbench `tb/unit/tb_pc_reg.cpp` correspondente, compilará via Verilator e executará a simulação. Se o teste falhar, o commit é bloqueado.

2. `commit-msg` (Conventional Commits)

Garante que o histórico do Git seja legível e automatizável. O hook exige que sua mensagem de commit siga o formato:
```text
<tipo>(<escopo opcional>): <descrição>
```

Exemplos Válidos:

- `feat: adiciona decodificador de instrucoes`

- `fix(alu): corrige sinal de overflow`

- `docs: atualiza documentacao do ambiente`

## 4. Build System (Makefile)

Nosso `makefile` na raiz do projeto é o maestro de todas as operações. Ele utiliza Pattern Rules (regras dinâmicas) do GNU Make.

Você não precisa escrever uma regra para cada módulo novo. Se você criar um módulo `hw/core/alu.sv` e um testbench `tb/unit/tb_alu.cpp`, você pode simplesmente rodar:

```bash
make test-unit-alu
```

O Makefile fará o match do padrão `test-unit-%`, criará o diretório `build/`, chamará o Verilator com as flags corretas (geração de trace VCD ativada) e executará o binário final resultante.

## 5. Documentação (MkDocs)

Este site que você está lendo é gerado pelo MkDocs com o tema Material. O DevContainer já instala todos os pacotes Python necessários descritos no `requirements.txt`.

Para visualizar edições em tempo real na documentação enquanto escreve:

```bash
mkdocs serve -a 0.0.0.0:8000
```

O parâmetro `-a 0.0.0.0:8000` é importante para que o servidor MkDocs dentro do container Docker fique exposto e acessível para o navegador no seu sistema operacional hospedeiro (em `localhost:8000`).