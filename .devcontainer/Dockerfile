# Usa a imagem LTS do Ubuntu
FROM ubuntu:24.04

# Evita prompts interativos durante a instalação
ENV DEBIAN_FRONTEND=noninteractive

# Instalação das dependências fundamentais
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    python3 \
    python3-pip \
    python3-venv \
    device-tree-compiler \
    verilator \
    gcc-riscv64-unknown-elf \
    wget \
    curl \
    make \
    unzip \
    universal-ctags \
    && rm -rf /var/lib/apt/lists/*

# Instala o Verible (Linter e Formatador) buscando a versão mais recente automaticamente
RUN wget $(curl -s https://api.github.com/repos/chipsalliance/verible/releases/latest | grep browser_download_url | grep linux-static-x86_64.tar.gz | cut -d '"' -f 4) -O verible.tar.gz && \
    tar -xzf verible.tar.gz && \
    mv verible-*/bin/* /usr/local/bin/ && \
    rm -rf verible*

# Configura um prompt (PS1) nativo, colorido e universal no bashrc
RUN echo "export PS1='\[\033[01;32m\]\u@SuperNova-RV\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '" >> ~/.bashrc

# Diretório de trabalho
WORKDIR /workspace