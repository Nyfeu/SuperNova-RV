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

# Cores para o Terminal

GREEN  := \033[0;32m
RED    := \033[0;31m
YELLOW := \033[0;33m
NC     := \033[0m

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
# RISC-V Software Toolchain
# ==========================================
RISCV_PREFIX := riscv64-unknown-elf-
RISCV_GCC    := $(RISCV_PREFIX)gcc
RISCV_LD     := $(RISCV_PREFIX)ld
RISCV_OBJCOPY:= $(RISCV_PREFIX)objcopy

# Flags para RV32I (Note que usamos o prefixo para forçar 32-bits)
SW_FLAGS     := -march=rv32i -mabi=ilp32 -nostdlib -T sw/boot/link.ld

# --- Regras para APPS (Softwares Gerais) ---
sw-compile-app-%: prepare
	@echo "==> [Software] Compiling APP $*..."
	@$(RISCV_GCC) $(SW_FLAGS) sw/boot/crt0.S sw/apps/$*.S -o $(BIN_DIR)/$*.elf
	@$(RISCV_OBJCOPY) -O verilog --verilog-data-width 4 $(BIN_DIR)/$*.elf $(BIN_DIR)/$*.hex

test-app-%: $(PKG_FILES) top_level.sv tb/e2e/tb_e2e_rv32i.cpp | sw-compile-app-% prepare
	@echo "==> [E2E] Verilating and running APP..."
	@export LC_ALL=C MAKEFLAGS="-s"; \
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module top_level \
	--Mdir $(GEN_DIR)/top_level_e2e -o ../../bin/Vtop_level_e2e \
	$(foreach f,$^,$(abspath $(f)))
	@./$(BIN_DIR)/Vtop_level_e2e $(BIN_DIR)/$*.hex --app 2>&1 | tee $(TRACES_DIR)/$*.log
	@grep -q "SUCCESS" $(TRACES_DIR)/$*.log || exit 1

# --- Regras para COMPLIANCE OFICIAL ---
SW_COMPLIANCE_FLAGS := -march=rv32i -mabi=ilp32 -nostdlib -DXLEN=32 -DTEST_CASE_1=1 -T sw/compliance/target/compliance.ld

sw-compile-compliance-%: prepare
	@$(RISCV_GCC) $(SW_COMPLIANCE_FLAGS) \
		-I sw/compliance/env \
		-I sw/compliance/target \
		sw/compliance/rv32i_m/I/src/$*.S \
		-o $(BIN_DIR)/$*.elf
	@$(RISCV_OBJCOPY) -O verilog --verilog-data-width 4 $(BIN_DIR)/$*.elf $(BIN_DIR)/$*.hex

test-compliance-%: $(PKG_FILES) top_level.sv tb/e2e/tb_e2e_rv32i.cpp | sw-compile-compliance-% prepare
	@printf "  %-40s" "TESTING $*..."
	
	@# 1. Compilação do Verilator (silenciosa, log em arquivo)
	@export LC_ALL=C MAKEFLAGS="-s"; \
	$(VERILATOR) $(VERILATOR_FLAGS) -GBOOT_ADDR=32\'h80000000 --top-module top_level \
	--Mdir $(GEN_DIR)/top_level_e2e -o ../../bin/Vtop_level_e2e \
	$(foreach f,$^,$(abspath $(f))) > $(TRACES_DIR)/$*.build_verilator.log 2>&1
	
	@# 2. Descobre os endereços de assinatura
	$(eval SIG_START=$(shell $(RISCV_PREFIX)nm $(BIN_DIR)/$*.elf | grep begin_signature | awk '{print $$1}'))
	$(eval SIG_END=$(shell $(RISCV_PREFIX)nm $(BIN_DIR)/$*.elf | grep end_signature | awk '{print $$1}'))
	
	@# 3. Roda a simulação (silenciosa, log em arquivo)
	@./$(BIN_DIR)/Vtop_level_e2e $(BIN_DIR)/$*.hex --compliance \
	--sig-start $(SIG_START) --sig-end $(SIG_END) --sig-file $(TRACES_DIR)/$*.signature \
	> $(TRACES_DIR)/$*.run.log 2>&1
	
	@# 4. Verifica se falhou no meio (Assert/Timeout)
	@grep -q "SUCCESS" $(TRACES_DIR)/$*.run.log || \
		(printf "[ $(RED)FAIL$(NC) ] (Verilator Error)\n" && cat $(TRACES_DIR)/$*.run.log && exit 1)
		
	@# 5. Compara com a Assinatura Estática
	@diff -q $(TRACES_DIR)/$*.signature sw/compliance/references/$*.reference_output > /dev/null \
		&& printf "[ $(GREEN)PASS$(NC) ]\n" \
		|| (printf "[ $(RED)FAIL$(NC) ] (Signature Mismatch)\n" && diff -y $(TRACES_DIR)/$*.signature sw/compliance/references/$*.reference_output | head -n 20 && exit 1)

# Captura todos os arquivos .S na pasta de testes do RISC-V e extrai apenas o nome do teste
COMPLIANCE_SRCS := $(wildcard sw/compliance/rv32i_m/I/src/*.S)
COMPLIANCE_TESTS := $(patsubst sw/compliance/rv32i_m/I/src/%.S, %, $(COMPLIANCE_SRCS))

# Cria uma regra que depende de todos os test-compliance-[nome]
.PHONY: test-compliance-all
test-compliance-all: $(addprefix test-compliance-, $(COMPLIANCE_TESTS))
	@printf "\n"
	@printf "$(GREEN)========================================================$(NC)\n"
	@printf "$(GREEN)           SUPERNOVA-RV COMPLIANCE TEST SUITE           $(NC)\n"
	@printf "$(GREEN)========================================================$(NC)\n"
	@printf "  TARGET ARCHITECTURE : $(YELLOW)RV32I Base Integer$(NC)\n"
	@printf "  VERIFICATION STATUS : $(GREEN)100%% PASSED$(NC)\n"
	@printf "  TOTAL TESTS RUN     : $(YELLOW)$(words $(COMPLIANCE_TESTS)) / $(words $(COMPLIANCE_TESTS))$(NC)\n"
	@printf "$(GREEN)========================================================$(NC)\n\n"

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