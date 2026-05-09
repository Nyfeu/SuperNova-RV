#!/usr/bin/env python3
import subprocess
import sys
import os
import yaml

CONFIG_FILE = "supernova.yml"

def get_changed_files():
    """Obtém a lista de arquivos alterados, adaptando-se ao ambiente (Local vs CI vs PR)."""
    is_ci = os.environ.get('GITHUB_ACTIONS') == 'true'
    event_name = os.environ.get('GITHUB_EVENT_NAME')
    base_ref = os.environ.get('GITHUB_BASE_REF')
    
    if is_ci:
        if event_name == 'pull_request' and base_ref:
            cmd = ["git", "diff", "--name-only", f"origin/{base_ref}", "HEAD"]
            print(f"==> Modo PR detectado: Comparando origin/{base_ref} com HEAD")
        else:
            cmd = ["git", "diff", "--name-only", "HEAD~1", "HEAD"]
            print("==> Modo Push detectado: Comparando HEAD~1 com HEAD")
    else:
        cmd = ["git", "diff", "--cached", "--name-only"]
        print("==> Modo Local detectado: Analisando o stage do Git")
        
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"❌ Erro ao executar git diff: {result.stderr}", file=sys.stderr)
        sys.exit(1)
        
    return [f for f in result.stdout.strip().split('\n') if f]

def main():
    print("==> Analisando modificações do Git...")
    changed_files = get_changed_files()
    
    if not changed_files:
        print("==> Nenhuma alteração detectada. Pulando testes.")
        return 0

    try:
        with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
    except FileNotFoundError:
        print(f"❌ Erro fatal: O arquivo '{CONFIG_FILE}' não foi encontrado.")
        return 1
    except yaml.YAMLError as exc:
        print(f"❌ Erro de sintaxe no YAML: {exc}")
        return 1

    targets_dict = config.get("targets", {})
    core_targets = targets_dict.get("core", {})
    
    # Armazena tuplas com a arquitetura e o nome do alvo: ex -> ('nebula', 'alu')
    targets_to_run = set()

    for changed_file in changed_files:
        # Navega na hierarquia: arch (nebula/protostar) -> target (alu/lsu) -> deps
        for arch, arch_modules in core_targets.items():
            for target_name, dependencies in arch_modules.items():
                if changed_file in dependencies:
                    targets_to_run.add((arch, target_name))

    if not targets_to_run:
        print("==> As alterações não afetam nenhum módulo mapeado no YAML. Pulando testes.")
        return 0
        
    print(f"==> Alvos identificados para teste: {len(targets_to_run)}")
    
    # Execução isolada por arquitetura
    for arch, target in targets_to_run:
        dependencies = core_targets[arch][target]
        
        # Lógica de roteamento do Makefile baseada no domínio do teste
        if any(f"tb/core/{arch}/e2e/" in f for f in dependencies) or "e2e" in target:
            # Smoke Test: Se afetou o E2E ou o Top Level, roda o app de validação geral
            cmd = f"make test-app-fibonacci CORE_ARCH={arch}"
        elif any(f"tb/core/{arch}/integration/" in f for f in dependencies):
            cmd = f"make test-int-{target} CORE_ARCH={arch}"
        else:
            cmd = f"make test-unit-{target} CORE_ARCH={arch}"
        
        print(f"\n--- 🚀 Executando: {cmd} ---")
        result = subprocess.run(cmd, shell=True)
        
        if result.returncode != 0:
            print(f"\n❌ Falha crítica no teste do módulo '{target}' ({arch}).")
            return 1
            
    print("\n✅ Todos os testes condicionais passaram!")
    return 0

if __name__ == "__main__":
    sys.exit(main())