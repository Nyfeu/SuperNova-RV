## 🏁 Quickstart

Para começar a explorar ou simular o núcleo, você não precisa instalar ferramentas locais de hardware, apenas o Docker e o VS Code com a extensão de Dev Containers.  

1. Clone o repositório e abra-o no VS Code.  

2. Ao ser solicitado, clique em "Reopen in Container".  

3. Para rodar toda a suíte de verificação de compliance oficial:

```bash
make test-compliance-all
```

## 📜 Qualidade e Contribuição

O repositório impõe verificações automáticas estritas para manter a integridade do hardware:

- **Smart Tests**: Um script Python (`scripts/smart_test.py`) detecta alterações no código e executa apenas os testbenches unitários afetados antes de cada commit.  

- **Conventional Commits**: Mensagens de commit padronizadas são exigidas para manter um histórico legível (ex: `feat: add ALU module`).  

- **Linting RTL**: Validação constante via `verible-verilog-lint` para garantir boas práticas de escrita de hardware.