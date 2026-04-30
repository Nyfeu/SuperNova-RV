import subprocess
import sys
import os

def get_changed_files():
    """Obtém a lista de arquivos alterados, adaptando-se ao ambiente (Local vs CI)."""
    
    # Verifica se estamos rodando dentro do GitHub Actions
    is_ci = os.environ.get('GITHUB_ACTIONS') == 'true'
    
    if is_ci:
        # No CI, comparamos o commit atual (HEAD) com o anterior (HEAD~1)
        cmd = ["git", "diff", "--name-only", "HEAD~1", "HEAD"]
    else:
        # Localmente, continuamos olhando o que está no 'stage' (git add)
        cmd = ["git", "diff", "--cached", "--name-only"]
        
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.stdout.strip().split('\n')

def map_rtl_to_testbench(changed_files):
    """Mapeia arquivos de hardware (hw/X.sv) para alvos do makefile (test-X)."""
    targets_to_run = set()
    
    for file in changed_files:
        if file.startswith('hw/') and file.endswith('.sv'):
            # Extrai o nome do módulo (ex: de 'hw/dummy.sv' tira 'dummy')
            base_name = os.path.basename(file).replace('.sv', '')
            
            # Verifica se o testbench físico existe (ex: tb/unit/tb_dummy.cpp)
            # Se você tiver testes de integração, a lógica pode expandir aqui
            if os.path.exists(f"tb/unit/tb_{base_name}.cpp"):
                targets_to_run.add(f"test-unit-{base_name}")
            
            if os.path.exists(f"tb/integration/tb_{base_name}.cpp"):
                targets_to_run.add(f"test-int-{base_name}")
                
        # Se mexer em um arquivo de interface ou globais, roda TODOS os testes de integração
        elif file.startswith('hw/interfaces/'):
            targets_to_run.add("test-integration-all")
            
    return list(targets_to_run)

if __name__ == "__main__":
    print("==> Analisando modificações do Git...")
    changed = get_changed_files()
    
    targets = map_rtl_to_testbench(changed)
    
    if not targets:
        print("==> Nenhuma alteração de RTL com testbench mapeado detectada. Pulando testes.")
        sys.exit(0)
        
    print(f"==> Alvos identificados para teste: {', '.join(targets)}")
    
    # Executa os testes mapeados
    for target in targets:
        print(f"\n--- Executando {target} ---")
        # Roda o comando make para o target específico
        result = subprocess.run(["make", target])
        
        # Se um teste falhar, quebra o CI imediatamente
        if result.returncode != 0:
            print(f"❌ Falha no teste: {target}")
            sys.exit(1)
            
    print("✅ Todos os testes condicionais passaram!")