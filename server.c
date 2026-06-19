#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")
#define MAX_ERROS 6
#define TAM_BUFFER 512

// RODAR NO TERMINAL
//    gcc server.c -o server -lws2_32
//    gcc client.c -o client -lws2_32
//    ./server
//    ./client

//Funções principais
// WSAStartup - Inicializa a biblioteca Winsock
// WSACleanup - Encerra o uso da biblioteca Winsock
// socket - Cria um novo socket
// closesocket - Fecha um socket
// bind - Associa um endereço de IP e um número de porta a um socket
// listen - Coloca o socket em modo de escuta para conexões de entrada
// accept - Aceita uma conexão de entrada
// send - Envia dados através de um socket conectado
// recv - Recebe dados de um socket conectado
// sendto - Envia dados através d eum socket sem conexão UDP
// recvfrom - Recebe dados de um socket sem conexão
// connect - Estabelece conexão com um socket remoto
// shutdown - Encerra a transmissão em um ou em ambos os sentidos do socket
// setsockopt - Configura as opções d eum socket
// getsockopt - Obtém opções de um socket
// gethostbyname – Obtém informações sobre um host a partir do nome
// gethostbynaddr – Obtém informações sobre um host a partir do end de IP
// gethostname - Obtém o nome do host local
// getaddrinfo - Obtém informações deum endereço para um nome ou serviço



// Palavras para o jogo!
const char *palavras[] = {"computador", "operacional", "clusters", "processo", "deadlock", "sistema", "memoria", "registrador", "grafos"};

//Estrutura compartilhada para os jogadores
typedef struct{
    const char *palavra;
    char palavra_secreta[100];
    char letras_usadas[27];
    int num_letras_usadas;
    int erros;
    int turno;
    int jogo_ativo;
} Estadojogo;

//Função geral que envia a mensagem pros dois jogadores
void broadcast(SOCKET jogador1, SOCKET jogador2, const char *msg){
    send(jogador1,msg,(int)strlen(msg) + 1, 0);
    send(jogador2,msg,(int)strlen(msg) + 1, 0);
}

//Função que envia mensagens para ambos os jogadores com as informações mais importantes
void enviar_descobertas(SOCKET jogador1, SOCKET jogador2, Estadojogo *estado){
    char buffer[512];

    //Envia a palavra secreta
    sprintf(buffer,"Palavra: %s\n",estado->palavra_secreta);
    broadcast(jogador1,jogador2,buffer);

    //Envia a quantidade de erros
    sprintf(buffer,"Erros: %d/%d\n",estado->erros,MAX_ERROS);
    broadcast(jogador1,jogador2,buffer);

    //Envia as letras já usadas
    sprintf(buffer,"Letras já usadas: %s\n",estado->letras_usadas);
    broadcast(jogador1,jogador2,buffer);
}

//Função que verifica se a palavra secreta não possui mais *_*, ou seja, foi descoberta
int palavra_descoberta(Estadojogo *estado){
    return strchr(estado->palavra_secreta,"_") == NULL;
}


//Função de processar letra, laço de repetição na palavra originl, se encontrou uma letra igual
//troca essa letra na mesma posição, mas na palavra secreta (cheia de _), caso não encontre, adiciona um erro
int processar_letra(Estadojogo *estado, char letra){
    letra = tolower((unsigned char)letra);
    for (int i = 0; i < estado->num_letras_usadas; i++){
        if (estado->letras_usadas[i] == letra) return -1;
        }
    estado->letras_usadas[estado->num_letras_usadas++] = letra;
    estado->letras_usadas[estado->num_letras_usadas] = '\0';

    int acertou = 0;
    for (int i = 0; i < (int)strlen(estado->palavra);i++){
        if (estado->palavra[i] == letra){
            estado->palavra_secreta[i] = letra;
            acertou = 1;
        }
    }
    if (!acertou) estado->erros++;
    return acertou;
    }


//Função de processar chute, antes coloca todo o chute em minúsculo, e compara o chute com a palavra original, se acertou retorna 1,
//se errou retorna 0 e adicionam 2 erros, como penalidade, ao invés de 1
int processa_chute(Estadojogo *estado, const char *chute){
    char chute_lower[100];
    strcpy(chute_lower,chute);
    for (int i = 0; chute_lower[i]; i++){
        chute_lower[i] = tolower((unsigned char)chute_lower[i]);
        return 1;
    }
    if (strcmp(chute_lower,estado->palavra) == 0){
        return 1;
    }
    estado->erros += 2;
    return 0;
}


int main(void)
{
//Ativar a biblioteca
    WSADATA winsocketsDados;
    int temp;
    temp = WSAStartup(MAKEWORD(2,2),&winsocketsDados);
    if(temp != 0)
    {
        printf("WSAStratup falhou: %d\n", temp);
        return 1;

    }
    else {printf("WSAStratup carregado com sucesso\n");}
    
//Criar o socket
    SOCKET sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (sock ==INVALID_SOCKET){
        printf("Errro ao criar o socket: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    else {printf("Socket Criado com sucesso\n");}

//Bind no socket
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; //associa a qualquer endereço de ip
    server.sin_port = htons(51171); //porta de ip
    if (bind(sock,(struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR){
        printf("Erro ao associar o socket: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup();
        return 1;
    }
    else{printf("Bind realizado com sucesso\n");}

//Modo escuta
//somaxconn = numero maximo de conexoes
    if (listen(sock,SOMAXCONN) == SOCKET_ERROR){
        printf("Erro ao colocar o socket em estado de escuta: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    else {printf("Listen realizado com sucesso\n");}

//aceitar conexao
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    SOCKET jogador1 = accept(sock, (struct sockaddr*)&clientAddr,&clientAddrLen);
    SOCKET jogador2 = accept(sock, (struct sockaddr*)&clientAddr,&clientAddrLen);
    if (jogador1 == INVALID_SOCKET || jogador2 == INVALID_SOCKET){
        printf("Erro ao aceitar a conexão: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    } else {printf("Conexao aceita com sucesso\n");}
    send(jogador1, "SUA_VEZ", 8, 0);
    send(jogador2, "ESPERE", 7, 0);

//Fazendo o loop de mensagens
    char recvBuffer[512];
    char envioBuffer[512];
    int turno = 1;
    SOCKET atual = jogador1;
    char *palavra = "pedrinhe";
    char palavra_secreta[100];
    for (int i = 0; i < strlen(palavra); i++){
        palavra_secreta[i] = '_';
    }
    palavra_secreta[strlen(palavra)] = '\0';

    while (1){
    int bytesReceived = recv(atual,recvBuffer,sizeof(recvBuffer),0);
    if (bytesReceived == SOCKET_ERROR){
        printf("Erro ao receber dados: %d\n", WSAGetLastError());
        closesocket(atual);
        WSACleanup();
        return 1;
    }else if(bytesReceived == 0){
        printf("O cliente fechou a conexao\n");
        break;
    }
    else{
        recvBuffer[bytesReceived] = '\0'; //terminador nulo ao final dos dados recebidos
        printf("RECEBIDO: %s\n",recvBuffer);
        printf(" ");
        char letra = recvBuffer[1];
        for (int i = 0; i< strlen(palavra);i++){ if(palavra[i]==letra){ palavra_secreta[i]=letra;}}
        printf("Palavra: %s\n",palavra_secreta);
        if (strcmp(recvBuffer,"sair")== 0){ // compara a mensagem com *sair*, se for, fecha o jogo
            printf("Cliente pediu para sair do jogo!");
            break;
        }
    }
    //int bytesSent = send(atual,palavra_secreta,strlen(palavra_secreta),0);
    int bytesSent = send(jogador1,palavra_secreta,strlen(palavra_secreta),0);
    bytesSent = send(jogador2,palavra_secreta,strlen(palavra_secreta),0);
    if (bytesSent == SOCKET_ERROR){
        printf("Erro ao enviar palavra; %d!\n",WSAGetLastError());
        closesocket(atual);
        WSACleanup();
        return 1;
    }
    if (turno == 1) turno = 2;
    else turno = 1;

    if (turno == 1) atual = jogador1;
    else atual = jogador2;
}

//usar client/server
    getchar();
//Fechar o socket do cliente
    closesocket(atual);
//Fechar o socket criado
    closesocket(sock);
//Finalizar a biblioteca Winsock
    WSACleanup();
    return 0;
}





