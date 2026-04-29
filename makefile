# ==========================================
# SuperNova-RV - Master Makefile
# ==========================================

# Variáveis de Diretórios
HW_DIR   := hw
TB_DIR   := tb
SW_DIR   := sw
DOCS_DIR := docs
BUILD_DIR:= build

# Ferramentas
LINTER   := verible-verilog-lint
SIMULATOR:= verilator

# ==========================================
# Alvos (Targets) Principais
# ==========================================

.PHONY: all lint clean help

help:
	@echo "Comandos disponíveis no SuperNova-RV:"
	@echo "  make lint    - Roda o Verible para checar o estilo do SystemVerilog"
	@echo "  make sim     - Compila o design com o Verilator (Em breve)"
	@echo "  make docs    - Sobe o servidor local do MkDocs (Em breve)"
	@echo "  make clean   - Remove todos os arquivos gerados no build"

lint:
	@echo "==> Executando Verible Linter no diretório de Hardware..."
	@find $(HW_DIR) -name '*.sv' -o -name '*.v' | xargs $(LINTER) --rules_config_search --lint_fatal

clean:
	@echo "==> Limpando o diretório de build..."
	@rm -rf $(BUILD_DIR)/*
	@echo "Limpeza concluída."