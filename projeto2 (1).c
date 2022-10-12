#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char isbn[14];
    char titulo[50];
    char autor[50];
    char ano[5];
}Livro;

typedef struct {
    char isbn[14];
    int bof;
}indicePrimario;

typedef struct {
    char autor[50];
    int bof;
}indiceSecundario;

typedef struct {
    int qtdRegistrosLidos;
    int size;
}Header_arq;

typedef struct {
    int qtdPrimarioLidos;
    int qtdProcurados;
}Header_primario;

typedef struct {
    int qtdSecundarioLidos;
}Header_secundario;

int open_file() {
    FILE *fp_data, *fp_index_p;
    Header_arq header;
    Header_primario header_p;
    Header_secundario header_s;

    header.qtdRegistrosLidos = 0;
    header.size = sizeof(header);
    header_p.qtdPrimarioLidos = 0;
    header_p.qtdProcurados = 0;
    header_s.qtdSecundarioLidos = 0;
        
    if((fp_data = fopen("arquivo.bin", "r+b")) == NULL) {
        if((fp_data = fopen("arquivo.bin", "w+b")) == NULL) {
            printf("Erro ao abrir o arquivo.\n");
            return 1;
        } else {
            fwrite(&header, sizeof(Header_arq), 1, fp_data);
        }   
    }

    if((fp_index_p = fopen("index-p.bin", "r+b")) == NULL) {
        if((fp_index_p = fopen("index-p.bin", "w+b")) == NULL) {
            printf("Erro ao abrir o arquivo.\n");
            return 1;
        } else {
            fwrite(&header_p, sizeof(Header_primario), 1, fp_index_p);
        }   
    }

    fclose(fp_data);
    fclose(fp_index_p);
    return 0;
}

void keysort() {
    FILE *fp = fopen("index-p.bin", "r+b");
    Header_primario header_p;
    fread(&header_p, sizeof(header_p), 1, fp);

    indicePrimario *indice = malloc(sizeof(indicePrimario) * header_p.qtdPrimarioLidos);
    int i = 0;
    bool b = true;

    while((fread(&indice[i], sizeof(indicePrimario), 1, fp))) {
        i++;
    }
    
    
    indicePrimario aux;
    int j;

    while(b) {
        b = false;

        for(j = 0; j < i-1; j++) {
            if(strcmp(indice[j].isbn, indice[j+1].isbn) > 0) {
                aux = indice[j];
                indice[j] = indice[j+1];
                indice[j+1] = aux;
                b = true;
            }
        }
    }
    fseek(fp, 8, SEEK_SET);

    for(j = 0; j < i; j++) {
        fwrite(&indice[j], sizeof(indicePrimario), 1, fp);
        printf("%s\n", indice[j].isbn);
    }

    fclose(fp);
}

void insert_reg() {
    FILE *fp, *fp_insere, *fp_index_p;

    if((fp = fopen("arquivo.bin", "r+b")) == NULL ||
        (fp_insere = fopen("insere.bin", "rb")) == NULL || 
        (fp_index_p = fopen("index-p.bin", "r+b")) == NULL) {
            printf("Erro ao abrir o arquivo.\n");
            return;
        }

    Header_arq header;
    Header_primario header_p;
    Livro livro;
    indicePrimario indice;
    char buffer[256];
    int buffer_size;

    fread(&header, sizeof(header), 1, fp); //le o header do arquivo principal
    if(header.qtdRegistrosLidos >= 6){
        printf("Todos os registros ja foram inseridos \n");
        return;
    }
    
    fread(&header_p, sizeof(header_p), 1, fp_index_p);
    fseek(fp_insere, header.qtdRegistrosLidos * sizeof(Livro), SEEK_SET); //coloca o ponteiro na posicao de um arquivo nao lido
    fread(&livro, sizeof(Livro), 1, fp_insere); //le o registro
    strcpy(indice.isbn, livro.isbn);

    sprintf(buffer, "%s#%s#%s#%s", livro.isbn, livro.titulo, livro.autor, livro.ano);
    buffer_size = strlen(buffer);

    fseek(fp, 0, SEEK_END); //coloca o ponteiro no final do arquivo
    fwrite(&buffer_size, sizeof(int), 1, fp); //escreve o tamanho do registro
    fwrite(buffer, buffer_size, 1, fp); //escreve o registro
    
    indice.bof = header.size;

    fseek(fp_index_p, 0, SEEK_END);
    fwrite(&indice, sizeof(indice), 1, fp_index_p);
    header.qtdRegistrosLidos++;
    header.size = header.size + buffer_size + sizeof(int);
    header_p.qtdPrimarioLidos++;
    
    rewind(fp);
    rewind(fp_index_p);
    fwrite(&header, sizeof(header), 1, fp); //atualiza o header
    fwrite(&header_p, sizeof(header_p), 1, fp_index_p);

    fclose(fp);
    fclose(fp_insere);
    fclose(fp_index_p);

    keysort();
    printf("Registro carregado.\n");
}

void search_primary() {
    FILE *fp_busca_p, *fp_index, *fp;
    
    if((fp = fopen("arquivo.bin", "rb")) == NULL || 
        (fp_busca_p = fopen("busca_p.bin", "rb")) == NULL || 
        (fp_index = fopen("index-p.bin", "r+b")) == NULL) {
            printf("Erro ao abrir arquivo.\n");
            return;
    }

    Header_primario header_p;
    indicePrimario indice;
    bool b = true;
    bool flag = true;
    char aux[14];

    fread(&header_p, sizeof(header_p), 1, fp_index);
    fseek(fp_busca_p, header_p.qtdProcurados * 14, SEEK_SET);
    fread(aux, sizeof(aux), 1, fp_busca_p);
    header_p.qtdProcurados++;
    
    while(b) {
        fread(&indice, sizeof(indice), 1, fp_index);
        
        if(strcmp(aux, indice.isbn) == 0) {
            b = false;
            flag = false;
        }

        if((feof(fp_index))) {
            b = false;
        }
    }

    if(flag) {
        printf("O registro de ISBN %s nao existe no arquivo.\n", aux);
        rewind(fp_index);
        fwrite(&header_p, sizeof(header_p), 1, fp_index);
        fclose(fp);
        fclose(fp_busca_p);
        fclose(fp_index);
        return;
    }

    
    int reg_size;
    char buffer[512];
    Livro livro;
    char *tok;
    fseek(fp, indice.bof, SEEK_SET);
    fread(&reg_size, sizeof(int), 1, fp);
    fread(buffer, reg_size, 1, fp);
    
    tok = strtok(buffer, "#");
    strcpy(livro.isbn, tok);
    tok = strtok(NULL, "#");
    strcpy(livro.titulo, tok);
    tok = strtok(NULL, "#");
    strcpy(livro.autor, tok);
    tok = strtok(NULL, "#");
    strcpy(livro.ano, tok);
    livro.ano[4] = '\0';

    printf("\n=========Dados do livro=========\n");
    printf("ISBN: %s\n", livro.isbn);
    printf("Titulo: %s\n", livro.titulo);
    printf("Autor: %s\n", livro.autor);
    printf("Ano: %s\n\n", livro.ano);
    
    rewind(fp_index);
    fwrite(&header_p, sizeof(header_p), 1, fp_index);
    fclose(fp);
    fclose(fp_busca_p);
    fclose(fp_index);
}

int main() {
    open_file();

    int op;

    while(op != 4) {
        printf("1. Inserir registro.\n");
        printf("2. Pesquisa por chave primaria.\n");
        printf("3. Pesquisa por chave secundaria.\n");
        printf("4. Sair.\n");
        printf("Opcao: ");
        scanf("%d", &op);

        switch(op) {
            case 1:
                insert_reg();
                break;

            case 2:
                search_primary();
                break;

            case 3:
                //search_secondary();
                break;

            case 4:
                printf("Saindo...\n");
                break;

            default: printf("Opcao invalida.\n");
        }
    }

    return 0;
}