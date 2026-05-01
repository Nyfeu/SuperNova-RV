#!/usr/bin/env python3
import subprocess
import sys
import os
import yaml

CONFIG_FILE = "supernova.yml"

def get_changed_files():
    """Obtém a lista de arquivos alterados, adaptando-se ao ambiente (Local vs CI)."""
    
    is_ci = os.environ.get('GITHUB_ACTIONS') == 'true'
    
    if is_ci:
        cmd = ["git", "diff", "--name-only", "HEAD~1", "HEAD"]
    else:
        cmd = ["git", "diff", "--cached", "--name-only"]
        
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    # Adicione esta verificação de segurança:
    if result.returncode != 0:
        print(f"❌ Erro ao executar git diff: {result.stderr}", file=sys.stderr)
        sys.exit(1) # Interrompe o script se o Git falhar
        
    return [f for f in result.stdout.strip().split('\n') if f]

def main():
    print("==> Analisando modificações do Git...")
    changed_files = get_changed_files()
    
    if not changed_files:
        print("==> Nenhuma alteração detectada. Pulando testes.")
        return 0

    # 1. Carrega a matriz de dependências do arquivo YAML
    try:
        with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
    except FileNotFoundError:
        print(f"❌ Erro fatal: O arquivo '{CONFIG_FILE}' não foi encontrado na raiz do projeto.")
        return 1
    except yaml.YAMLError as exc:
        print(f"❌ Erro de sintaxe no YAML: {exc}")
        return 1

    targets_dict = config.get("targets", {})
    targets_to_run = set()

    # 2. Cruza os arquivos alterados com a matriz do YAML
    for changed_file in changed_files:
        for target_name, dependencies in targets_dict.items():
            if changed_file in dependencies:
                targets_to_run.add(target_name)

    if not targets_to_run:
        print("==> As alterações não afetam nenhum módulo mapeado no YAML. Pulando testes.")
        return 0
        
    print(f"==> Alvos identificados para teste: {', '.join(targets_to_run)}")
    
    # 3. Executa os testes mapeados
    for target in targets_to_run:
        # Assumimos que o YAML guarda o nome base (ex: 'alu'). Montamos o comando do make.
        cmd = f"make test-unit-{target}"
        print(f"\n--- Executando {cmd} ---")
        
        # subprocess.run com shell=True permite executar o comando em formato de string
        result = subprocess.run(cmd, shell=True)
        
        # Se um teste falhar, quebra o CI imediatamente
        if result.returncode != 0:
            print(f"\n❌ Falha no teste: {target}")
            return 1
            
    print("\n✅ Todos os testes condicionais passaram!")
    return 0

if __name__ == "__main__":
    sys.exit(main())