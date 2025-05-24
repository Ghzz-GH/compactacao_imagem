// Felipe Kanamaru de Oliveira - RA: 10435742 
// Felipe Silva Siqueira - RA: 10445036
// Gustavo Henrique de Sousa Santos - RA: 10721355


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TAM 11000
#define CABECALHO_TAM 54
#define CABECALHO_EXTRA_TAM 84
#define MAX_R 4096

// Vetores globais para guardar os canais RGB
unsigned char vetorR[MAX_R], vetorG[MAX_R], vetorB[MAX_R];
// Índice pra saber onde estamos nesses vetores
int idxRGB = 0;

// Função pra ler um inteiro no formato little endian
int ler_int_little_endian(unsigned char* v, int inicio) {
    return v[inicio] + (v[inicio + 1] << 8) + (v[inicio + 2] << 16) + (v[inicio + 3] << 24);
}

// Pega o pixel central de uma região da matriz
void pegar_pixel_central(unsigned char** matriz, int linha_inicial, int coluna_inicial, int linha_final, int coluna_final, unsigned char* r, unsigned char* g, unsigned char* b) {
    int linha = (linha_inicial + linha_final) / 2;
    int coluna = (coluna_inicial + coluna_final) / 2;
    int index = coluna * 3;
    *r = matriz[linha][index];
    *g = matriz[linha][index + 1];
    *b = matriz[linha][index + 2];
}

// Função recursiva que faz a compactação da imagem
void compactar(unsigned char** matriz, int linha_inicial, int coluna_inicial, int linha_final, int coluna_final) {
    
    // Se o quadrante for bem pequeno, só pega o pixel central. Esta é a base da recursão
    if ((linha_final - linha_inicial <= 3) || (coluna_final - coluna_inicial <= 3)) {
        unsigned char r, g, b;
        pegar_pixel_central(matriz, linha_inicial, coluna_inicial, linha_final, coluna_final, &r, &g, &b);
        vetorR[idxRGB] = r;
        vetorG[idxRGB] = g;
        vetorB[idxRGB] = b;
        idxRGB++;

    } else { // Senão, divide em 4 e chama a função pra cada pedaço novamente (recursividade)
        int linha_meio = (linha_inicial + linha_final) / 2;
        int coluna_meio = (coluna_inicial + coluna_final) / 2;
        compactar(matriz, linha_inicial, coluna_inicial, linha_meio, coluna_meio);
        compactar(matriz, linha_inicial, coluna_meio, linha_meio, coluna_final);
        compactar(matriz, linha_meio, coluna_inicial, linha_final, coluna_meio);
        compactar(matriz, linha_meio, coluna_meio, linha_final, coluna_final);
    }
}

// Mesma ideia da compactar, mas pra reconstruir a imagem a partir dos vetores
void descompactar(unsigned char** matriz, int linha_inicial, int coluna_inicial, int linha_final, int coluna_final) {
    if ((linha_final - linha_inicial <= 3) || (coluna_final - coluna_inicial <= 3)) {
        unsigned char r = vetorR[idxRGB];
        unsigned char g = vetorG[idxRGB];
        unsigned char b = vetorB[idxRGB];
        idxRGB++;
        for (int i = linha_inicial; i < linha_final; i++) {
            for (int j = coluna_inicial * 3; j < coluna_final * 3; j += 3) {
                matriz[i][j] = r;
                matriz[i][j + 1] = g;
                matriz[i][j + 2] = b;
            }
        }
    } else {
        int linha_meio = (linha_inicial + linha_final) / 2;
        int coluna_meio = (coluna_inicial + coluna_final) / 2;
        descompactar(matriz, linha_inicial, coluna_inicial, linha_meio, coluna_meio);
        descompactar(matriz, linha_inicial, coluna_meio, linha_meio, coluna_final);
        descompactar(matriz, linha_meio, coluna_inicial, linha_final, coluna_meio);
        descompactar(matriz, linha_meio, coluna_meio, linha_final, coluna_final);
    }
}

// Aqui é onde tudo acontece: leitura, compactação, gravação, descompactação, relatório
void processar_imagem(const char* entrada_bmp, const char* saida_zmp, const char* saida_bmp) {
    FILE *fp;
    unsigned char cabecalho[CABECALHO_TAM + CABECALHO_EXTRA_TAM];
    unsigned char* imagem[MAX_TAM];
    int linhas, colunas, padding, tamanhoArquivo, offset;

    fp = fopen(entrada_bmp, "rb");
    if (!fp) {
        printf("Erro ao abrir %s\n", entrada_bmp);
        return;
    }

    // Lê cabeçalho e guarda no vetor
    fread(cabecalho, 1, CABECALHO_TAM + CABECALHO_EXTRA_TAM, fp);
    tamanhoArquivo = ler_int_little_endian(cabecalho, 2);
    offset = ler_int_little_endian(cabecalho, 10);
    colunas = ler_int_little_endian(cabecalho, 18);
    linhas = ler_int_little_endian(cabecalho, 22);
    padding = (4 - (colunas * 3) % 4) % 4;

    // Lê dados da imagem e aloca dinamicamente espaço conforme o numero de colunas dados no cabeçalho
    fseek(fp, offset, SEEK_SET);
    for (int i = 0; i < linhas; i++) {
        imagem[i] = (unsigned char*)malloc((colunas * 3 + padding));
        fread(imagem[i], 1, colunas * 3 + padding, fp);
    }
    fclose(fp);

    // compacta a imagem, chamando a função
    idxRGB = 0;
    compactar(imagem, 0, 0, linhas, colunas);

    // Grava compactado (.zmp)
    fp = fopen(saida_zmp, "wb");
    fwrite(cabecalho, 1, CABECALHO_TAM + CABECALHO_EXTRA_TAM, fp);
    for (int i = 0; i < idxRGB; i++) {
        fputc(vetorR[i], fp);
        fputc(vetorG[i], fp);
        fputc(vetorB[i], fp);
    }
    fclose(fp);

    // Lê de volta pra descompactar
    FILE* zmp = fopen(saida_zmp, "rb");
    fread(cabecalho, 1, CABECALHO_TAM + CABECALHO_EXTRA_TAM, zmp);
    idxRGB = 0;
    while (fread(&vetorR[idxRGB], 1, 1, zmp) &&
           fread(&vetorG[idxRGB], 1, 1, zmp) &&
           fread(&vetorB[idxRGB], 1, 1, zmp)) {
        idxRGB++;
    }
    fclose(zmp);

    // Aloca nova imagem
    unsigned char* novaImagem[MAX_TAM];
    for (int i = 0; i < linhas; i++) {
        novaImagem[i] = (unsigned char*)malloc(colunas * 3 + padding);
    }

    // Descompacta, chamando a função
    idxRGB = 0;
    descompactar(novaImagem, 0, 0, linhas, colunas);

    // Grava imagem descompactada a partir da compactação feita
    FILE* out = fopen(saida_bmp, "wb");
    fwrite(cabecalho, 1, offset, out);
    for (int i = 0; i < linhas; i++) {
        fwrite(novaImagem[i], 1, colunas * 3, out);
        for (int j = 0; j < padding; j++) fputc(0, out);
    }
    fclose(out);

    // Valores para o relatório
    FILE* f1 = fopen(entrada_bmp, "rb");
    fseek(f1, 0, SEEK_END);
    int tamanho_original = ftell(f1);
    fclose(f1);

    FILE* f2 = fopen(saida_zmp, "rb");
    fseek(f2, 0, SEEK_END);
    int tamanho_compactado = ftell(f2);
    fclose(f2);

    FILE* f3 = fopen(saida_bmp, "rb");
    fseek(f3, 0, SEEK_END);
    int tamanho_descompactado = ftell(f3);
    fclose(f3);

    float porcentagem = 100.0 * (tamanho_original - tamanho_compactado) / tamanho_descompactado;

    printf("\nImagem: %s\n", entrada_bmp);
    printf("Tamanho original: %d bytes\n", tamanho_original);
    printf("Tamanho compactado: %d bytes\n", tamanho_compactado);
    printf("Tamanho descompactado: %d bytes\n", tamanho_descompactado);
    printf("Porcentagem de compactacao: %.2f%%\n", porcentagem);

    for (int i = 0; i < linhas; i++) {
        free(imagem[i]);
        free(novaImagem[i]);
    }
}

int main() {
    // Para processar outras imagens:
    // processar_imagem("outra.bmp", "outrozmp.zmp", "outradescompactada.bmp");
    // nao se esqueça de colocar as extensões certas: .bmp ; .zmp ; .bmp nesta sequencia
    processar_imagem("imagem22x20.bmp", "imagemCompactada1.zmp", "imagemDescompactada1.bmp");
    processar_imagem("imagemListras.bmp", "imagemCompactadaListras.zmp", "imagemDescompactadaListras.bmp");
    processar_imagem("imagemArcoiris.bmp", "imagemCompactadaArcoiris.zmp", "imagemDescompactadaArcoiris.bmp");

    return 0;
}

