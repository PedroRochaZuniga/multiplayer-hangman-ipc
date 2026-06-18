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
    if (jogador1 == INVALID_SOCKET){
        printf("Erro ao aceitar a conexão: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    } else {printf("Conexao aceita com sucesso\n");}
    //send(jogador1, "SUA_VEZ", 8, 0);
    //send(jogador2, "ESPERE", 7, 0);

//Fazendo o loop de mensagens
    char recvBuffer[512];
    char envioBuffer[512];
    int turno = 1;
    SOCKET atual = jogador1;
    char *palavra = "computador";
    char palavra_secreta[100];
    for (int i = 0; i < strlen(palavra); i++){
        palavra_secreta[i] = '_';
    }
    palavra_secreta[strlen(palavra)] = '\0';

    while (1){
    send(atual, "SUA_VEZ", 8, 0);
    if (atual == jogador1){
        send(jogador2, "ESPERE", 7, 0);
    }
    else{
        send(jogador1, "ESPERE", 7, 0);
    }
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
        }
    }
    //int bytesSent = send(atual,palavra_secreta,strlen(palavra_secreta),0);
    int bytesSent = send(jogador1,palavra_secreta,strlen(palavra_secreta),0);
    int bytesSent = send(jogador2,palavra_secreta,strlen(palavra_secreta),0);
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






