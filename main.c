#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INSERIR_UM_A_UM 1
#define INSERIR_TODOS 2
#define PESQUISAR_POR_CHAVE_PRIMARIA 3
#define PESQUISAR_POR_CHAVE_SECUNDARIA 4

#define TAM_MAX 32

typedef struct segurados {
    char cod_cli[4];
    char nome_cli[50];
    char nome_seg[50];
    char tipo_seg[30];
} seg;

typedef struct indice {
    char cod_cli[4];
    int pos;
} INDICE;

typedef struct {
    char nome_seg[50];
    int pos;
} INDICE_SECUNDARIO;

typedef struct busca_primaria {
    char cod_cli[4];
} BUSCA_P;

//Função para escrever o índice no arquivo de índices
void escrever_no_arquivo_indice(FILE * indice_fp, INDICE * indice) {
    FILE * aux = fopen("indice_temp.dad", "wb");
    for (int i = 0; i < TAM_MAX; i++) {
        if (strlen(indice[i].cod_cli) == 0) break;

        fwrite(indice[i].cod_cli, sizeof(char), 3, aux);
        fwrite(&indice[i].pos, sizeof(int), 1, aux);
    }
    remove("indice.dad");
    rename("indice_temp.dad", "indice.dad");
    fclose(aux);
    indice_fp = fopen("indice.dad", "r+b");
    return;
}

// Função para inserir um registro no vetor de índices
void inserir_no_indice(INDICE * indice, seg segurado, int byteoffset) {
    int posicao = -1;
    for (int i = 0; i < TAM_MAX; i++) {
        // se o cod_cli for menor
        if (strcmp(segurado.cod_cli, indice[i].cod_cli) < 0) {
            posicao = i;
            break;
        }
    }

    if (posicao == -1) {
        for (int i = 0; i < TAM_MAX; i++) {
            if (strlen(indice[i].cod_cli) == 0) {
                posicao = i;
                break;
            }
        }
        strcpy(indice[posicao].cod_cli, segurado.cod_cli);
        indice[posicao].pos = byteoffset;
        return;
    }

    INDICE swap = indice[posicao];
    for (int i = posicao; i < TAM_MAX; i++) {
        INDICE swap2 = indice[i + 1];
        indice[i + 1] = swap;
        swap = swap2;
    }

    strcpy(indice[posicao].cod_cli, segurado.cod_cli);
    indice[posicao].pos = byteoffset;

    return;
}

// Função para ler o arquivo de índices e armazenar em um vetor de structs
INDICE * ler_indice(FILE * indice_fp) {
    INDICE * indice = (INDICE*) malloc(sizeof(INDICE) * TAM_MAX);
    int cont = 0;
    while (!feof(indice_fp)) {
        int posicao_atual = ftell(indice_fp);
        fseek(indice_fp, 0, SEEK_END);
        if (posicao_atual == ftell(indice_fp)) break;

        fseek(indice_fp, posicao_atual, SEEK_SET);

        char cod_cli[4];
        int pos;
        fread(cod_cli, sizeof(char), 3, indice_fp);
        fread(&pos, sizeof(int), 1, indice_fp);

        strcpy(indice[cont].cod_cli, cod_cli);
        indice[cont].pos = pos;
        cont++;
    }

    return indice;
}

void inserir_no_indice_secundario(INDICE_SECUNDARIO * indice_secundario, seg registro, FILE * indice_secundario_lista_fp) {
    int ja_existe_seguradora = 0;
    int offset = -1;
    for (int i = 0; i < TAM_MAX; i++) {
        if (strlen(indice_secundario[i].nome_seg) == 0) break;

        if (strcmp(indice_secundario[i].nome_seg, registro.nome_seg) == 0) {
            ja_existe_seguradora = 1;
            offset = indice_secundario[i].pos;
            break;
        }
    }

    fseek(indice_secundario_lista_fp, 0, SEEK_END);
    int novo_offset = ftell(indice_secundario_lista_fp);

    fwrite(registro.cod_cli, sizeof(char), 3, indice_secundario_lista_fp);
    fwrite(&offset, sizeof(int), 1, indice_secundario_lista_fp);
    fflush(indice_secundario_lista_fp);

    if (ja_existe_seguradora) {
        // alterar o offset no indice secundario
        for (int i = 0; i < TAM_MAX; i++) {
            if (strcmp(indice_secundario[i].nome_seg, registro.nome_seg) == 0) {
                indice_secundario[i].pos = novo_offset;
                break;
            }
        }
        return;
    }

    int posicao = -1;
    for (int i = 0; i < TAM_MAX; i++) {
        if (strcmp(registro.nome_seg, indice_secundario[i].nome_seg) < 0 || strlen(indice_secundario[i].nome_seg) == 0) {
            posicao = i;
            break;
        }
    }

    INDICE_SECUNDARIO swap = indice_secundario[posicao];
    for (int i = posicao; i < TAM_MAX; i++) {
        INDICE_SECUNDARIO swap2 = indice_secundario[i + 1];
        indice_secundario[i + 1] = swap;
        swap = swap2;
    }

    strcpy(indice_secundario[posicao].nome_seg, registro.nome_seg);
    indice_secundario[posicao].pos = novo_offset;
}

INDICE_SECUNDARIO * criar_indice_secundario(FILE * arquivo_dados, FILE * indice_secundario_fp, FILE * indice_secundario_lista_fp) {
    INDICE_SECUNDARIO * indice_secundario = (INDICE_SECUNDARIO*) malloc(sizeof(INDICE_SECUNDARIO) * TAM_MAX);

    indice_secundario_lista_fp = fopen("indice_secundario_lista.dad", "w+b");

    for (int i = 0; i < TAM_MAX; i++) {
        strcpy(indice_secundario[i].nome_seg, "");
        indice_secundario[i].pos = -1;
    }

    while (1) {
        int posicao_atual = ftell(arquivo_dados);
        fseek(arquivo_dados, 0, SEEK_END);
        if (posicao_atual == ftell(arquivo_dados)) break;
        fseek(arquivo_dados, posicao_atual, SEEK_SET);

        unsigned char tam_buffer;
        char buffer[135];

        fread(&tam_buffer, sizeof(char), 1, arquivo_dados);
        fread(buffer, sizeof(char), tam_buffer, arquivo_dados);

        char * codigo_cli = strtok(buffer, "#");
        char * nome_cli = strtok(NULL, "#");
        char * seguradora = strtok(NULL, "#");

        seg seg;
        strcpy(seg.cod_cli, codigo_cli);
        strcpy(seg.nome_cli, nome_cli);
        strcpy(seg.nome_seg, seguradora);

        inserir_no_indice_secundario(indice_secundario, seg, indice_secundario_lista_fp);
    }

    return indice_secundario;
}

void escrever_no_arquivo_indice_secundario(FILE * indice_secundario_fp, INDICE_SECUNDARIO * indice) {
    FILE * aux = fopen("indice_secundario_temp.dad", "wb");
    for (int i = 0; i < TAM_MAX; i++) {
        if (strlen(indice[i].nome_seg) == 0) break;

        unsigned char tam_buffer = strlen(indice[i].nome_seg);

        fwrite(&indice[i].pos, sizeof(int), 1, aux);
        fwrite(&tam_buffer, sizeof(char), 1, aux);
        fwrite(indice[i].nome_seg, sizeof(char), strlen(indice[i].nome_seg), aux);
    }

    remove("indice_secundario.dad");
    rename("indice_secundario_temp.dad", "indice_secundario.dad");
    fclose(aux);
    indice_secundario_fp = fopen("indice_secundario.dad", "r+b");
    return;
}

void pesquisar_por_chave_primaria(char * chave_p, FILE * fp, INDICE * indice) { 
    for (int i = 0; i < TAM_MAX; i++) {
        if (strcmp(indice[i].cod_cli, chave_p) == 0) {
            int byteoffset = indice[i].pos;

            unsigned char tamanho_buffer;
            char buffer[135];

            fseek(fp, byteoffset, SEEK_SET);

            fread(&tamanho_buffer, sizeof(char), 1, fp);
            fread(buffer, sizeof(char), tamanho_buffer, fp);

            printf("%s\n", buffer);

            break;
        }
    }
}

int main() {
    //Abertura arquivo principal dos segurados 
    FILE * fp = fopen("seguradoras.dad", "r+b");
    if(fp == NULL) fp = fopen("seguradoras.dad", "w+b");

    //Abertura arquivo de índices
    FILE * indice_fp = fopen("indice.dad", "r+b");
    if(indice_fp == NULL) indice_fp = fopen("indice.dad", "w+b");

    FILE * indice_secundario_fp = fopen("indice_secundario.dad", "r+b");
    if(indice_secundario_fp == NULL) indice_secundario_fp = fopen("indice_secundario.dad", "w+b");

    FILE * indice_secundario_lista_fp = fopen("indice_secundario_lista.dad", "r+b");
    if(indice_secundario_lista_fp == NULL) indice_secundario_lista_fp = fopen("indice_secundario_lista.dad", "w+b");

    //Declaração das variáveis dos registros
    char codigo[4];
    char nome[50], seguradora[50], tipo_seg[30], buffer_aux[135];

    //Abertura do arquivo com dados para inserção
    FILE * fp2 = fopen("insere.bin", "rb");

    if(fp2 == NULL) {
        printf("Arquivo de inserção não encontrado.\n");
        return 0;
    }

    seg segurado[32];
    int i = 0;

    INDICE * indice = ler_indice(indice_fp);
    INDICE_SECUNDARIO * indice_secundario = criar_indice_secundario(fp, indice_secundario_fp, indice_secundario_lista_fp);
    escrever_no_arquivo_indice_secundario(indice_secundario_fp, indice_secundario);

    //Leitura dos dados do arquivo de inserção e armazenamento em um vetor de structs
    while(fread(codigo, sizeof(char), 4, fp2) == 4 &&
        fread(nome, sizeof(char), 50, fp2) == 50 &&
        fread(seguradora, sizeof(char), 50, fp2) == 50 &&
        fread(tipo_seg, sizeof(char), 30, fp2) == 30) {

        strcpy(segurado[i].cod_cli, codigo);
        strcpy(segurado[i].nome_cli, nome);
        strcpy(segurado[i].nome_seg, seguradora);
        strcpy(segurado[i].tipo_seg, tipo_seg);

        i++;
    }

    int aux_contador = i;

    fclose(fp2);

    BUSCA_P busca[TAM_MAX];
    char cod_bp[4];
    int k = 0;

    //Abertura do arquivo com os índices para busca primária
    FILE * busca_primaria = fopen("busca_p.bin", "rb");

    if(busca_primaria == NULL) {
        printf("Arquivo de índices para busca primária não encontrado.\n");
        return 0;
    }

    //Leitura dos índices para busca primária e armazenamento em um vetor de structs
    while(fread(cod_bp, sizeof(char), 4, busca_primaria) == 4) {
        strcpy(busca[k].cod_cli, cod_bp);
        k++;
    }

    //Arquivo .txt auxiliar para impedir que um índice seja ocupado mais de uma vez
    FILE *aux = fopen("auxiliar.txt", "r+");

    if (aux == NULL) {
        aux = fopen("auxiliar.txt", "w+");
        printf("Arquivo auxiliar criado.\n");
        for (int i = 0; i < 32; i++) {
            fprintf(aux, "%d", -1);
        }
    } else {
        printf("Arquivo auxiliar aberto com sucesso.\n");
    }

    //Variável para escolher a função, começa com -1 para entrar no loop
    int funcao = -1;

    //Flag para verificar se a inserção foi feita um a um ou todos de uma vez
    int flag_tipo_de_insercao = -1;

    //Menu de funções
    while(funcao != 0) {

        //Sair 0
        //Inserção UM A UM 1
        //Inserção em TODOS 2
        //Pesquisa por CHAVE PRIMÁRIA 3
        printf("Escolha uma função\n");
        scanf("%d", &funcao);

        //Insere um a um os dados do vetor de structs no arquivo principal
        //e no arquivo de índices
        if (funcao == INSERIR_UM_A_UM) {

            printf("Escolha um índice para inserir:\n");
            int index;
            scanf("%d", &index);

            if(index < 1 || index > 32) {
                printf("Índice inválido.\n");
                return 0;
            }
            index--;

            fseek(aux, index * sizeof(int), SEEK_SET);
            int flag = -1;
            fread(&flag, sizeof(int), 1, aux);

            if(flag == 1) {
                printf("Índice já ocupado.\n");
                return 0;
            }

            fseek(aux, -sizeof(int), SEEK_CUR);
            int flag2 = 1;
            fwrite(&flag2, sizeof(int), 1, aux);

            strcpy(buffer_aux, "");

            fseek(fp, 0, SEEK_END);

            int pos_arquivo_principal = ftell(fp);

            sprintf(buffer_aux, "%s#%s#%s#%s", segurado[index].cod_cli, segurado[index].nome_cli,
            segurado[index].nome_seg, segurado[index].tipo_seg);
            int tam_buff = strlen(buffer_aux);
            unsigned char tam_buff_char = tam_buff;
            fseek(fp, 0, SEEK_END);
            fwrite(&tam_buff_char, sizeof(char), 1, fp);
            fwrite(buffer_aux, sizeof(char), tam_buff, fp);
            fflush(fp);

            inserir_no_indice(indice, segurado[index], pos_arquivo_principal);
            inserir_no_indice_secundario(indice_secundario, segurado[index], indice_secundario_lista_fp);
            escrever_no_arquivo_indice(indice_fp, indice);
            escrever_no_arquivo_indice_secundario(indice_secundario_fp, indice_secundario);

            flag_tipo_de_insercao = 1;
        }

        //Insere todos os dados do vetor de structs no arquivo principal 
        //e no arquivo de índices 
        //OBS: O uso de INSERIR_TODOS mais de uma vez pode causar erros
        //OBS: Usar INSERIR_UM_A_UM na execução implica em não usar INSERIR_TODOS
        else if(funcao == INSERIR_TODOS) {

            if(flag_tipo_de_insercao == 1) {
                printf("Não é possível inserir todos os dados de uma vez após já ter inserido pelo menos um separadamente.\n");
                return 0;
            }

            for (int j = 0; j < aux_contador; j++) {
                int pos_arquivo_principal = ftell(fp);
                sprintf(buffer_aux, "%s#%s#%s#%s", segurado[j].cod_cli, segurado[j].nome_cli,
                segurado[j].nome_seg, segurado[j].tipo_seg);
                int tam_buff = strlen(buffer_aux);
                unsigned char tam_buff_char = tam_buff;
                fwrite(&tam_buff_char, sizeof(char), 1, fp);
                fwrite(buffer_aux, sizeof(char), tam_buff, fp);
                inserir_no_indice(indice, segurado[j], pos_arquivo_principal);
                inserir_no_indice_secundario(indice_secundario, segurado[j], indice_secundario_lista_fp);
                escrever_no_arquivo_indice(indice_fp, indice);
                escrever_no_arquivo_indice_secundario(indice_secundario_fp, indice_secundario);
            }
        }

        else if (funcao == PESQUISAR_POR_CHAVE_PRIMARIA) {
            char chave_p[4];
            printf("Digite a chave primária:\n");
            scanf("%s", chave_p);

            for (int i = 0; i < TAM_MAX; i++) {

                if (strcmp(indice[i].cod_cli, chave_p) == 0) {
                    printf("Chave primária encontrada.\n");
                    int byteoffset = indice[i].pos;

                    unsigned char tamanho_buffer;
                    char buffer[135];

                    fseek(fp, byteoffset, SEEK_SET);

                    fread(&tamanho_buffer, sizeof(char), 1, fp);
                    fread(buffer, sizeof(char), tamanho_buffer, fp);

                    printf("%s\n", buffer);

                    break;
                }
            }
            // BUSCA_P busca_p[TAM_MAX];
            // BUSCA_P busca_p_utilizados[TAM_MAX];
            // int i = 0;

            // FILE * busca_primaria_fp = fopen("busca_p.bin", "rb");
            // FILE * busca_primaria_utilizados_fp = fopen("busca_p_utilizados.bin", "r+b");

            // while (!feof(busca_primaria_utilizados_fp)) {
            //     printf("i: %d\n", i);
            //     fread(busca_p_utilizados[i++].cod_cli, sizeof(char), 4, busca_primaria_fp);
            // }

            // i = 0;
            // while (!feof(busca_primaria_fp)) {
            //     char aux[4];
            //     fread(aux, sizeof(char), 4, busca_primaria_fp);

            //     int ja_usado = 0;
            //     for (int j = 0; j < TAM_MAX; j++) {
            //         if (strcmp(busca_p_utilizados[j].cod_cli, aux) == 0) {
            //             ja_usado = 1;
            //             break;
            //         }
            //     }

            //     if (ja_usado) continue;

            //     strcpy(busca_p[i++].cod_cli, aux);
            // }

            // for (int i = 0; i < TAM_MAX; i++) {
            //     printf("%s\n", busca_p[i].cod_cli);
            // }

            // char resposta;
            // i = 0;
            // while (1) {
            //     printf("Ler próximo? [s/n]\n");
            //     scanf("%c", &resposta);

            //     if (resposta == 'n') break;

            //     fwrite(busca_p[i].cod_cli, sizeof(char), 4, busca_primaria_utilizados_fp);

            //     pesquisar_por_chave_primaria(busca_p[i++].cod_cli, fp, indice);

            // }
            // fclose(busca_primaria_fp);
            // fclose(busca_primaria_utilizados_fp);
        }
        else if (funcao == PESQUISAR_POR_CHAVE_SECUNDARIA) {
            char seguradora[50];
            printf("Digite a seguradora:\n");
            scanf("%s", seguradora);

            for (int i = 0; i < TAM_MAX; i++) {
                if (strcmp(indice_secundario[i].nome_seg, seguradora) == 0) {
                    int byteoffset = indice_secundario[i].pos;

                    fseek(indice_secundario_lista_fp, byteoffset, SEEK_SET);

                    char cod_cli[4];
                    int prox = 0;

                    while (prox != -1) {
                        fread(cod_cli, sizeof(char), 3, indice_secundario_lista_fp);
                        fread(&prox, sizeof(int), 1, indice_secundario_lista_fp);

                        pesquisar_por_chave_primaria(cod_cli, fp, indice);
                        fseek(indice_secundario_lista_fp, prox, SEEK_SET);
                    }

                    break;
                }
            }
        }
    }

    fclose(indice_fp);
    fclose(aux);
    fclose(fp);
    fclose(busca_primaria);
    fclose(indice_secundario_fp);
    fclose(indice_secundario_lista_fp);
}