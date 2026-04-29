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
    && rm -rf /var/lib/apt/lists/*

# Configura um prompt (PS1) nativo, colorido e universal no bashrc
RUN echo "export PS1='\[\033[01;32m\]\u@SuperNova-RV\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '" >> ~/.bashrc

# Diretório de trabalho
WORKDIR /workspace