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

//Inicializar a bibliteca
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
        WSACleanup;
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
    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    clientSocket = accept(sock,(struct sockaddr*)&clientAddr,&clientAddrLen);
    if (clientSocket == INVALID_SOCKET){
        printf("Erro ao aceitar a conexão: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    } else {printf("Conexão aceita com sucesso\n");}

//imprimir a mensagem
    char recvBuffer[512];
    int bytesReceived = recv(clientSocket,recvBuffer,sizeof(recvBuffer),0);
    if (bytesReceived == SOCKET_ERROR){
        printf("Erro ao receber dados: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    else{
        recvBuffer[bytesReceived] = '\0'; //terminador nulo ao final dos dados recebidos
        printf("RECEBIDO %d bytes do servidor: %s\n",bytesReceived,recvBuffer);
    }

//usar client/server

    getchar();
//Fechar o socket criado
    closesocket(sock);
//Finalizar a biblioteca Winsock
    WSACleanup();
    return 0;
}





