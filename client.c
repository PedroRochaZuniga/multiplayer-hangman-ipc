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

// Inicalização dos dados de um jogador
    char palavra_agora[100] = "";
    char letras_usadas[27] = "";
    int erros_agora = 0;
    int meu_numero = 0;
    int minha_vez = 0;
    char recvBuffer[512];
    char sendBuffer[512];
    int bytesReceived;

// Enviando mensagem
// Recendo a mensagem
// Loop com socket do servidor
    while (1){
         bytesReceived = recv(clientSocket, recvBuffer,sizeof(recvBuffer),0);

    if (bytesReceived <= 0){
        printf("Servidor desconectou\n");
        break;
    }

    recvBuffer[bytesReceived] = '\0';

    // servidor liberou jogada
    if(strcmp(recvBuffer, "VOCE_JG1") == 0){
        meu_numero = 1;
        printf("Você é o jogador 1!\n");
        printf(" ");
    }

    else if(strcmp(recvBuffer, "VOCE_JG2") == 0){
        meu_numero = 2;
        printf("Você é o jogador 2!\n");
        printf(" ");
    }
    else if(strcmp(recvBuffer, "PALAVRA:") == 0){
        strcpy(palavra_agora, recvBuffer + 8);
        printf("Palavra atual: %s\n", palavra_agora);
    }
    else if(strcmp(recvBuffer, "USADAS:") == 0){
        strcpy(letras_usadas, recvBuffer + 7);
        if (strlen(letras_usadas) > 0){
            printf("Letras usadas: %s\n", letras_usadas);

        }}
    else if(strcmp(recvBuffer, "SUA_VEZ") == 0){
        minha_vez = 1;
        printf("-----------------------------------");
        printf(" Sua vez, Jogador %d!\n",meu_numero);
        printf("-----------------------------------\n");
        printf("[%dL] Para tentar uma letra (ex:%dLa)\n",meu_numero,meu_numero);
        printf("[%dC] Para chutar uma palavra (ex; [%dCpedrinho])\n",meu_numero,meu_numero);
        printf("-----------------------------------\n");
        printf(" ");
        printf("Digite sua ação: ");
        fflush(stdout);
        //Analisa a entrada agora
        char entrada[512];
        fgets(entrada,sizeof(entrada),stdin);
        entrada[strcspn(entrada,"\n")] = '\0'; // Remove o espaço em branco
        //Monta a ação (identificador + ação + dado)
        if (tolower((unsigned char)entrada[1]) == 'l' && entrada[2] != '\0') {
            char letra =tolower((unsigned char)entrada[2]);
            sprintf(sendBuffer,"%dL:%c", meu_numero,letra);
        } else if (tolower((unsigned char)entrada[1]) == 'c' && entrada[2] != '\0') {
            sprintf(sendBuffer,"%dC:%s", meu_numero,entrada + 1);
        } else {
            printf("[!] Formato invalido! Use %dL? para letra ou %dC????? para chutar!\n", meu_numero, meu_numero);
        }
        printf("[Enviando] %s...\n", sendBuffer);
        send(clientSocket,sendBuffer,(int)strlen(sendBuffer)+1,0);
        minha_vez = 0;
    }
    else if (strcmp(recvBuffer, "ESPERE") == 0) {
        printf("Aguardando a jogada do oponente... \n");
    }
        else if (strcmp(recvBuffer, "ACERTO") == 0) {
        printf("Boa! Letra correta! Você mantém a vez... \n");
    }
        else if (strcmp(recvBuffer, "ERRO") == 0) {
        printf("Putzzz! Letra errada! Vez do outro jogador... \n");
    }
        else if (strcmp(recvBuffer, "ACERTO_OPNENTE") == 0) {
        printf("Sério isso?! O oponente acertou uma letra! Não podemos deixar ele ganhar... \n");
    }
        else if (strcmp(recvBuffer, "ERRO_OPONENTE") == 0) {
        printf("Kkkkkk o oponente errou! Vai lá e mostre oq é capaz...\n");
    }
        else if (strcmp(recvBuffer, "LETRA_JA_USADA") == 0) {
        printf("Acho que já tentaram essa letra! Tente outra, você mantém a vez... \n");
    }
        else if (strcmp(recvBuffer, "CHUTE_CERTO") == 0) {
        printf("BOAAAA! ACERTOU A PALAVRA! \n");
    }
        else if (strcmp(recvBuffer, "CHUTE_ERRADO") == 0) {
        printf("NÃOOOO! VOCÊ ERROU O CHUTE :( perdeu 2 vidas... \n");
    }
        else if (strcmp(recvBuffer, "CHUTE_OPONENTE_ERRADO") == 0) {
        printf("KKKKKKK oponente tentou chutar e errou, bora ganhar agora... \n");
    }
        else if (strcmp(recvBuffer, "VITORIA:") == 0) {
        char vencedor[10];
        strcpy(vencedor,recvBuffer+8);
        printf("-----------------------------------");
        if (((meu_numero == 1 && strcmp(vencedor, "JG1")) == 0) || (meu_numero == 2 && strcmp(vencedor, "JG2") == 0)) {
            printf("PARABÉNSSSSS! VOCÊ VENCEU!");
        }
        else {
            printf("Você infelizmente perdeu :( o oponente completou a palavra...");
        }
        printf("Palavra: %s\n", palavra_agora);
        printf("-----------------------------------\n");
        break;

    } else if(strcmp(recvBuffer, "DERROTA:")==0){
        printf("-----------------------------------\n");
        printf("FIM DE JOGO! VOCÊS DOIS PERDERAM! Mais sorte na próxima...\n");
        printf("A palavra era: %s\n", recvBuffer + 8);
        break;
    } else if(strcmp(recvBuffer,"INVALIDO") == 0){
        printf("Comando inválido! Tente novamente...");
    }
}
printf("Pressione ENTER para sair...\n");
// Finalizando socket/biblioteca
    getchar();
// Usar client/server
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}