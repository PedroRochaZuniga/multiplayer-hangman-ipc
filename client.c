#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock.h>

//O socket é o meio de comunicação entre o cliente e o servidor, como se fosse um telefone

//Inicializar a biblioteca
int main(void){
    WSADATA winsocketsDados;
    if (WSAStartup(MAKEWORD(2,2),&winsocketsDados)!= 0){
        printf("Falha ao inicializar o Winsock\n");
        return 1;
    }
    else{printf("WSAStratup carregado com sucesso\n");}

//Criando socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET){
        printf("Erro ao criar o socket: %d\n",GetLastError());
        WSACleanup();
        return 1;
    }
    else{printf("Socket criado com sucesso\n");}


// Conectando a servidor
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //Endereço de ip do servidor
    serverAddr.sin_port = htons(51171); //Porta do servidor
    if (connect(clientSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) == SOCKET_ERROR){
        printf("Erro ao concectar ao servidor: %d\n",WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    else{printf("Conectando ao servidor\n");}

// Enviando mensagem
// Recendo a mensagem
// Loop com socket do servidor

    char texto[512];
    char recvBuffer[512];
    int bytesReceived;

    while (1){
         bytesReceived = recv(clientSocket, recvBuffer,sizeof(recvBuffer),0);

    if (bytesReceived <= 0){
        printf("Servidor desconectou\n");
        break;
    }

    recvBuffer[bytesReceived] = '\0';

    // servidor liberou jogada
    if(strcmp(recvBuffer, "SUA_VEZ") == 0){

        printf("\n=== SUA VEZ ===\n");

        printf("Digite uma letra: ");
        fgets(texto,sizeof(texto),stdin);

        texto[strcspn(texto,"\n")] = 0;

        send(clientSocket, texto, strlen(texto),0);
    }

    // servidor mandou esperar
    else if(strcmp(recvBuffer, "ESPERE") == 0){

        printf("Aguardando outro jogador...\n");
    }

    // palavra atualizada
    else{

        printf("Palavra: %s\n",recvBuffer);
    }
    printf("Digite uma letra: ");
    fgets(texto,sizeof(texto),stdin);
    texto[strcspn(texto,"\n")] = 0; // Encontra o \n da mensagem e troca por \0
    int bytesSent = send(clientSocket, texto, strlen(texto),0);
    if (bytesSent == SOCKET_ERROR){
        printf("Erro ao enviar dados: %d\n",WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }if (strcmp(texto,"sair")== 0){
        printf("Encerrando a conexao com os servidor!\n");
        break;
    }
    bytesReceived = recv(clientSocket, recvBuffer,sizeof(recvBuffer),0);
    if (bytesReceived == SOCKET_ERROR){
        printf("Erro ao receber dados: %d\n",WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    else if (bytesReceived == 0){
        printf("O servidor fechou o jogo!\n");
        break;
    }
// Finalizando socket/biblioteca
    getchar();
// Usar client/server
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}}