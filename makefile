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
# RISC-V Software Toolchain
# ==========================================
RISCV_PREFIX := riscv64-unknown-elf-
RISCV_GCC    := $(RISCV_PREFIX)gcc
RISCV_LD     := $(RISCV_PREFIX)ld
RISCV_OBJCOPY:= $(RISCV_PREFIX)objcopy

# Flags para RV32I (Note que usamos o prefixo para forçar 32-bits)
SW_FLAGS     := -march=rv32i -mabi=ilp32 -nostdlib -T sw/boot/link.ld

# --- Regras para Testes ISA (Hardware Base) ---
sw-compile-isa-%: prepare
	@echo "==> [Software] Compiling ISA test $*..."
	@$(RISCV_GCC) $(SW_FLAGS) sw/boot/crt0.S sw/isa/$*.S -o $(BIN_DIR)/$*.elf
	@$(RISCV_OBJCOPY) -O verilog --verilog-data-width 4 $(BIN_DIR)/$*.elf $(BIN_DIR)/$*.hex

test-isa-%: $(PKG_FILES) top_level.sv tb/e2e/tb_e2e_rv32i.cpp | sw-compile-isa-% prepare
	@echo "==> [E2E] Verilating and running ISA test..."
	@export LC_ALL=C MAKEFLAGS="-s"; \
	$(VERILATOR) $(VERILATOR_FLAGS) --top-module top_level \
	--Mdir $(GEN_DIR)/top_level_e2e -o ../../bin/Vtop_level_e2e \
	$(foreach f,$^,$(abspath $(f)))
	@./$(BIN_DIR)/Vtop_level_e2e $(BIN_DIR)/$*.hex --isa --trace 2>&1 | tee $(TRACES_DIR)/$*.log
	@grep -q "SUCCESS" $(TRACES_DIR)/$*.log || exit 1

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
	@echo "==> [Software] Compiling Compliance Test: $*..."
	@$(RISCV_GCC) $(SW_COMPLIANCE_FLAGS) \
		-I sw/compliance/env \
		-I sw/compliance/target \
		sw/compliance/rv32i_m/I/src/$*.S \
		-o $(BIN_DIR)/$*.elf
	@$(RISCV_OBJCOPY) -O verilog --verilog-data-width 4 $(BIN_DIR)/$*.elf $(BIN_DIR)/$*.hex

test-compliance-%: $(PKG_FILES) top_level.sv tb/e2e/tb_e2e_rv32i.cpp | sw-compile-compliance-% prepare
	@echo "==> [E2E] Verilating and running Compliance Test..."
	@export LC_ALL=C MAKEFLAGS="-s"; \
	$(VERILATOR) $(VERILATOR_FLAGS) -GBOOT_ADDR=32\'h80000000 --top-module top_level \
	--Mdir $(GEN_DIR)/top_level_e2e -o ../../bin/Vtop_level_e2e \
	$(foreach f,$^,$(abspath $(f)))
	$(eval SIG_START=$(shell $(RISCV_PREFIX)nm $(BIN_DIR)/$*.elf | grep begin_signature | awk '{print $$1}'))
	$(eval SIG_END=$(shell $(RISCV_PREFIX)nm $(BIN_DIR)/$*.elf | grep end_signature | awk '{print $$1}'))
	@./$(BIN_DIR)/Vtop_level_e2e $(BIN_DIR)/$*.hex --compliance \
	--sig-start $(SIG_START) --sig-end $(SIG_END) --sig-file $(TRACES_DIR)/$*.signature \
	2>&1 | tee $(TRACES_DIR)/$*.log
	@grep -q "SUCCESS" $(TRACES_DIR)/$*.log && echo "✅ Execução no Hardware Finalizada!\n" || exit 1
	@echo "==> [Spike] Gerando Golden Reference Signature..."
	@spike --isa=rv32i +signature=$(TRACES_DIR)/$*.reference_output $(BIN_DIR)/$*.elf
	@echo "==> [Diff] Comparando SuperNova-RV vs Spike...\n"
	@diff -q $(TRACES_DIR)/$*.signature $(TRACES_DIR)/$*.reference_output > /dev/null && echo "✅ Assinatura 100% Compatível!\n" || (echo "❌ ERRO: O Hardware calculou algo diferente do Spike!\n" && exit 1)

# Captura todos os arquivos .S na pasta de testes do RISC-V e extrai apenas o nome do teste
COMPLIANCE_SRCS := $(wildcard sw/compliance/rv32i_m/I/src/*.S)
COMPLIANCE_TESTS := $(patsubst sw/compliance/rv32i_m/I/src/%.S, %, $(COMPLIANCE_SRCS))

# Cria uma regra que depende de todos os test-compliance-[nome]
.PHONY: test-compliance-all
test-compliance-all: $(addprefix test-compliance-, $(COMPLIANCE_TESTS))
	@echo " "
	@echo "🏆================================================🏆"
	@echo "🏆 100% COMPLIANCE RV32I ALCANÇADO COM SUCESSO! 🏆"
	@echo "🏆 CPU oficialmente pronta e validada.          🏆"
	@echo "🏆================================================🏆"

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