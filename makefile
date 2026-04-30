# ==========================================
# SuperNova-RV - Master Makefile
# ==========================================

# ==========================================
# Variáveis de Diretórios
# ==========================================
HW_DIR     := hw
TB_DIR     := tb
UNIT_DIR   := $(TB_DIR)/unit
INT_DIR    := $(TB_DIR)/integration
INC_DIR    := $(TB_DIR)/include
BUILD_DIR  := build

# ==========================================
# Ferramentas e Flags
# ==========================================
LINTER     := verible-verilog-lint
VERILATOR  := verilator

# Adicionamos a flag --Mdir apontando para o BUILD_DIR
VERILATOR_FLAGS := --cc --exe --build -Wall --trace -y $(HW_DIR) -I$(HW_DIR) -I$(INC_DIR) --Mdir $(BUILD_DIR)

# ==========================================
# Regras Dinâmicas de Simulação (Pattern Rules)
# ==========================================

test-unit-%: $(HW_DIR)/%.sv $(UNIT_DIR)/tb_%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "==> [Unidade] Compilando módulo $* com Verilator..."
	@export LC_ALL=C MAKEFLAGS="-s"; $(VERILATOR) $(VERILATOR_FLAGS) $^
	@echo "==> [Unidade] Executando simulação de $*..."
	@./$(BUILD_DIR)/V$*

test-int-%: $(HW_DIR)/%.sv $(INT_DIR)/tb_%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "==> [Integração] Compilando módulo top-level $*..."
	@export LC_ALL=C MAKEFLAGS="-s"; $(VERILATOR) $(VERILATOR_FLAGS) $^
	@echo "==> [Integração] Executando simulação de $*..."
	@./$(BUILD_DIR)/V$*

# ==========================================
# Targets Administrativos
# ==========================================
.PHONY: lint clean

lint:
	@echo "==> Executando Verible Linter..."
	@find $(HW_DIR) -name '*.sv' -o -name '*.v' | xargs $(LINTER) --rules_config_search --lint_fatal

clean:
	@echo "==> Limpando diretório de build..."
	@rm -rf $(BUILD_DIR)
	@echo "Limpeza concluída."