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
    SOCKET clientSocket;
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    clientSocket = accept(sock,(struct sockaddr*)&clientAddr,&clientAddrLen);
    if (clientSocket == INVALID_SOCKET){
        printf("Erro ao aceitar a conexão: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    } else {printf("Conexao aceita com sucesso\n");}

//Fazendo o loop de mensagens
    char recvBuffer[512];
    char envioBuffer[512];
    while (1){
    int bytesReceived = recv(clientSocket,recvBuffer,sizeof(recvBuffer),0);
    if (bytesReceived == SOCKET_ERROR){
        printf("Erro ao receber dados: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }else if(bytesReceived == 0){
        printf("O cliente fechou a conexao\n");
        break;
    }
    else{
        recvBuffer[bytesReceived] = '\0'; //terminador nulo ao final dos dados recebidos
        printf("RECEBIDO: %s\n",recvBuffer);
        if (strcmp(recvBuffer,"sair")== 0){ // compara a mensagem com *sair*, se for, fecha o jogo
            printf("Cliente pediu para sair do jogo!");
            break;
        }
    }
    printf("Digite a mensagem para enviar ao cliente: ");
    fgets(envioBuffer,sizeof(envioBuffer),stdin);
    envioBuffer[strcspn(envioBuffer,"\n")] = 0; // troca o \n por \0

    int bytesSent = send(clientSocket,envioBuffer,strlen(envioBuffer),0);
    if (bytesSent == SOCKET_ERROR){
        printf("Erro ao enviar dados; %d!\n",WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    if (strcmp(envioBuffer,"sair") == 0){
        printf("Encerrando o jogo com o cliente\n");
        break;
    }
}

//usar client/server
    getchar();
//Fechar o socket do cliente
    closesocket(clientSocket);
//Fechar o socket criado
    closesocket(sock);
//Finalizar a biblioteca Winsock
    WSACleanup();
    return 0;
}





