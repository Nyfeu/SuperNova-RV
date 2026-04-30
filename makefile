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
OBJ_DIR    := obj_dir

# ==========================================
# Ferramentas e Flags
# ==========================================
LINTER     := verible-verilog-lint
VERILATOR  := verilator

# O '-y $(HW_DIR)' e '-I$(HW_DIR)' são a mágica da dependência. 
# Se a ULA instanciar um Somador, o Verilator procura automaticamente somador.sv na pasta hw/
VERILATOR_FLAGS := --cc --exe --build -Wall --trace -y $(HW_DIR) -I$(HW_DIR) -I$(INC_DIR)

# ==========================================
# Regras Dinâmicas de Simulação (Pattern Rules)
# ==========================================

# Regra para Testes de UNIDADE (ex: make test-unit-dummy)
# O símbolo '%' funciona como um coringa (wildcard).
test-unit-%: $(HW_DIR)/%.sv $(UNIT_DIR)/tb_%.cpp
	@echo "==> [Unidade] Compilando módulo $* com Verilator..."
	@export LC_ALL=C MAKEFLAGS="-s"; $(VERILATOR) $(VERILATOR_FLAGS) $^
	@echo "==> [Unidade] Executando simulação de $*..."
	@./$(OBJ_DIR)/V$*

# Regra para Testes de INTEGRAÇÃO (ex: make test-int-datapath)
test-int-%: $(HW_DIR)/%.sv $(INT_DIR)/tb_%.cpp
	@echo "==> [Integração] Compilando módulo top-level $*..."
	@export LC_ALL=C MAKEFLAGS="-s"; $(VERILATOR) $(VERILATOR_FLAGS) $^
	@echo "==> [Integração] Executando simulação de $*..."
	@./$(OBJ_DIR)/V$*

# ==========================================
# Targets Administrativos
# ==========================================
.PHONY: lint clean

lint:
	@echo "==> Executando Verible Linter..."
	@find $(HW_DIR) -name '*.sv' -o -name '*.v' | xargs $(LINTER) --rules_config_search --lint_fatal

clean:
	@echo "==> Limpando artefatos..."
	@rm -rf $(BUILD_DIR)/* $(OBJ_DIR) dump.vcd
	@echo "Limpeza concluída."