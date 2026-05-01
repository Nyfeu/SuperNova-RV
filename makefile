# ==========================================
# SuperNova-RV - Master Makefile
# ==========================================

# ==========================================
# Variáveis de Diretórios
# ==========================================
HW_DIR      := hw
TB_DIR      := tb
UNIT_DIR    := $(TB_DIR)/unit
INT_DIR     := $(TB_DIR)/integration
INC_DIR     := $(TB_DIR)/include

# Estrutura de Build Organizada
BUILD_DIR   := build
GEN_DIR     := $(BUILD_DIR)/gen
BIN_DIR     := $(BUILD_DIR)/bin
TRACES_DIR  := traces

PKG_FILES   := hw/core/supernova_pkg.sv

# Descoberta de subdiretórios de hardware para o Verilator
HW_SUBDIRS  := $(shell find $(HW_DIR) -type d)
VPATH        = $(HW_SUBDIRS)

# ==========================================
# Ferramentas e Flags
# ==========================================
LINTER      := verible-verilog-lint
VERILATOR   := verilator

# Configuração de busca do Verilator
VERILATOR_SEARCH := $(foreach dir, $(HW_SUBDIRS), -y $(dir))

# Flags base (Mdir e o binário final são definidos dinamicamente nas regras)
VERILATOR_FLAGS := --cc --exe --build -Wall --trace $(VERILATOR_SEARCH) -I$(INC_DIR)

# ==========================================
# Regras de Preparação
# ==========================================
.PHONY: prepare
prepare:
	@mkdir -p $(GEN_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(TRACES_DIR)

# ==========================================
# Regras Dinâmicas de Simulação (Pattern Rules)
# ==========================================

# UNIT TESTS
test-unit-%: $(PKG_FILES) %.sv $(UNIT_DIR)/tb_%.cpp | prepare
	@echo " "
	@echo "==> [Unit] Verilating and building module $*..."
	@export LC_ALL=C MAKEFLAGS="-s"; \
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $* \
	--Mdir $(GEN_DIR)/$* -o ../../bin/V$* \
	$(foreach f,$^,$(abspath $(f)))
	@echo " "
	@echo "==> [Unit] Running simulation: $*..."
	@./$(BIN_DIR)/V$*

# INTEGRATION TESTS
test-int-%: $(PKG_FILES) %.sv $(INT_DIR)/tb_%.cpp | prepare
	@echo " "
	@echo "==> [Integration] Verilating and building module $*..."
	@export LC_ALL=C MAKEFLAGS="-s"; \
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module $* \
	--Mdir $(GEN_DIR)/$* -o ../../bin/V$* \
	$(foreach f,$^,$(abspath $(f)))
	@echo " "
	@echo "==> [Integration] Running simulation: $*..."
	@./$(BIN_DIR)/V$*

# ==========================================
# Targets Administrativos
# ==========================================
.PHONY: lint clean

lint:
	@echo "==> Executando Verible Linter..."
	@find $(HW_DIR) -name '*.sv' -o -name '*.v' | xargs $(LINTER) --rules_config_search --lint_fatal

clean:
	@echo "==> Limpando artefatos de build e logs de simulação..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(TRACES_DIR)
	@echo "Limpeza concluída."