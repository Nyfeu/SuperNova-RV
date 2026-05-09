#!/usr/bin/env python3
import yaml
import sys
import json

def main():
    # Uso: python3 query_yaml.py <arch> <target> <comando_opcional>
    # Ex: python3 query_yaml.py nebula alu
    if len(sys.argv) < 3:
        print("Uso incorreto. Esperado: <arch> <target>", file=sys.stderr)
        return
    
    arch, target = sys.argv[1], sys.argv[2]
    
    try:
        with open("supernova.yml", 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
            
        targets_dict = config.get("targets", {})
        
        # Acessa os dados específicos do core e da arquitetura
        target_data = targets_dict.get("core", {}).get(arch, {}).get(target, [])
        
        if target_data:
            # Retorna como uma string JSON formatada (útil para GitHub Actions Matrix ou logs)
            print(json.dumps(target_data))
            
    except Exception as e:
        print(f"Erro ao consultar YAML: {e}", file=sys.stderr)

if __name__ == "__main__":
    main()