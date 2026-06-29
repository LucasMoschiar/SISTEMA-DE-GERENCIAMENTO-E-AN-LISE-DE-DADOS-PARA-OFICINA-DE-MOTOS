
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <map>
#include <functional>
#include <cctype>
#include <limits>

// evita ter que escrever std:: antes dos comandos da biblioteca padrão.
using namespace std;

// ============================================================
// NOME DO ARQUIVO TXT
// ============================================================

/*
    NOME DO ARQUIVO TXT (constante para facilitar em renomear).
   Ex: const string NOME_ARQUIVO = "dataset_pecas_motos_unificado_refeito.txt";

*/

const string NOME_ARQUIVO = "dataset_pecas_motos_unificado_refeito.txt";

// ============================================================
// CONFIGURACOES GERAIS
// ============================================================

const int TAM_HASH_FIXA = 1009;          // Hash simples inicializa menor para evidenciar as colisoes
const int TAM_HASH_OTIMIZADA_INICIAL = 1009; // Hash otimizada inicializa menor para evidenciar os redirecionamentos
const int TAM_BLOOM = 100003; 			
const int LIMITE_RESULTADOS = 50; 		//	Aparecer mais resumido
const int LIMITE_BENCHMARK = 5000;
const int REPETICOES_CONSULTA = 100000;

// ============================================================
// STRUCT DO REGISTRO DE PECA
// ============================================================

struct Peca {
    string codigo_peca;
    string descricao_original;
    string nome_peca;
    string categoria;
    string marca_fabricante;
    string modelo_moto;
    double preco;
    int quantidade_estoque;
    int quantidade_minima;
    string status;
    bool ativo;
};

// ============================================================
// FUNCOES AUXILIARES
// ============================================================

string removerEspacosDasPontas(string texto) {
    // Remove espacos, quebras de linha e tabulacoes do inicio e do fim do texto.
    // Isso evita erro em comparacoes, buscas e leitura no arquivo TXT.
    
    while (!texto.empty() &&
          (texto.back() == '\r' || texto.back() == '\n' || texto.back() == ' ' || texto.back() == '\t')) {
        texto.pop_back();
    }

    while (!texto.empty() && (texto.front() == ' ' || texto.front() == '\t')) {
        texto.erase(texto.begin());
    }

    return texto;
}

string removerMarcadorBOM(string texto) {
    // Alguns arquivos TXT podem iniciar com um marcador invisivel, pode ficar com caracteres estranhos.
    
    if (texto.size() >= 3 &&
        (unsigned char)texto[0] == 0xEF &&
        (unsigned char)texto[1] == 0xBB &&
        (unsigned char)texto[2] == 0xBF) {
        return texto.substr(3);
    }

    return texto;
}

string transformarParaMaiusculo(string texto) {
    // Padroniza textos para maiusculo, assim, "motor", "Motor" e "MOTOR" sao tratados da mesma forma.
    
    for (int i = 0; i < (int)texto.size(); i++) {
        texto[i] = toupper((unsigned char)texto[i]);
    }

    return texto;
}

vector<string> separarCamposDaLinha(string linha, char separador) {
    // Divide uma linha do TXT em campos usando o separador ponto e virgula.
    // Cada campo representa uma informacao da peca.
    
    vector<string> campos;
    string campo;
    stringstream ss(linha);

    while (getline(ss, campo, separador)) {
        campos.push_back(removerEspacosDasPontas(campo));
    }

    return campos;
}

int converterTextoParaInteiro(string texto) {
    // Converte texto para inteiro.
    // Se o valor estiver invalido, retorna 0 para evitar que o programa pare.
    
    texto = removerEspacosDasPontas(texto);

    try {
        return stoi(texto);
    } catch (...) {
        return 0;
    }
}

double converterTextoParaDecimal(string texto) {
    // Converte texto para numero decimal.
    // Tambem troca virgula por ponto para aceitar valores como 22,22 e salve errado.
    
    texto = removerEspacosDasPontas(texto);

    for (int i = 0; i < (int)texto.size(); i++) {
        if (texto[i] == ',') {
            texto[i] = '.';
        }
    }

    try {
        return stod(texto);
    } catch (...) {
        return 0.0;
    }
}

// ============================================================
// LEITURA SEGURA DE NUMEROS DIGITADOS PELO USUARIO
// ============================================================

bool validarConverterTextoParaDecimal(string texto, double& valor) {
    // Valida se o texto digitado realmente pode virar um numero decimal.
    
    texto = removerEspacosDasPontas(texto);

    if (texto.empty()) {
        return false;
    }

    for (int i = 0; i < (int)texto.size(); i++) {
        if (texto[i] == ',') {
            texto[i] = '.';
        }
    }

    try {
        size_t posicao = 0;
        valor = stod(texto, &posicao);

        while (posicao < texto.size()) {
            if (texto[posicao] != ' ' && texto[posicao] != '\t') {
                return false;
            }

            posicao++;
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool validarConverterTextoParaInteiro(string texto, int& valor) {
    // Valida se o texto digitado pelo usuario realmente pode virar um numero inteiro.
    // Usado para estoque e quantidade minima.
    
    texto = removerEspacosDasPontas(texto);

    if (texto.empty()) {
        return false;
    }

    try {
        size_t posicao = 0;
        valor = stoi(texto, &posicao);

        while (posicao < texto.size()) {
            if (texto[posicao] != ' ' && texto[posicao] != '\t') {
                return false;
            }

            posicao++;
        }

        return true;
    } catch (...) {
        return false;
    }
}

double lerNumeroDecimalDoUsuario(string mensagem) {
    // Fica repetindo a pergunta ate o usuario digitar um preco valido.
    // Evita o erro de salvar a peca antes de preencher todos os campos.
    
    string texto;
    double valor = 0.0;

    while (true) {
        cout << mensagem;
        getline(cin, texto);

        if (validarConverterTextoParaDecimal(texto, valor) && valor >= 0) {
            return valor;
        }

        cout << "Valor invalido. Digite um numero valido. Exemplo: 22.22 ou 22,22\n";
    }
}

int lerNumeroInteiroDoUsuario(string mensagem) {
    // Isso protege os campos de estoque contra letras, virgulas ou valores invalidos.
    
    string texto;
    int valor = 0;

    while (true) {
        cout << mensagem;
        getline(cin, texto);

        if (validarConverterTextoParaInteiro(texto, valor) && valor >= 0) {
            return valor;
        }

        cout << "Valor invalido. Digite um numero inteiro maior ou igual a zero.\n";
    }
}

bool textoComecaComPrefixo(string texto, string prefixo) {
    // Verifica se um texto comeca com determinado prefixo.
    // Usado para comparar a busca por prefixo feita pela lista e com a Trie.
    
    texto = transformarParaMaiusculo(texto);
    prefixo = transformarParaMaiusculo(prefixo);

    if (prefixo.size() > texto.size()) {
        return false;
    }

    for (int i = 0; i < (int)prefixo.size(); i++) {
        if (texto[i] != prefixo[i]) {
            return false;
        }
    }

    return true;
}

double medirTempoDaOperacao(function<void()> acao) {
    // Recebe uma operacao qualquer, executa ela e mede quanto tempo demorou.
    // Essa funcao e reutilizada em insercao, remocao e testes.
    
    auto inicio = chrono::high_resolution_clock::now();
    acao();
    auto fim = chrono::high_resolution_clock::now();

    chrono::duration<double> tempo = fim - inicio;
    return tempo.count();
}

void imprimirTempoEmSegundosEMicrossegundos(string nome, double segundos) {
    cout << fixed << setprecision(10);
    cout << "\n" << nome << ": " << segundos << " segundos";
    cout << fixed << setprecision(3);
    cout << " | " << segundos * 1000000.0 << " microssegundos";
}

void pausarEntrada() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void aguardarEnterParaMenu() {
    cout << "\n\nPressione ENTER para voltar ao menu...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

// ============================================================
// STATUS DERIVADO DO ESTOQUE
// ============================================================

string calcularStatusDoEstoque(int quantidade_estoque, int quantidade_minima) {
    // Calcula o status da peca com base no estoque real.
    // O status nao depende apenas do texto vindo do arquivo.
    
    if (quantidade_estoque == 0) {
        return "SEM ESTOQUE";
    }

    if (quantidade_estoque <= quantidade_minima) {
        return "ESTOQUE BAIXO";
    }

    return "DISPONIVEL";
}

// ============================================================
// MOSTRAR PECA
// ============================================================

void exibirPecaCompleta(const Peca& p) {
    cout << "\n==========================================";
    cout << "\nCodigo da peca: " << p.codigo_peca;
    cout << "\nDescricao original: " << p.descricao_original;
    cout << "\nNome da peca: " << p.nome_peca;
    cout << "\nCategoria: " << p.categoria;
    cout << "\nMarca/Fabricante: " << p.marca_fabricante;
    cout << "\nModelo da moto: " << p.modelo_moto;
    cout << "\nPreco: R$ " << fixed << setprecision(2) << p.preco;
    cout << "\nQuantidade em estoque: " << p.quantidade_estoque;
    cout << "\nQuantidade minima: " << p.quantidade_minima;
    cout << "\nStatus calculado: " << p.status;
    cout << "\nAtivo: " << (p.ativo ? "SIM" : "NAO");
    cout << "\n==========================================\n";
}

// usada em listagens, para não poluir a tela.
void exibirPecaResumida(const Peca& p) {
    cout << "\nCodigo: " << p.codigo_peca;
    cout << " | Nome: " << p.nome_peca;
    cout << " | Categoria: " << p.categoria;
    cout << " | Modelo: " << p.modelo_moto;
    cout << " | Estoque: " << p.quantidade_estoque;
    cout << " | Minimo: " << p.quantidade_minima;
    cout << " | Status: " << p.status;
}

// ============================================================
// LEITURA DO DATASET
// ============================================================

// Le o arquivo TXT, transforma cada linha em uma Peca e coloca tudo em um vector.
vector<Peca> carregarPecasDoArquivoTXT(string nomeArquivo) {
    // Abre o arquivo TXT, le linha por linha e transforma cada linha em uma struct Peca.
    // Retorna um vector com todas as pecas carregadas.
    
    vector<Peca> dados;
    ifstream arquivo(nomeArquivo.c_str());

    if (!arquivo.is_open()) {
        cout << "Erro ao abrir o arquivo: " << nomeArquivo << endl;
        cout << "Verifique se o TXT esta na mesma pasta do codigo ou se o caminho esta correto." << endl;
        return dados;
    }

    string linha;

    // Ignora cabecalho
    getline(arquivo, linha);

    while (getline(arquivo, linha)) {
        linha = removerMarcadorBOM(linha);

        if (linha.empty()) {
            continue;
        }

        vector<string> campos = separarCamposDaLinha(linha, ';');
        
        if (campos.size() < 9) {
            continue;
        }

        Peca p;

        p.codigo_peca = campos[0];
        p.descricao_original = campos[1];
        p.nome_peca = campos[2];
        p.categoria = campos[3];
        p.marca_fabricante = campos[4];
        p.modelo_moto = campos[5];
        p.preco = converterTextoParaDecimal(campos[6]);
        p.quantidade_estoque = converterTextoParaInteiro(campos[7]);
        p.quantidade_minima = converterTextoParaInteiro(campos[8]);

        p.status = calcularStatusDoEstoque(p.quantidade_estoque, p.quantidade_minima);

        p.ativo = true;

        if (!p.codigo_peca.empty()) {
            dados.push_back(p);
        }
    }

    arquivo.close();
    return dados;
}

// ============================================================
// SALVAR DATASET NO TXT
// ============================================================

// Regrava o TXT com todas as pecas ativas. Isso garante persistencia apos inserir/remover.
bool salvarPecasAtivasNoArquivoTXT(string nomeArquivo, vector<Peca>& dados) {
    // Isso faz com que insercoes e remocoes continuem salvas apos fechar o programa.
    
    ofstream arquivo(nomeArquivo.c_str());

    if (!arquivo.is_open()) {
        cout << "\nErro ao salvar o arquivo: " << nomeArquivo;
        cout << "\nVerifique se o arquivo nao esta aberto no Excel/Bloco de Notas e se o caminho esta correto.\n";
        return false;
    }

    arquivo << "codigo_peca;descricao_original;nome_peca;categoria;marca_fabricante;modelo_moto;preco;quantidade_estoque;quantidade_minima;status\n";

    for (int i = 0; i < (int)dados.size(); i++) {
        if (dados[i].ativo) {
            dados[i].status = calcularStatusDoEstoque(dados[i].quantidade_estoque, dados[i].quantidade_minima);

            arquivo << dados[i].codigo_peca << ";";
            arquivo << dados[i].descricao_original << ";";
            arquivo << dados[i].nome_peca << ";";
            arquivo << dados[i].categoria << ";";
            arquivo << dados[i].marca_fabricante << ";";
            arquivo << dados[i].modelo_moto << ";";
            arquivo << fixed << setprecision(2) << dados[i].preco << ";";
            arquivo << dados[i].quantidade_estoque << ";";
            arquivo << dados[i].quantidade_minima << ";";
            arquivo << dados[i].status << "\n";
        }
    }

    arquivo.close();
    return true;
}

// ============================================================
// LISTA ENCADEADA
// ============================================================

struct NoLista {
    string chave;
    int indice;
    NoLista* prox;
};

// Estrutura principal 1: Lista Encadeada.
class ListaEncadeada {
private:
    NoLista* inicio;
    int quantidade;

public:
    ListaEncadeada() {
        inicio = NULL;
        quantidade = 0;
    }

    ~ListaEncadeada() {
        limpar();
    }

    void limpar() {
        NoLista* atual = inicio;

        while (atual != NULL) {
            NoLista* prox = atual->prox;
            delete atual;
            atual = prox;
        }

        inicio = NULL;
        quantidade = 0;
    }

// Insere a chave no inicio da lista. O indice aponta para a peca no vector principal (posição).
    void inserir(string chave, int indice) {
        NoLista* novo = new NoLista;
        novo->chave = chave;
        novo->indice = indice;
        novo->prox = inicio;
        inicio = novo;
        quantidade++;
    }

// Percorre a lista nó a nó ate encontrar a chave procurada.
    int buscar(string chave) {
        NoLista* atual = inicio;

        while (atual != NULL) {
            if (atual->chave == chave) {
                return atual->indice;
            }

            atual = atual->prox;
        }

        return -1;
    }

// Remove o nó da lista que possui a chave informada.
    bool remover(string chave) {
        NoLista* atual = inicio;
        NoLista* anterior = NULL;

        while (atual != NULL) {
            if (atual->chave == chave) {
                if (anterior == NULL) {
                    inicio = atual->prox;
                } else {
                    anterior->prox = atual->prox;
                }

                delete atual;
                quantidade--;
                return true;
            }

            anterior = atual;
            atual = atual->prox;
        }

        return false;
    }

    int getQuantidade() {
        return quantidade;
    }

    size_t memoriaAproximada() {
        return sizeof(ListaEncadeada) + ((size_t)quantidade * sizeof(NoLista));
    }
};

// ============================================================
// TABELA HASH COM ENCADEAMENTO
// ============================================================

struct NoHash {
    string chave;
    int indice;
    NoHash* prox;
};

// Estrutura principal 2: Tabela Hash fixa com encadeamento para tratar colisoes.
class TabelaHash {
private:
    vector<NoHash*> tabela;
    int tamanho;
    int colisoes;
    int quantidade;

// Calcula a posicao da chave dentro da tabela usando uma funcao hash.
    int funcaoHash(string chave) {
        unsigned long hash = 5381;

        for (int i = 0; i < (int)chave.size(); i++) {
            hash = ((hash << 5) + hash) + chave[i];
        }

        return hash % tamanho;
    }

public:
    TabelaHash(int tam = TAM_HASH_FIXA) {
        tamanho = tam;
        colisoes = 0;
        quantidade = 0;
        tabela.resize(tamanho, NULL);
    }

    ~TabelaHash() {
        limpar();
    }

    void limpar() {
        for (int i = 0; i < tamanho; i++) {
            NoHash* atual = tabela[i];

            while (atual != NULL) {
                NoHash* prox = atual->prox;
                delete atual;
                atual = prox;
            }

            tabela[i] = NULL;
        }

        colisoes = 0;
        quantidade = 0;
    }

    void inserir(string chave, int indice) {
        int pos = funcaoHash(chave);

        if (tabela[pos] != NULL) {
            colisoes++;
        }

        NoHash* novo = new NoHash;
        novo->chave = chave;
        novo->indice = indice;
        novo->prox = tabela[pos];
        tabela[pos] = novo;
        quantidade++;
    }

    int buscar(string chave) {
        int pos = funcaoHash(chave);
        NoHash* atual = tabela[pos];

        while (atual != NULL) {
            if (atual->chave == chave) {
                return atual->indice;
            }

            atual = atual->prox;
        }

        return -1;
    }

    bool remover(string chave) {
        int pos = funcaoHash(chave);
        NoHash* atual = tabela[pos];
        NoHash* anterior = NULL;

        while (atual != NULL) {
            if (atual->chave == chave) {
                if (anterior == NULL) {
                    tabela[pos] = atual->prox;
                } else {
                    anterior->prox = atual->prox;
                }

                delete atual;
                quantidade--;
                return true;
            }

            anterior = atual;
            atual = atual->prox;
        }

        return false;
    }

    int getColisoes() {
        return colisoes;
    }

    int getQuantidade() {
        return quantidade;
    }

    int getTamanho() {
        return tamanho;
    }

    double getFatorCarga() {
        if (tamanho == 0) {
            return 0.0;
        }

        return (double)quantidade / tamanho;
    }

    size_t memoriaAproximada() {
        return ((size_t)tamanho * sizeof(NoHash*)) + ((size_t)quantidade * sizeof(NoHash));
    }
};

// ============================================================
// HASH OTIMIZADA COM REHASH
// ============================================================

// Estrutura otimizada: Hash com redimensionamento automatico quando o fator de carga aumenta (0,75).
class HashOtimizada {
private:
    vector<NoHash*> tabela;
    int tamanho;
    int colisoes;
    int quantidade;
    int rehashes;

    int funcaoHash(string chave) {
        unsigned long hash = 5381;

        for (int i = 0; i < (int)chave.size(); i++) {
            hash = ((hash << 5) + hash) + chave[i];
        }

        return hash % tamanho;
    }

    void inserirSemRehash(string chave, int indice) {
        int pos = funcaoHash(chave);

        if (tabela[pos] != NULL) {
            colisoes++;
        }

        NoHash* novo = new NoHash;
        novo->chave = chave;
        novo->indice = indice;
        novo->prox = tabela[pos];
        tabela[pos] = novo;

        quantidade++;
    }

// Aumenta a tabela e reinsere as chaves quando a Hash Otimizada fica muito cheia.
    void redimensionar() {
        vector<NoHash*> antiga = tabela;
        int tamanhoAntigo = tamanho;

        tamanho = tamanho * 2 + 1;
        tabela.clear();
        tabela.resize(tamanho, NULL);

        quantidade = 0;
        colisoes = 0;
        rehashes++;

        for (int i = 0; i < tamanhoAntigo; i++) {
            NoHash* atual = antiga[i];

            while (atual != NULL) {
                inserirSemRehash(atual->chave, atual->indice);

                NoHash* temp = atual;
                atual = atual->prox;
                delete temp;
            }
        }
    }

public:
    HashOtimizada(int tam = TAM_HASH_OTIMIZADA_INICIAL) {
        tamanho = tam;
        colisoes = 0;
        quantidade = 0;
        rehashes = 0;
        tabela.resize(tamanho, NULL);
    }

    ~HashOtimizada() {
        limpar();
    }

    void limpar() {
        for (int i = 0; i < tamanho; i++) {
            NoHash* atual = tabela[i];

            while (atual != NULL) {
                NoHash* prox = atual->prox;
                delete atual;
                atual = prox;
            }

            tabela[i] = NULL;
        }

        colisoes = 0;
        quantidade = 0;
    }

    void inserir(string chave, int indice) {
        double fatorCarga = (double)(quantidade + 1) / tamanho;

        if (fatorCarga > 0.75) {
            redimensionar();
        }

        inserirSemRehash(chave, indice);
    }

    int buscar(string chave) {
        int pos = funcaoHash(chave);
        NoHash* atual = tabela[pos];

        while (atual != NULL) {
            if (atual->chave == chave) {
                return atual->indice;
            }

            atual = atual->prox;
        }

        return -1;
    }

    bool remover(string chave) {
        int pos = funcaoHash(chave);
        NoHash* atual = tabela[pos];
        NoHash* anterior = NULL;

        while (atual != NULL) {
            if (atual->chave == chave) {
                if (anterior == NULL) {
                    tabela[pos] = atual->prox;
                } else {
                    anterior->prox = atual->prox;
                }

                delete atual;
                quantidade--;
                return true;
            }

            anterior = atual;
            atual = atual->prox;
        }

        return false;
    }

    int getColisoes() {
        return colisoes;
    }

    int getQuantidade() {
        return quantidade;
    }

    int getTamanho() {
        return tamanho;
    }

    int getRehashes() {
        return rehashes;
    }

    double getFatorCarga() {
        if (tamanho == 0) {
            return 0.0;
        }

        return (double)quantidade / tamanho;
    }

    size_t memoriaAproximada() {
        return ((size_t)tamanho * sizeof(NoHash*)) + ((size_t)quantidade * sizeof(NoHash));
    }
};

// ============================================================
// ARVORE AVL
// ============================================================

struct NoAVL {
    string chave;
    int indice;
    int altura;
    NoAVL* esquerda;
    NoAVL* direita;
};

int altura(NoAVL* no) {
    if (no == NULL) {
        return 0;
    }

    return no->altura;
}

int maximo(int a, int b) {
    return (a > b) ? a : b;
}

NoAVL* criarNoAVL(string chave, int indice) {
    NoAVL* novo = new NoAVL;
    novo->chave = chave;
    novo->indice = indice;
    novo->altura = 1;
    novo->esquerda = NULL;
    novo->direita = NULL;
    return novo;
}

// Rotacao usada para rebalancear a AVL quando o lado esquerdo fica mais cheio.
NoAVL* rotacaoDireita(NoAVL* y) {
    NoAVL* x = y->esquerda;
    NoAVL* t2 = x->direita;

    x->direita = y;
    y->esquerda = t2;

    y->altura = maximo(altura(y->esquerda), altura(y->direita)) + 1;
    x->altura = maximo(altura(x->esquerda), altura(x->direita)) + 1;

    return x;
}

// Rotacao da AVL quando o lado direito fica mais cheio.
NoAVL* rotacaoEsquerda(NoAVL* x) {
    NoAVL* y = x->direita;
    NoAVL* t2 = y->esquerda;

    y->esquerda = x;
    x->direita = t2;

    x->altura = maximo(altura(x->esquerda), altura(x->direita)) + 1;
    y->altura = maximo(altura(y->esquerda), altura(y->direita)) + 1;

    return y;
}

int fatorBalanceamento(NoAVL* no) {
    if (no == NULL) {
        return 0;
    }

    return altura(no->esquerda) - altura(no->direita);
}

// Insere uma chave na AVL e rebalanceia a arvore.
NoAVL* inserirAVL(NoAVL* raiz, string chave, int indice) {
    if (raiz == NULL) {
        return criarNoAVL(chave, indice);
    }

    if (chave < raiz->chave) {
        raiz->esquerda = inserirAVL(raiz->esquerda, chave, indice);
    } else if (chave > raiz->chave) {
        raiz->direita = inserirAVL(raiz->direita, chave, indice);
    } else {
        return raiz;
    }

    raiz->altura = 1 + maximo(altura(raiz->esquerda), altura(raiz->direita));

    int balanceamento = fatorBalanceamento(raiz);

    if (balanceamento > 1 && chave < raiz->esquerda->chave) {
        return rotacaoDireita(raiz);
    }

    if (balanceamento < -1 && chave > raiz->direita->chave) {
        return rotacaoEsquerda(raiz);
    }

    if (balanceamento > 1 && chave > raiz->esquerda->chave) {
        raiz->esquerda = rotacaoEsquerda(raiz->esquerda);
        return rotacaoDireita(raiz);
    }

    if (balanceamento < -1 && chave < raiz->direita->chave) {
        raiz->direita = rotacaoDireita(raiz->direita);
        return rotacaoEsquerda(raiz);
    }

    return raiz;
}

int buscarAVL(NoAVL* raiz, string chave) {
    if (raiz == NULL) {
        return -1;
    }

    if (chave == raiz->chave) {
        return raiz->indice;
    }

    if (chave < raiz->chave) {
        return buscarAVL(raiz->esquerda, chave);
    }

    return buscarAVL(raiz->direita, chave);
}

NoAVL* menorValorAVL(NoAVL* no) {
    NoAVL* atual = no;

    while (atual->esquerda != NULL) {
        atual = atual->esquerda;
    }

    return atual;
}

// Remove uma chave da AVL e rebalanceia a arvore depois da remocao
NoAVL* removerAVL(NoAVL* raiz, string chave) {
    if (raiz == NULL) {
        return raiz;
    }

    if (chave < raiz->chave) {
        raiz->esquerda = removerAVL(raiz->esquerda, chave);
    } else if (chave > raiz->chave) {
        raiz->direita = removerAVL(raiz->direita, chave);
    } else {
        if (raiz->esquerda == NULL || raiz->direita == NULL) {
            NoAVL* temp = raiz->esquerda ? raiz->esquerda : raiz->direita;

            if (temp == NULL) {
                temp = raiz;
                raiz = NULL;
            } else {
                *raiz = *temp;
            }

            delete temp;
        } else {
            NoAVL* temp = menorValorAVL(raiz->direita);
            raiz->chave = temp->chave;
            raiz->indice = temp->indice;
            raiz->direita = removerAVL(raiz->direita, temp->chave);
        }
    }

    if (raiz == NULL) {
        return raiz;
    }

    raiz->altura = 1 + maximo(altura(raiz->esquerda), altura(raiz->direita));

    int balanceamento = fatorBalanceamento(raiz);

    if (balanceamento > 1 && fatorBalanceamento(raiz->esquerda) >= 0) {
        return rotacaoDireita(raiz);
    }

    if (balanceamento > 1 && fatorBalanceamento(raiz->esquerda) < 0) {
        raiz->esquerda = rotacaoEsquerda(raiz->esquerda);
        return rotacaoDireita(raiz);
    }

    if (balanceamento < -1 && fatorBalanceamento(raiz->direita) <= 0) {
        return rotacaoEsquerda(raiz);
    }

    if (balanceamento < -1 && fatorBalanceamento(raiz->direita) > 0) {
        raiz->direita = rotacaoDireita(raiz->direita);
        return rotacaoEsquerda(raiz);
    }

    return raiz;
}

int contarNosAVL(NoAVL* raiz) {
    if (raiz == NULL) {
        return 0;
    }

    return 1 + contarNosAVL(raiz->esquerda) + contarNosAVL(raiz->direita);
}

void liberarAVL(NoAVL* raiz) {
    if (raiz == NULL) {
        return;
    }

    liberarAVL(raiz->esquerda);
    liberarAVL(raiz->direita);
    delete raiz;
}

size_t memoriaAproximadaAVL(NoAVL* raiz) {
    return (size_t)contarNosAVL(raiz) * sizeof(NoAVL);
}

// ============================================================
// TRIE PARA PREFIXO DO NOME DA PECA - ESTRUTURA AUXILIAR
// ===========================================================

struct NoTrie {
    map<char, NoTrie*> filhos;
    int quantidadePrefixo;
    bool fim;

    NoTrie() {
        quantidadePrefixo = 0;
        fim = false;
    }
};

// Estrutura auxiliar: Trie para contar rapidamente nomes por prefixo.
class Trie {
private:
    NoTrie* raiz;

    void limparNo(NoTrie* no) {
        if (no == NULL) {
            return;
        }

        for (map<char, NoTrie*>::iterator it = no->filhos.begin(); it != no->filhos.end(); it++) {
            limparNo(it->second);
        }

        delete no;
    }

    int contarNos(NoTrie* no) {
        if (no == NULL) {
            return 0;
        }

        int total = 1;

        for (map<char, NoTrie*>::iterator it = no->filhos.begin(); it != no->filhos.end(); it++) {
            total += contarNos(it->second);
        }

        return total;
    }

public:
    Trie() {
        raiz = new NoTrie();
    }

    ~Trie() {
        limparNo(raiz);
    }

    void limpar() {
        limparNo(raiz);
        raiz = new NoTrie();
    }

    void inserir(string palavra) {
        palavra = transformarParaMaiusculo(palavra);

        NoTrie* atual = raiz;

        for (int i = 0; i < (int)palavra.size(); i++) {
            char c = palavra[i];

            if (atual->filhos[c] == NULL) {
                atual->filhos[c] = new NoTrie();
            }

            atual = atual->filhos[c];
            atual->quantidadePrefixo++;
        }

        atual->fim = true;
    }

    int contarPrefixo(string prefixo) {
        prefixo = transformarParaMaiusculo(prefixo);

        NoTrie* atual = raiz;

        for (int i = 0; i < (int)prefixo.size(); i++) {
            char c = prefixo[i];

            if (atual->filhos[c] == NULL) {
                return 0;
            }

            atual = atual->filhos[c];
        }

        return atual->quantidadePrefixo;
    }

    int getQuantidadeNos() {
        return contarNos(raiz);
    }

    size_t memoriaAproximada() {
        return (size_t)getQuantidadeNos() * sizeof(NoTrie);
    }
};

// ===========================================================
// BLOOM FILTER - ESTRUTURA AUXILIAR
// ============================================================

// Estrutura auxiliar: Bloom Filter para teste probabilistico de existencia
class BloomFilter {
private:
    vector<bool> bits;
    int tamanho;
    int inseridos;

    int hash1(string chave) {
        unsigned long hash = 5381;

        for (int i = 0; i < (int)chave.size(); i++) {
            hash = ((hash << 5) + hash) + chave[i];
        }

        return hash % tamanho;
    }

    int hash2(string chave) {
        unsigned long hash = 0;

        for (int i = 0; i < (int)chave.size(); i++) {
            hash = chave[i] + (hash << 6) + (hash << 16) - hash;
        }

        return hash % tamanho;
    }

    int hash3(string chave) {
        unsigned long hash = 2166136261u;

        for (int i = 0; i < (int)chave.size(); i++) {
            hash ^= chave[i];
            hash *= 16777619;
        }

        return hash % tamanho;
    }

public:
    BloomFilter(int tam = TAM_BLOOM) {
        tamanho = tam;
        inseridos = 0;
        bits.resize(tamanho, false);
    }

    void limpar() {
        for (int i = 0; i < tamanho; i++) {
            bits[i] = false;
        }

        inseridos = 0;
    }

    void inserir(string chave) {
        bits[hash1(chave)] = true;
        bits[hash2(chave)] = true;
        bits[hash3(chave)] = true;
        inseridos++;
    }

    bool talvezExiste(string chave) {
        return bits[hash1(chave)] && bits[hash2(chave)] && bits[hash3(chave)];
    }

    int getInseridos() {
        return inseridos;
    }

    int contarBitsMarcados() {
        int total = 0;

        for (int i = 0; i < tamanho; i++) {
            if (bits[i]) {
                total++;
            }
        }

        return total;
    }

    size_t memoriaAproximada() {
        return ((size_t)tamanho + 7) / 8;
    }
};

// ============================================================
// FUNCAO DE CONSTRUCAO DAS ESTRUTURAS
// ============================================================

void inserirTodasPecasNasEstruturas(
    // Pega as pecas carregadas no vector e cria os indices em todas as estruturas.
    // Essa etapa prepara Lista, Hash, Hash Otimizada, AVL, Trie e Bloom para uso, como as posições do vector.
    
    vector<Peca>& dados,
    ListaEncadeada& lista,
    TabelaHash& hashCodigo,
    HashOtimizada& hashOtimizada,
    NoAVL*& raizAVL,
    Trie& trie,
    BloomFilter& bloom
) {
    for (int i = 0; i < (int)dados.size(); i++) {
        if (dados[i].ativo) {
            lista.inserir(dados[i].codigo_peca, i);
            hashCodigo.inserir(dados[i].codigo_peca, i);
            hashOtimizada.inserir(dados[i].codigo_peca, i);
            raizAVL = inserirAVL(raizAVL, dados[i].codigo_peca, i);
            trie.inserir(dados[i].nome_peca);
            bloom.inserir(dados[i].codigo_peca);
        }
    }
}

double reconstruirTrieEBloomAuxiliares(vector<Peca>& dados, Trie& trie, BloomFilter& bloom) {
    // Como Trie e Bloom sao auxiliares, apos uma remocao elas sao reconstruidas.
    // Isso evita que elas continuem considerando pecas removidas.
    
    double tempo = medirTempoDaOperacao([&]() {
        trie.limpar();
        bloom.limpar();

        for (int i = 0; i < (int)dados.size(); i++) {
            if (dados[i].ativo) {
                trie.inserir(dados[i].nome_peca);
                bloom.inserir(dados[i].codigo_peca);
            }
        }
    });

    return tempo;
}

// ============================================================
// OPERACOES DO SISTEMA
// ============================================================

// Opcao 1 do menu: busca uma peca pelo codigo usando a estrutura escolhida pelo usuario.
void menuBuscarPecaPorCodigo(

    vector<Peca>& dados,
    ListaEncadeada& lista,
    TabelaHash& hashCodigo,
    HashOtimizada& hashOtimizada,
    NoAVL* raizAVL
) {
    string codigo;
    int opcao;
    int indice = -1;

    string nomeEstrutura;
    string complexidadeTeorica;
    function<int(string)> funcaoBusca;

    cout << "\nDigite o codigo da peca: ";
    cin >> codigo;

    codigo = transformarParaMaiusculo(removerEspacosDasPontas(codigo));

    cout << "\nEscolha a estrutura principal:";
    cout << "\n1 - Lista Encadeada";
    cout << "\n2 - Tabela Hash";
    cout << "\n3 - Hash Otimizada";
    cout << "\n4 - Arvore AVL";
    cout << "\nDigite: ";
    cin >> opcao;

    if (opcao == 1) {
        nomeEstrutura = "Lista Encadeada";
        complexidadeTeorica = "O(n)";
        funcaoBusca = [&](string chave) {
            return lista.buscar(chave);
        };
    }
    else if (opcao == 2) {
        nomeEstrutura = "Tabela Hash";
        complexidadeTeorica = "O(1) medio";
        funcaoBusca = [&](string chave) {
            return hashCodigo.buscar(chave);
        };
    }
    else if (opcao == 3) {
        nomeEstrutura = "Hash Otimizada";
        complexidadeTeorica = "O(1) medio";
        funcaoBusca = [&](string chave) {
            return hashOtimizada.buscar(chave);
        };
    }
    else if (opcao == 4) {
        nomeEstrutura = "Arvore AVL";
        complexidadeTeorica = "O(log n)";
        funcaoBusca = [&](string chave) {
            return buscarAVL(raizAVL, chave);
        };
    }
    else {
        cout << "Opcao invalida.\n";
        return;
    }

    auto inicioConsulta = chrono::high_resolution_clock::now();
    indice = funcaoBusca(codigo);
    auto fimConsulta = chrono::high_resolution_clock::now();

    chrono::duration<double> tempoConsulta = fimConsulta - inicioConsulta;

    volatile int encontrados = 0;

    auto inicioMedia = chrono::high_resolution_clock::now();

    for (int i = 0; i < REPETICOES_CONSULTA; i++) {
        if (funcaoBusca(codigo) != -1) {
            encontrados++;
        }
    }

    auto fimMedia = chrono::high_resolution_clock::now();

    chrono::duration<double> tempoTotalMedia = fimMedia - inicioMedia;
    double tempoMedioSegundos = tempoTotalMedia.count() / REPETICOES_CONSULTA;

    if (indice != -1 && dados[indice].ativo) {
        exibirPecaCompleta(dados[indice]);
    } else {
        cout << "\nPeca nao encontrada.\n";
    }

    cout << "\n===== DADOS DA CONSULTA =====";
    cout << "\nEstrutura utilizada: " << nomeEstrutura;
    cout << "\nComplexidade teorica: " << complexidadeTeorica;

    cout << fixed << setprecision(10);
    cout << "\nTempo da consulta unica: " << tempoConsulta.count() << " segundos";
    cout << "\nTempo total das repeticoes: " << tempoTotalMedia.count() << " segundos";
    cout << "\nTempo medio por consulta: " << tempoMedioSegundos << " segundos";

    cout << "\nRepeticoes usadas na media: " << REPETICOES_CONSULTA;
    cout << "\n=============================\n";
}

// Opcao 4 do menu: usa a Trie auxiliar para contar nomes que iniciam com um prefixo.
void menuBuscarPorPrefixoDoNome(vector<Peca>& dados, Trie& trie) {

    string prefixo;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "\nDigite o prefixo do nome da peca: ";
    getline(cin, prefixo);

    prefixo = removerEspacosDasPontas(prefixo);

    auto inicioTrie = chrono::high_resolution_clock::now();
    int qtdTrie = trie.contarPrefixo(prefixo);
    auto fimTrie = chrono::high_resolution_clock::now();

    chrono::duration<double> tempoTrie = fimTrie - inicioTrie;

    cout << "\nQuantidade encontrada pela Trie auxiliar: " << qtdTrie;
    imprimirTempoEmSegundosEMicrossegundos("Tempo da consulta na Trie", tempoTrie.count());

    cout << "\n\nExemplos encontrados pela lista principal de dados ativos:\n";

    int contador = 0;

    for (int i = 0; i < (int)dados.size(); i++) {
        if (dados[i].ativo && textoComecaComPrefixo(dados[i].nome_peca, prefixo)) {
            exibirPecaResumida(dados[i]);

            contador++;

            if (contador >= 20) {
                break;
            }
        }
    }

    if (contador == 0) {
        cout << "Nenhum exemplo encontrado.\n";
    }

    cout << "\n\nObservacao: a Trie e usada como indice auxiliar para consulta por prefixo.\n";
}

// Opcao 5 do menu: usa Bloom Filter como filtro probabilistico e Hash como confirmacao real.
void menuVerificarCodigoComBloom(TabelaHash& hashCodigo, BloomFilter& bloom) {

    string codigo;

    cout << "\nDigite o codigo da peca: ";
    cin >> codigo;

    codigo = removerEspacosDasPontas(codigo);

    double tempoBloom = 0.0;
    double tempoHash = 0.0;

    bool bloomResposta = false;
    int hashResposta = -1;

    tempoBloom = medirTempoDaOperacao([&]() {
        bloomResposta = bloom.talvezExiste(codigo);
    });

    tempoHash = medirTempoDaOperacao([&]() {
        hashResposta = hashCodigo.buscar(codigo);
    });

    cout << "\nBloom Filter auxiliar: ";

    if (bloomResposta) {
        cout << "provavelmente existe.";
    } else {
        cout << "com certeza nao existe.";
    }

    cout << "\nHash principal de confirmacao: ";

    if (hashResposta != -1) {
        cout << "existe no dataset.";
    } else {
        cout << "nao encontrado no dataset.";
    }

    imprimirTempoEmSegundosEMicrossegundos("\nTempo Bloom", tempoBloom);
    imprimirTempoEmSegundosEMicrossegundos("Tempo Hash de confirmacao", tempoHash);

    cout << "\n\nObservacao: Bloom Filter e auxiliar e probabilistico. A confirmacao real e feita pela Hash.\n";
}

// Opcao 6 do menu: percorre o vector e exibe pecas de uma categoria especifica.
void menuFiltrarPorCategoria(vector<Peca>& dados) {

    string categoria;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "\nDigite a categoria: ";
    getline(cin, categoria);

    categoria = transformarParaMaiusculo(removerEspacosDasPontas(categoria));

    int contador = 0;

    for (int i = 0; i < (int)dados.size(); i++) {
        if (dados[i].ativo && transformarParaMaiusculo(dados[i].categoria) == categoria) {
            exibirPecaResumida(dados[i]);

            contador++;

            if (contador >= LIMITE_RESULTADOS) {
                cout << "\n\nMostrando apenas os " << LIMITE_RESULTADOS << " primeiros resultados.\n";
                break;
            }
        }
    }

    if (contador == 0) {
        cout << "Nenhuma peca encontrada para essa categoria.\n";
    }

    cout << endl;
}

// Opcao 7 do menu: calcula as estatisticas gerais do catalogo e do estoque.
void menuMostrarEstatisticas(vector<Peca>& dados) {

    int total = 0;
    int disponiveis = 0;
    int estoqueBaixo = 0;
    int semEstoque = 0;
    int precoZero = 0;
    int modeloNaoIdentificado = 0;

    int totalPrecosValidos = 0;

    double somaPrecosValidos = 0.0;
    double somaQuadradosValidos = 0.0;

    double somaPrecosBruta = 0.0;
    double valorTotalEstoque = 0.0;

    double maiorPrecoValido = 0.0;
    double menorPrecoValido = 0.0;
    bool primeiroPrecoValido = true;

    map<string, int> porCategoria;

    for (int i = 0; i < (int)dados.size(); i++) {
        if (!dados[i].ativo) {
            continue;
        }

        total++;

        valorTotalEstoque += dados[i].preco * dados[i].quantidade_estoque;
        somaPrecosBruta += dados[i].preco;
        porCategoria[dados[i].categoria]++;

        if (dados[i].status == "SEM ESTOQUE") {
            semEstoque++;
        } else if (dados[i].status == "ESTOQUE BAIXO") {
            estoqueBaixo++;
        } else {
            disponiveis++;
        }

        if (dados[i].preco == 0) {
            precoZero++;
        } else {
            totalPrecosValidos++;

            somaPrecosValidos += dados[i].preco;
            somaQuadradosValidos += dados[i].preco * dados[i].preco;

            if (primeiroPrecoValido) {
                maiorPrecoValido = dados[i].preco;
                menorPrecoValido = dados[i].preco;
                primeiroPrecoValido = false;
            } else {
                if (dados[i].preco > maiorPrecoValido) {
                    maiorPrecoValido = dados[i].preco;
                }

                if (dados[i].preco < menorPrecoValido) {
                    menorPrecoValido = dados[i].preco;
                }
            }
        }

        if (transformarParaMaiusculo(dados[i].modelo_moto).find("IDENTIFICADO") != string::npos) {
            modeloNaoIdentificado++;
        }
    }

    if (total == 0) {
        cout << "Nenhum registro ativo.\n";
        return;
    }

    double mediaBruta = somaPrecosBruta / total;

    cout << "\n===== ESTATISTICAS =====";
    cout << "\nTotal de pecas ativas: " << total;
    cout << "\nPreco medio bruto: R$ " << fixed << setprecision(2) << mediaBruta;

    if (totalPrecosValidos > 0) {
        double mediaValida = somaPrecosValidos / totalPrecosValidos;
        double varianciaValida = (somaQuadradosValidos / totalPrecosValidos) - (mediaValida * mediaValida);

        if (varianciaValida < 0) {
            varianciaValida = 0;
        }

        double desvioValido = sqrt(varianciaValida);

        cout << "\nPreco medio valido: R$ " << fixed << setprecision(2) << mediaValida;
        cout << "\nDesvio padrao dos precos validos: R$ " << fixed << setprecision(2) << desvioValido;
        cout << "\nMaior preco valido: R$ " << fixed << setprecision(2) << maiorPrecoValido;
        cout << "\nMenor preco valido: R$ " << fixed << setprecision(2) << menorPrecoValido;
    } else {
        cout << "\nPreco medio valido: nao foi possivel calcular";
        cout << "\nDesvio padrao dos precos validos: nao foi possivel calcular";
        cout << "\nMaior preco valido: nao foi possivel calcular";
        cout << "\nMenor preco valido: nao foi possivel calcular";
    }

    cout << "\nValor total estimado em estoque: R$ " << fixed << setprecision(2) << valorTotalEstoque;

    cout << "\nDisponiveis: " << disponiveis;
    cout << "\nEstoque baixo: " << estoqueBaixo;
    cout << "\nSem estoque: " << semEstoque;

    cout << "\nPecas com preco zero: " << precoZero;
    cout << "\nTotal de precos validos: " << totalPrecosValidos;
    cout << "\nModelo nao identificado: " << modeloNaoIdentificado;

    cout << "\n\nQuantidade por categoria:";

    for (map<string, int>::iterator it = porCategoria.begin(); it != porCategoria.end(); it++) {
        cout << "\n- " << it->first << ": " << it->second;
    }

    cout << endl;
}

// Opcao 8 do menu: lista pecas em que estoque <= quantidade minima (faz uma condição)
void menuListarEstoqueCritico(vector<Peca>& dados) {
 
    int contador = 0;

    cout << "\n===== PECAS COM ESTOQUE CRITICO =====\n";

    for (int i = 0; i < (int)dados.size(); i++) {
        if (dados[i].ativo && dados[i].quantidade_estoque <= dados[i].quantidade_minima) {
            exibirPecaResumida(dados[i]);

            contador++;

            if (contador >= LIMITE_RESULTADOS) {
                cout << "\n\nMostrando apenas os " << LIMITE_RESULTADOS << " primeiros resultados.\n";
                break;
            }
        }
    }

    if (contador == 0) {
        cout << "Nenhuma peca com estoque critico encontrada.\n";
    }

    cout << endl;
}

// ============================================================
// INSERIR E REMOVER COM TEMPOS
// ============================================================

// Opcao 2 do menu: cadastra uma nova peca, atualiza estruturas e salva no TXT.
void menuInserirNovaPeca(

    vector<Peca>& dados,
    ListaEncadeada& lista,
    TabelaHash& hashCodigo,
    HashOtimizada& hashOtimizada,
    NoAVL*& raizAVL,
    Trie& trie,
    BloomFilter& bloom
) {
    Peca p;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << "\nCodigo da peca: ";
    getline(cin, p.codigo_peca);

    p.codigo_peca = transformarParaMaiusculo(removerEspacosDasPontas(p.codigo_peca));

    if (p.codigo_peca.empty()) {
        cout << "Erro: o codigo da peca nao pode ficar vazio.\n";
        return;
    }

    int indiceExistente = hashCodigo.buscar(p.codigo_peca);

    if (indiceExistente != -1 && dados[indiceExistente].ativo) {
        cout << "Ja existe uma peca ATIVA com esse codigo.\n";
        return;
    }

    cout << "Descricao original: ";
    getline(cin, p.descricao_original);
    p.descricao_original = transformarParaMaiusculo(removerEspacosDasPontas(p.descricao_original));

    cout << "Nome da peca: ";
    getline(cin, p.nome_peca);
    p.nome_peca = transformarParaMaiusculo(removerEspacosDasPontas(p.nome_peca));

    cout << "Categoria: ";
    getline(cin, p.categoria);
    p.categoria = transformarParaMaiusculo(removerEspacosDasPontas(p.categoria));

    cout << "Marca/Fabricante: ";
    getline(cin, p.marca_fabricante);
    p.marca_fabricante = transformarParaMaiusculo(removerEspacosDasPontas(p.marca_fabricante));

    cout << "Modelo da moto: ";
    getline(cin, p.modelo_moto);
    p.modelo_moto = transformarParaMaiusculo(removerEspacosDasPontas(p.modelo_moto));

    p.preco = lerNumeroDecimalDoUsuario("Preco: ");
    p.quantidade_estoque = lerNumeroInteiroDoUsuario("Quantidade em estoque: ");
    p.quantidade_minima = lerNumeroInteiroDoUsuario("Quantidade minima: ");

    p.status = calcularStatusDoEstoque(p.quantidade_estoque, p.quantidade_minima);
    p.ativo = true;

    dados.push_back(p);
    int indice = dados.size() - 1;

    double tempoLista = medirTempoDaOperacao([&]() {
        lista.inserir(p.codigo_peca, indice);
    });

    double tempoHash = medirTempoDaOperacao([&]() {
        hashCodigo.inserir(p.codigo_peca, indice);
    });

    double tempoHashOpt = medirTempoDaOperacao([&]() {
        hashOtimizada.inserir(p.codigo_peca, indice);
    });

    double tempoAVL = medirTempoDaOperacao([&]() {
        raizAVL = inserirAVL(raizAVL, p.codigo_peca, indice);
    });

    double tempoTrie = medirTempoDaOperacao([&]() {
        trie.inserir(p.nome_peca);
    });

    double tempoBloom = medirTempoDaOperacao([&]() {
        bloom.inserir(p.codigo_peca);
    });

    bool arquivoSalvo = salvarPecasAtivasNoArquivoTXT(NOME_ARQUIVO, dados);

    cout << "\nPeca inserida com sucesso.";
    if (arquivoSalvo) {
        cout << "\nArquivo TXT atualizado com a nova peca.";
    } else {
        cout << "\nAtencao: a peca foi inserida na memoria, mas nao foi salva no TXT.";
    }
    cout << "\nStatus calculado: " << p.status;

    cout << "\n\n===== TEMPOS DE INSERCAO =====";
    imprimirTempoEmSegundosEMicrossegundos("Lista Encadeada", tempoLista);
    imprimirTempoEmSegundosEMicrossegundos("Tabela Hash", tempoHash);
    imprimirTempoEmSegundosEMicrossegundos("Hash Otimizada", tempoHashOpt);
    imprimirTempoEmSegundosEMicrossegundos("Arvore AVL", tempoAVL);
    imprimirTempoEmSegundosEMicrossegundos("Trie auxiliar", tempoTrie);
    imprimirTempoEmSegundosEMicrossegundos("Bloom Filter auxiliar", tempoBloom);
    cout << "\n==============================\n";
}

// Opcao 3 do menu: remove logicamente a peca, atualiza estruturas e salva no TXT (usa o campo -> ativo)
void menuRemoverPeca(

    vector<Peca>& dados,
    ListaEncadeada& lista,
    TabelaHash& hashCodigo,
    HashOtimizada& hashOtimizada,
    NoAVL*& raizAVL,
    Trie& trie,
    BloomFilter& bloom
) {
    string codigo;

    cout << "\nDigite o codigo da peca para remover: ";
    cin >> codigo;

    codigo = removerEspacosDasPontas(codigo);

    int indice = hashCodigo.buscar(codigo);

    if (indice == -1 || !dados[indice].ativo) {
        cout << "Peca nao encontrada.\n";
        return;
    }

    dados[indice].ativo = false;

    double tempoLista = medirTempoDaOperacao([&]() {
        lista.remover(codigo);
    });

    double tempoHash = medirTempoDaOperacao([&]() {
        hashCodigo.remover(codigo);
    });

    double tempoHashOpt = medirTempoDaOperacao([&]() {
        hashOtimizada.remover(codigo);
    });

    double tempoAVL = medirTempoDaOperacao([&]() {
        raizAVL = removerAVL(raizAVL, codigo);
    });

    double tempoReconstrucaoAux = reconstruirTrieEBloomAuxiliares(dados, trie, bloom);

    bool arquivoSalvo = salvarPecasAtivasNoArquivoTXT(NOME_ARQUIVO, dados);

    cout << "\nPeca removida com sucesso das estruturas principais.";
    if (arquivoSalvo) {
        cout << "\nArquivo TXT atualizado apos a remocao.";
    } else {
        cout << "\nAtencao: a peca foi removida da memoria, mas a alteracao nao foi salva no TXT.";
    }
    cout << "\nEstruturas principais atualizadas: Lista, Hash, Hash Otimizada e AVL.";
    cout << "\nEstruturas auxiliares reconstruidas: Trie e Bloom Filter.";

    cout << "\n\n===== TEMPOS DE REMOCAO =====";
    imprimirTempoEmSegundosEMicrossegundos("Lista Encadeada", tempoLista);
    imprimirTempoEmSegundosEMicrossegundos("Tabela Hash", tempoHash);
    imprimirTempoEmSegundosEMicrossegundos("Hash Otimizada", tempoHashOpt);
    imprimirTempoEmSegundosEMicrossegundos("Arvore AVL", tempoAVL);
    imprimirTempoEmSegundosEMicrossegundos("Reconstrucao dos indices auxiliares Trie/Bloom", tempoReconstrucaoAux);
    cout << "\n=============================\n";
}

// ===========================================================
// BENCHMARKS
// ==========================================================

vector<string> montarListaDeCodigosParaTestes(vector<Peca>& dados, int limite) {
    vector<string> codigos;

    for (int i = 0; i < (int)dados.size() && (int)codigos.size() < limite; i++) {
        if (dados[i].ativo) {
            codigos.push_back(dados[i].codigo_peca);
        }
    }

    return codigos;
}

vector<int> montarListaDeIndicesAtivosParaTestes(vector<Peca>& dados, int limite) {
    vector<int> indices;

    for (int i = 0; i < (int)dados.size() && (int)indices.size() < limite; i++) {
        if (dados[i].ativo) {
            indices.push_back(i);
        }
    }

    return indices;
}

double medirTempoDasBuscas(vector<string>& chaves, function<int(string)> funcaoBusca) {
    volatile int encontrados = 0;

    auto inicio = chrono::high_resolution_clock::now();

    for (int i = 0; i < (int)chaves.size(); i++) {
        if (funcaoBusca(chaves[i]) != -1) {
            encontrados++;
        }
    }

    auto fim = chrono::high_resolution_clock::now();

    chrono::duration<double> tempo = fim - inicio;
    return tempo.count();
}

// Opcao 9 do menu: compara o tempo de busca por codigo nas estruturas principais.
void menuBenchmarkBuscaPorCodigo(

    ListaEncadeada& lista,
    TabelaHash& hashCodigo,
    HashOtimizada& hashOtimizada,
    NoAVL* raizAVL,
    vector<Peca>& dados
) {
    vector<string> codigos = montarListaDeCodigosParaTestes(dados, LIMITE_BENCHMARK);

    if (codigos.empty()) {
        cout << "Sem dados para benchmark.\n";
        return;
    }

    double tLista = medirTempoDasBuscas(codigos, [&](string codigo) {
        return lista.buscar(codigo);
    });

    double tHash = medirTempoDasBuscas(codigos, [&](string codigo) {
        return hashCodigo.buscar(codigo);
    });

    double tHashOtimizada = medirTempoDasBuscas(codigos, [&](string codigo) {
        return hashOtimizada.buscar(codigo);
    });

    double tAVL = medirTempoDasBuscas(codigos, [&](string codigo) {
        return buscarAVL(raizAVL, codigo);
    });

    cout << "\n===== BENCHMARK: BUSCA POR CODIGO =====";
    cout << "\nQuantidade de buscas: " << codigos.size();
    cout << fixed << setprecision(10);
    cout << "\nLista Encadeada: " << tLista << " segundos";
    cout << "\nTabela Hash: " << tHash << " segundos";
    cout << "\nHash Otimizada: " << tHashOtimizada << " segundos";
    cout << "\nArvore AVL: " << tAVL << " segundos";
    cout << "\nColisoes na Hash: " << hashCodigo.getColisoes();
    cout << "\nColisoes na Hash Otimizada: " << hashOtimizada.getColisoes();
    cout << "\nRehashes da Hash Otimizada: " << hashOtimizada.getRehashes() << endl;
}

int contarPrefixoPercorrendoLista(vector<Peca>& dados, string prefixo) {
    int contador = 0;

    for (int i = 0; i < (int)dados.size(); i++) {
        if (dados[i].ativo && textoComecaComPrefixo(dados[i].nome_peca, prefixo)) {
            contador++;
        }
    }

    return contador;
}

// Opcao 10 do menu: compara busca por prefixo usando Lista e Trie auxiliar.
void menuBenchmarkListaVersusTrie(vector<Peca>& dados, Trie& trie) {
  
    vector<string> prefixos;

    prefixos.push_back("FILTRO");
    prefixos.push_back("OLEO");
    prefixos.push_back("BATERIA");
    prefixos.push_back("PASTILHA");
    prefixos.push_back("CABO");

    volatile int totalLista = 0;
    volatile int totalTrie = 0;

    auto inicioLista = chrono::high_resolution_clock::now();

    for (int i = 0; i < (int)prefixos.size(); i++) {
        totalLista += contarPrefixoPercorrendoLista(dados, prefixos[i]);
    }

    auto fimLista = chrono::high_resolution_clock::now();

    auto inicioTrie = chrono::high_resolution_clock::now();

    for (int i = 0; i < (int)prefixos.size(); i++) {
        totalTrie += trie.contarPrefixo(prefixos[i]);
    }

    auto fimTrie = chrono::high_resolution_clock::now();

    chrono::duration<double> tLista = fimLista - inicioLista;
    chrono::duration<double> tTrie = fimTrie - inicioTrie;

    cout << "\n===== BENCHMARK: PREFIXO DO NOME =====";
    cout << fixed << setprecision(10);
    cout << "\nLista: " << tLista.count() << " segundos";
    cout << "\nTrie auxiliar: " << tTrie.count() << " segundos";
    cout << "\nTotal encontrado Lista: " << totalLista;
    cout << "\nTotal encontrado Trie: " << totalTrie;
    cout << "\nObservacao: comparacao valida apenas para busca textual por prefixo." << endl;
}

// Opcao 11 do menu: compara Bloom Filter e Hash em teste de existencia.
void menuBenchmarkBloomVersusHash(vector<Peca>& dados, BloomFilter& bloom, TabelaHash& hashCodigo) {
  
    vector<string> codigos = montarListaDeCodigosParaTestes(dados, LIMITE_BENCHMARK);

    if (codigos.empty()) {
        cout << "Sem codigos para benchmark.\n";
        return;
    }

    volatile int encontradosBloom = 0;
    volatile int encontradosHash = 0;
    int falsosPositivos = 0;

    auto inicioBloom = chrono::high_resolution_clock::now();

    for (int i = 0; i < (int)codigos.size(); i++) {
        if (bloom.talvezExiste(codigos[i])) {
            encontradosBloom++;
        }
    }

    auto fimBloom = chrono::high_resolution_clock::now();

    auto inicioHash = chrono::high_resolution_clock::now();

    for (int i = 0; i < (int)codigos.size(); i++) {
        if (hashCodigo.buscar(codigos[i]) != -1) {
            encontradosHash++;
        }
    }

    auto fimHash = chrono::high_resolution_clock::now();

    for (int i = 0; i < (int)codigos.size(); i++) {
        string codigoInexistente = "999999" + to_string(i);

        if (bloom.talvezExiste(codigoInexistente)) {
            falsosPositivos++;
        }
    }

    chrono::duration<double> tBloom = fimBloom - inicioBloom;
    chrono::duration<double> tHash = fimHash - inicioHash;

    cout << "\n===== BENCHMARK: BLOOM x HASH =====";
    cout << fixed << setprecision(10);
    cout << "\nBloom Filter auxiliar: " << tBloom.count() << " segundos";
    cout << "\nHash de confirmacao: " << tHash.count() << " segundos";
    cout << "\nBloom encontrados: " << encontradosBloom;
    cout << "\nHash encontrados: " << encontradosHash;
    cout << "\nFalsos positivos simulados no Bloom: " << falsosPositivos;
    cout << fixed << setprecision(2);
    cout << "\nTaxa de falso positivo: " << (falsosPositivos * 100.0 / codigos.size()) << "%" << endl;
}

// ============================================================
// TESTES COM RESTRICOES
// ============================================================

// Opcao 13 do menu: executa os testes de memoria, processamento, latencia, dados e algoritmo.
void menuExecutarTestesRestritivos(

    vector<Peca>& dados,
    ListaEncadeada& lista,
    TabelaHash& hashCodigo,
    HashOtimizada& hashOtimizada,
    NoAVL* raizAVL
) {
    if (dados.empty()) {
        cout << "Sem dados para testes.\n";
        return;
    }

    cout << "\n===== TESTES COM RESTRICOES =====";

    // ------------------------------------------------------------
    // 1 - Restricao de memoria
    // Simula uma Hash muito pequena para aumentar colisoes (R2).
    // ------------------------------------------------------------
    int tamanhoHashPequena = 101;
    vector<int> ocupado(tamanhoHashPequena, 0);
    int colisoesMemoria = 0;

    for (int i = 0; i < (int)dados.size(); i++) {
        if (dados[i].ativo) {
            unsigned long h = 0;

            for (int j = 0; j < (int)dados[i].codigo_peca.size(); j++) {
                h = h * 31 + dados[i].codigo_peca[j];
            }

            int pos = h % tamanhoHashPequena;

            if (ocupado[pos] > 0) {
                colisoesMemoria++;
            }

            ocupado[pos]++;
        }
    }

    cout << "\n\n1 - Restricao de memoria";
    cout << "\nHash pequena simulada com tamanho 101.";
    cout << "\nColisoes observadas: " << colisoesMemoria;

    vector<string> codigos = montarListaDeCodigosParaTestes(dados, LIMITE_BENCHMARK);

    if (codigos.empty()) {
        cout << "\nSem codigos ativos para os demais testes.\n";
        return;
    }

    // ------------------------------------------------------------
    // 2 - Restricao de processamento
    // Executa uma grande quantidade de buscas na Hash (R9).
    // ------------------------------------------------------------
    auto inicioProcessamento = chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; i++) {
        hashCodigo.buscar(codigos[i % codigos.size()]);
    }

    auto fimProcessamento = chrono::high_resolution_clock::now();
    chrono::duration<double> tempoProcessamento = fimProcessamento - inicioProcessamento;

    cout << "\n\n2 - Restricao de processamento";
    cout << "\n100000 buscas na Hash.";
    cout << "\nTempo: " << fixed << setprecision(10) << tempoProcessamento.count() << " segundos";

    // ------------------------------------------------------------
    // 3 - Restricao de latencia
    // Adiciona atraso artificial para simular ambiente lento (R13).
    // ------------------------------------------------------------
    volatile long dummy = 0;

    auto inicioLatencia = chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; i++) {
        hashCodigo.buscar(codigos[i % codigos.size()]);

        for (int j = 0; j < 3000; j++) {
            dummy += j;
        }
    }

    auto fimLatencia = chrono::high_resolution_clock::now();
    chrono::duration<double> tempoLatencia = fimLatencia - inicioLatencia;

    cout << "\n\n3 - Restricao de latencia";
    cout << "\n1000 buscas com atraso artificial.";
    cout << "\nTempo: " << fixed << setprecision(10) << tempoLatencia.count() << " segundos";

    // ------------------------------------------------------------
    // 4 - Restricao de dados
    // Verifica campos com dados insuficientes do dataset (R18 e R20)
    // ------------------------------------------------------------
    int precoZero = 0;
    int modeloNaoIdentificado = 0;

    for (int i = 0; i < (int)dados.size(); i++) {
        if (dados[i].ativo) {
            if (dados[i].preco == 0) {
                precoZero++;
            }

            if (transformarParaMaiusculo(dados[i].modelo_moto).find("IDENTIFICADO") != string::npos) {
                modeloNaoIdentificado++;
            }
        }
    }

    cout << "\n\n4 - Restricao de dados";
    cout << "\nPecas com preco zero: " << precoZero;
    cout << "\nPecas com modelo nao identificado: " << modeloNaoIdentificado;

    // ------------------------------------------------------------
    // 5 - Restricao algoritmica/estrutural
    // Executa comparacao direta entre as estruturas principais sobre o tempo (R21)
    // ------------------------------------------------------------
    double tLista = medirTempoDasBuscas(codigos, [&](string codigo) {
        return lista.buscar(codigo);
    });

    double tHash = medirTempoDasBuscas(codigos, [&](string codigo) {
        return hashCodigo.buscar(codigo);
    });

    double tHashOpt = medirTempoDasBuscas(codigos, [&](string codigo) {
        return hashOtimizada.buscar(codigo);
    });

    double tAVL = medirTempoDasBuscas(codigos, [&](string codigo) {
        return buscarAVL(raizAVL, codigo);
    });

    cout << "\n\n5 - Restricao algoritmica e estrutural";
    cout << "\nComparacao direta executada entre estruturas principais.";
    cout << "\nQuantidade de buscas: " << codigos.size();
    cout << fixed << setprecision(10);
    cout << "\nLista Encadeada O(n): " << tLista << " segundos";
    cout << "\nTabela Hash O(1) medio: " << tHash << " segundos";
    cout << "\nHash Otimizada O(1) medio: " << tHashOpt << " segundos";
    cout << "\nArvore AVL O(log n): " << tAVL << " segundos";

    cout << "\n\nInterpretacao:";
    cout << "\nA Lista representa uma estrutura menos eficiente para busca por codigo.";
    cout << "\nHash, Hash Otimizada e AVL representam estruturas mais adequadas para consulta por chave.";

    // ------------------------------------------------------------
    // Complemento de escalabilidade
    // A ideia é aumentar a quantidade de consultas e observar o crescimento do tempo
    // ------------------------------------------------------------
    vector<int> cargas;
    cargas.push_back(1000);
    cargas.push_back(3000);
    cargas.push_back(5000);
    cargas.push_back(10000);

    cout << "\n\n6 - Complemento de escalabilidade";
    cout << "\nAumentando a quantidade de consultas nas estruturas principais.";

    for (int c = 0; c < (int)cargas.size(); c++) {
        int limite = cargas[c];

        if (limite > (int)dados.size()) {
            continue;
        }

        vector<string> codigosEscala = montarListaDeCodigosParaTestes(dados, limite);

        if (codigosEscala.empty()) {
            continue;
        }

        double eLista = medirTempoDasBuscas(codigosEscala, [&](string codigo) {
            return lista.buscar(codigo);
        });

        double eHash = medirTempoDasBuscas(codigosEscala, [&](string codigo) {
            return hashCodigo.buscar(codigo);
        });

        double eHashOpt = medirTempoDasBuscas(codigosEscala, [&](string codigo) {
            return hashOtimizada.buscar(codigo);
        });

        double eAVL = medirTempoDasBuscas(codigosEscala, [&](string codigo) {
            return buscarAVL(raizAVL, codigo);
        });

        cout << "\nConsultas: " << codigosEscala.size();
        cout << " | Lista=" << eLista;
        cout << " | Hash=" << eHash;
        cout << " | HashOpt=" << eHashOpt;
        cout << " | AVL=" << eAVL;
    }

    cout << "\n\n==================================\n";
}


// ============================================================
// INFORMACOES da Hash simples e Hash otimizada
// ============================================================

// Opcao 12 do menu: mostra tamanho, colisoes, fator de carga e rehash da Hash.
void menuMostrarInformacoesHash(TabelaHash& hashCodigo, HashOtimizada& hashOtimizada) {

    cout << "\n===== INFORMACOES DA HASH =====";
    cout << "\nHash simples:";
    cout << "\nTamanho fixo: " << hashCodigo.getTamanho();
    cout << "\nQuantidade: " << hashCodigo.getQuantidade();
    cout << "\nColisoes: " << hashCodigo.getColisoes();
    cout << fixed << setprecision(4);
    cout << "\nFator de carga: " << hashCodigo.getFatorCarga();

    cout << "\n\nHash otimizada:";
    cout << "\nTamanho atual: " << hashOtimizada.getTamanho();
    cout << "\nQuantidade: " << hashOtimizada.getQuantidade();
    cout << "\nColisoes: " << hashOtimizada.getColisoes();
    cout << "\nRehashes: " << hashOtimizada.getRehashes();
    cout << "\nFator de carga: " << hashOtimizada.getFatorCarga();

    cout << "\n\nJustificativa:";
    cout << "\nA Hash simples tem tamanho fixo.";
    cout << "\nA Hash otimizada controla fator de carga e redimensiona quando passa de 0.75.";
    cout << "\nIsso atende a comparacao entre uma estrutura original e uma versao otimizada.\n";
}

// Opcao 14 do menu: estima a memoria usada pelas estruturas.
void menuMostrarMemoriaAproximada(

    ListaEncadeada& lista,
    TabelaHash& hashCodigo,
    HashOtimizada& hashOtimizada,
    NoAVL* raizAVL,
    Trie& trie,
    BloomFilter& bloom
) {
    cout << "\n===== MEMORIA APROXIMADA DAS ESTRUTURAS =====";
    cout << "\nLista Encadeada: " << lista.memoriaAproximada() << " bytes";
    cout << "\nTabela Hash: " << hashCodigo.memoriaAproximada() << " bytes";
    cout << "\nHash Otimizada: " << hashOtimizada.memoriaAproximada() << " bytes";
    cout << "\nArvore AVL: " << memoriaAproximadaAVL(raizAVL) << " bytes";
    cout << "\nTrie auxiliar: " << trie.memoriaAproximada() << " bytes";
    cout << "\nBloom Filter auxiliar: " << bloom.memoriaAproximada() << " bytes";
    cout << "\n\nObservacao: a estimativa considera a memoria estrutural aproximada.";
    cout << "\nStrings e estruturas internas da STL podem consumir memoria adicional.\n";
}

// ============================================================
// MENU PRINCIPAL
// ============================================================

int main() {
	
	//carrega o arquivo TXT
    vector<Peca> dados = carregarPecasDoArquivoTXT(NOME_ARQUIVO);

    if (dados.empty()) {
        cout << "\nNenhum dado foi carregado.\n";
        return 0;
    }

// a size calcula a quantidade de registros
    cout << "\nTotal de pecas carregadas: " << dados.size() << endl;

    if (dados.size() < 10000) {
        cout << "\nAVISO: o dataset possui menos de 10.000 registros.";
        cout << "\nConfira o requisito minimo do trabalho.\n";
    }

    ListaEncadeada lista;
    TabelaHash hashCodigo(TAM_HASH_FIXA);
    HashOtimizada hashOtimizada(TAM_HASH_OTIMIZADA_INICIAL);
    NoAVL* raizAVL = NULL;
    Trie trie;
    BloomFilter bloom(TAM_BLOOM);

    inserirTodasPecasNasEstruturas(dados, lista, hashCodigo, hashOtimizada, raizAVL, trie, bloom);

    int opcao;

    do {
        cout << "\n\n========== SISTEMA DE CATALOGO DE PECAS DE MOTOS ==========";
        cout << "\n1  - Buscar peca por codigo";
        cout << "\n2  - Inserir nova peca";
        cout << "\n3  - Remover peca";
        cout << "\n4  - Buscar por prefixo do nome da peca";
        cout << "\n5  - Verificar codigo da peca com Bloom Filter";
        cout << "\n6  - Filtrar por categoria";
        cout << "\n7  - Mostrar estatisticas";
        cout << "\n8  - Listar estoque critico";
        cout << "\n9  - Benchmark Lista x Hash x Hash Otimizada x AVL";
        cout << "\n10 - Benchmark Lista x Trie";
        cout << "\n11 - Benchmark Bloom x Hash";
        cout << "\n12 - Informacoes da Hash e Hash Otimizada";
        cout << "\n13 - Testes com restricoes";
        cout << "\n14 - Memoria aproximada das estruturas";
        cout << "\n0  - Sair";
        cout << "\nDigite: ";

        if (!(cin >> opcao)) {
            cout << "\nEntrada invalida.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if (opcao == 1) {
            menuBuscarPecaPorCodigo(dados, lista, hashCodigo, hashOtimizada, raizAVL);
        } else if (opcao == 2) {
            menuInserirNovaPeca(dados, lista, hashCodigo, hashOtimizada, raizAVL, trie, bloom);
        } else if (opcao == 3) {
            menuRemoverPeca(dados, lista, hashCodigo, hashOtimizada, raizAVL, trie, bloom);
        } else if (opcao == 4) {
            menuBuscarPorPrefixoDoNome(dados, trie);
        } else if (opcao == 5) {
            menuVerificarCodigoComBloom(hashCodigo, bloom);
        } else if (opcao == 6) {
            menuFiltrarPorCategoria(dados);
        } else if (opcao == 7) {
            menuMostrarEstatisticas(dados);
        } else if (opcao == 8) {
            menuListarEstoqueCritico(dados);
        } else if (opcao == 9) {
            menuBenchmarkBuscaPorCodigo(lista, hashCodigo, hashOtimizada, raizAVL, dados);
        } else if (opcao == 10) {
            menuBenchmarkListaVersusTrie(dados, trie);
        } else if (opcao == 11) {
            menuBenchmarkBloomVersusHash(dados, bloom, hashCodigo);
        } else if (opcao == 12) {
            menuMostrarInformacoesHash(hashCodigo, hashOtimizada);
        } else if (opcao == 13) {
            menuExecutarTestesRestritivos(dados, lista, hashCodigo, hashOtimizada, raizAVL);
        } else if (opcao == 14) {
            menuMostrarMemoriaAproximada(lista, hashCodigo, hashOtimizada, raizAVL, trie, bloom);
        } else if (opcao == 0) {
            cout << "\nSistema finalizado.\n";
        } else {
            cout << "Opcao invalida.\n";
        }

    } while (opcao != 0);

    liberarAVL(raizAVL);

    return 0;
}