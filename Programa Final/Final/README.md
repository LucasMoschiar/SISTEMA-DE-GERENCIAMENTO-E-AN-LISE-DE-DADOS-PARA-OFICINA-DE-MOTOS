# Sistema de Gerenciamento e Análise de Dados para Oficina de Motos

Projeto final da disciplina de **Estrutura de Dados**, desenvolvido em **C++**, com o objetivo de gerenciar e analisar um catálogo de peças de motos utilizando diferentes estruturas de dados.

## Arquivos do projeto

- `SistemaDeGerenciamento.cpp`: código-fonte principal do sistema.
- `SistemaDeGerenciamento.exe`: executável do programa para Windows.
- `dataset_pecas_motos_unificado_refeito.txt`: dataset utilizado pelo sistema.
- `SISTEMA_DE_GERENCIAMENTO_E_ANÁLISE_DE_DADOS_PARA_OFICINA_DE_MOTOS.pdf`: relatório final do trabalho.
- `Resumo de Funcionamento do Sistema.pdf`: resumo/manual rápido de funcionamento do sistema.
- `Registro de Uso de Inteligência Artificial.pdf`: registro do uso auxiliar de IA no desenvolvimento, estudo e revisão conceitual do projeto.
- `README.md`: arquivo de orientação do repositório.

## Como executar

Para executar o sistema corretamente, mantenha o arquivo `dataset_pecas_motos_unificado_refeito.txt` na mesma pasta do executável `SistemaDeGerenciamento.exe`.

Em seguida, execute:

```txt
SistemaDeGerenciamento.exe
```

Ao iniciar, o programa realiza a leitura automática do arquivo TXT e carrega os registros para o sistema.

## Como compilar o código-fonte

Caso seja necessário compilar novamente o programa, utilize um compilador C++.

Exemplo com `g++`:

```bash
g++ SistemaDeGerenciamento.cpp -o SistemaDeGerenciamento
```

No Windows, o executável gerado pode ser:

```txt
SistemaDeGerenciamento.exe
```

## Formato do dataset

O dataset deve estar em arquivo `.txt`, com os campos separados por ponto e vírgula `;`.

Formato esperado:

```txt
codigo_peca;descricao_original;nome_peca;categoria;marca_fabricante;modelo_moto;preco;quantidade_estoque;quantidade_minima;status
```

Cada linha do arquivo representa uma peça do catálogo.

## Funcionamento geral

O programa carrega o dataset TXT, transforma cada linha em uma peça e armazena os registros completos em um `vector<Peca>`. As estruturas de dados funcionam como índices de acesso, guardando principalmente a chave da peça e o índice correspondente no vetor.

Fluxo básico:

```txt
TXT do dataset
      ↓
Leitura dos registros
      ↓
Armazenamento no vector<Peca>
      ↓
Construção das estruturas de dados
      ↓
Menu de operações
```

## Funcionalidades principais

O sistema permite:

- buscar peças por código;
- inserir novas peças;
- remover peças;
- buscar por prefixo do nome da peça;
- verificar código com Bloom Filter;
- filtrar peças por categoria;
- mostrar estatísticas do dataset;
- listar estoque crítico;
- executar benchmarks entre estruturas;
- executar testes com restrições;
- mostrar memória aproximada das estruturas.

## Estruturas de dados utilizadas

O projeto utiliza as seguintes estruturas:

- Lista Encadeada;
- Tabela Hash;
- Hash Otimizada;
- Árvore AVL;
- Trie;
- Bloom Filter.

A Lista Encadeada, a Tabela Hash, a Hash Otimizada e a Árvore AVL são utilizadas principalmente para busca por código. A Trie é utilizada para busca por prefixo do nome da peça, e o Bloom Filter é utilizado como verificação probabilística de existência de código, com confirmação pela Tabela Hash.

## Benchmarks e testes

O sistema possui comparações de desempenho entre estruturas:

- Lista Encadeada x Tabela Hash x Hash Otimizada x Árvore AVL, na busca por código;
- Lista x Trie, na busca por prefixo do nome da peça;
- Bloom Filter x Hash, na verificação de existência de código;
- testes restritivos de memória, processamento, latência, qualidade dos dados, escolha algorítmica e escalabilidade.

## Uso de Inteligência Artificial

O uso auxiliar de Inteligência Artificial está documentado no arquivo:

`Registro de Uso de Inteligência Artificial.pdf`

A IA foi utilizada como apoio no estudo, organização e revisão conceitual do sistema, sem substituir a responsabilidade do aluno sobre o projeto entregue.
