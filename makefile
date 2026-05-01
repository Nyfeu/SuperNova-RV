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

PKG_FILES  := hw/core/supernova_pkg.sv

HW_SUBDIRS := $(shell find $(HW_DIR) -type d)
VPATH = $(HW_SUBDIRS)

# ==========================================
# Ferramentas e Flags
# ==========================================
LINTER     := verible-verilog-lint
VERILATOR  := verilator

# Adicionamos a flag --Mdir apontando para o BUILD_DIR
VERILATOR_SEARCH := $(foreach dir, $(HW_SUBDIRS), -y $(dir))
VERILATOR_FLAGS := --cc --exe --build -Wall --trace $(VERILATOR_SEARCH) -I$(INC_DIR) --Mdir $(BUILD_DIR)

# ==========================================
# Regras Dinâmicas de Simulação (Pattern Rules)
# ==========================================

# UNIT TESTS: Source must be in tb/unit/tb_*.cpp
test-unit-%: $(PKG_FILES) %.sv $(UNIT_DIR)/tb_%.cpp
	@echo " "
	@mkdir -p $(BUILD_DIR)
	@echo "==> [Unit] Verilating module $*..."
	@export LC_ALL=C MAKEFLAGS="-s"; $(VERILATOR) $(VERILATOR_FLAGS) --top-module $* $^
	@echo " "
	@echo "==> [Unit] Running simulation: $*..."
	@./$(BUILD_DIR)/V$*

# INTEGRATION TESTS: Source must be in tb/integration/tb_*.cpp
# Added --top-module $* to allow testing sub-systems like controlpath
test-int-%: $(PKG_FILES) %.sv $(INT_DIR)/tb_%.cpp
	@echo " "
	@mkdir -p $(BUILD_DIR)
	@echo "==> [Integration] Verilating module $*..."
	@export LC_ALL=C MAKEFLAGS="-s"; $(VERILATOR) $(VERILATOR_FLAGS) --top-module $* $^
	@echo " "
	@echo "==> [Integration] Running simulation: $*..."
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