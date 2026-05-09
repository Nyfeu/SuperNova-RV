import os
import re
import yaml

HW_DIR = 'hw'
TB_DIR = 'tb'
OUTPUT_YAML = 'supernova.yml'

def parse_sv_dependencies(filepath):
    """Extrai módulos providos e dependências de um arquivo SystemVerilog."""
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
        # Remove comentários de linha (//) e de bloco (/* */)
        content = re.sub(r'//.*', '', content)
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
        
        # O que este arquivo provê? (module, package, interface)
        provides = re.findall(r'(?i)^\s*(?:module|package|interface)\s+(\w+)', content, re.MULTILINE)
        
        depends = []
        # Dependências explícitas (imports de pacotes)
        depends.extend(re.findall(r'(?i)import\s+(\w+)::', content))
        
        # Dependências implícitas (instanciação de submódulos)
        instantiations = re.findall(r'^\s*([a-zA-Z_]\w*)\s*(?:#\s*\([\s\S]*?\))?\s+[a-zA-Z_]\w*\s*\(', content, re.MULTILINE)
        
        # Filtra palavras-chave do SV
        sv_keywords = {'if', 'else', 'for', 'while', 'case', 'always', 'always_ff', 'always_comb', 'assign', 'logic', 'wire'}
        depends.extend([inst for inst in instantiations if inst not in sv_keywords])

    return list(set(provides)), list(set(depends))

def scan_hw():
    """Varre o diretório HW respeitando o isolamento do 'core'."""
    db = {}
    for root, _, files in os.walk(HW_DIR):
        for file in files:
            if file.endswith(('.sv', '.v')):
                filepath = os.path.join(root, file).replace('\\', '/')
                parts = filepath.split('/')
                
                # Mapeamento estrito: hw/core/nebula -> arch = nebula
                if len(parts) > 2 and parts[1] == 'core':
                    arch_identifier = parts[2]
                else:
                    arch_identifier = 'common'
                
                provides, depends = parse_sv_dependencies(filepath)
                for p in provides:
                    db[(arch_identifier, p)] = {'file': filepath, 'depends': depends}
    return db

def resolve_deps(arch, module, db, resolved=None):
    """Busca recursivamente todos os arquivos SystemVerilog que um módulo precisa."""
    if resolved is None:
        resolved = []
    
    # Prioriza a arquitetura específica, se não achar, busca nos comuns
    mod_info = db.get((arch, module)) or db.get(('common', module))
    
    if not mod_info:
        return resolved
        
    filepath = mod_info['file']
    if filepath not in resolved:
        resolved.append(filepath)
        for dep in mod_info['depends']:
            resolve_deps(arch, dep, db, resolved)
            
    return resolved

def main():
    db = scan_hw()
    manifest = {'targets': {'core': {}}}
    
    # Varre os testbenches em C++
    for root, _, files in os.walk(TB_DIR):
        for file in files:
            if file.startswith('tb_') and file.endswith('.cpp'):
                filepath = os.path.join(root, file).replace('\\', '/')
                parts = filepath.split('/')
                
                # Exemplo: tb/core/nebula/unit/tb_alu.cpp
                if len(parts) > 3 and parts[1] == 'core':
                    arch = parts[2]
                    
                    # Nome do alvo (ex: tb_alu.cpp -> alu)
                    target_name = file.replace('tb_', '').replace('.cpp', '')
                    
                    # Resolve os .sv recursivamente
                    sv_files = resolve_deps(arch, target_name, db)
                    
                    # Adiciona pacote global (caso não tenha sido instanciado explicitamente)
                    pkg_file = f"{HW_DIR}/core/{arch}/supernova_pkg.sv"
                    if os.path.exists(pkg_file) and pkg_file not in sv_files:
                        sv_files.insert(0, pkg_file) # Garante que compila antes
                    
                    # Adiciona testbenches e headers
                    cpp_files = [filepath]
                    include_dir = f"{TB_DIR}/core/{arch}/include"
                    if os.path.exists(include_dir):
                        for h_file in os.listdir(include_dir):
                            if h_file.endswith(('.hpp', '.h')):
                                cpp_files.append(f"{include_dir}/{h_file}")

                    target_data = sv_files + cpp_files
                    
                    if arch not in manifest['targets']['core']:
                        manifest['targets']['core'][arch] = {}
                        
                    manifest['targets']['core'][arch][target_name] = target_data
                    
    # Salva o arquivo YAML
    with open(OUTPUT_YAML, 'w', encoding='utf-8') as f:
        yaml.dump(manifest, f, default_flow_style=False, sort_keys=False)
        
    print(f"✅ Sucesso! Manifesto gerado e salvo em '{OUTPUT_YAML}'.")

if __name__ == '__main__':
    main()