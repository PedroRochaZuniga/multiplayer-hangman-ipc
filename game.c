#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* esconder_palavra(char* palavra){
    int tamanho = strlen(palavra);
    char* palavra_oculta = malloc(tamanho + 1);
    for(int i = 0; i < tamanho; i++){
        palavra_oculta[i] = '_';

    }
    palavra_oculta[tamanho] = '\0';
    return palavra_oculta;

}


