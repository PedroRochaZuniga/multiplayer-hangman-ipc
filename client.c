#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <ctype.h>
#include <time.h>

//O socket é o meio de comunicação entre o cliente e o servidor, como se fosse um telefone

// Imprime uma linha separadora para delimitar cada interação no terminal
void separador() {
    printf("\n========================================\n");
}

// Desenha a forca ASCII de acordo com o número de erros (0 a 6)
// Cada estágio adiciona uma parte do boneco: cabeça, corpo, braço esq, braço dir, perna esq, perna dir
void desenhar_forca(int erros) {
    printf("  +---+\n");
    printf("  |   |\n");
    printf("  %s   |\n",                      erros >= 1 ? "O" : " ");
    printf(" %s%s%s  |\n",  erros >= 3 ? "/" : " ",
                            erros >= 2 ? "|" : " ",
                            erros >= 4 ? "\\" : " ");
    printf(" %s %s  |\n",  erros >= 5 ? "/" : " ",
                            erros >= 6 ? "\\" : " ");
    printf("       |\n");
    printf("=========\n");
}


int main(void){
    WSADATA winsocketsDados;
    if (WSAStartup(MAKEWORD(2,2),&winsocketsDados)!= 0){
        printf("Falha ao inicializar o Winsock\n");
        return 1;
    }

//Criando socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET){
        printf("Erro ao criar o socket: %d\n",GetLastError());
        WSACleanup();
        return 1;
    }

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
    separador();
    printf("   BEM-VINDO AO JOGO DA FORCA!    \n");
    printf("   Conectado! Aguardando jogador 2...\n");

// Inicalização dos dados de um jogador
    char palavra_agora[100] = "";
    char letras_usadas[27]  = "";
    int  erros_agora        = 0;
    int  meu_numero         = 0;
    int  minha_vez          = 0;
    char recvBuffer[512];
    char sendBuffer[512];
    int  bytesReceived;

// Enviando mensagem
// Recendo a mensagem
// Loop com socket do servidor
    while (1){
        bytesReceived = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);

        if (bytesReceived <= 0){
            printf("Servidor desconectou\n");
            break;
        }
        recvBuffer[bytesReceived] = '\0';

        // Servidor informou qual jogador somos
        if(strcmp(recvBuffer, "VOCE_JG1") == 0){
            meu_numero = 1;
            separador();
            printf("   Voce eh o JOGADOR 1!\n");
        }
        else if(strcmp(recvBuffer, "VOCE_JG2") == 0){
            meu_numero = 2;
            separador();
            printf("   Voce eh o JOGADOR 2!\n");
        }

        // Atualiza e exibe a palavra secreta
        else if(strncmp(recvBuffer, "Palavra: ", 9) == 0){
            strcpy(palavra_agora, recvBuffer + 9);
            palavra_agora[strcspn(palavra_agora, "\n")] = '\0';
        }

        // Atualiza erros e exibe a forca junto com o estado atual do jogo
        else if(strncmp(recvBuffer, "Erros:", 6) == 0){
            sscanf(recvBuffer + 7, "%d", &erros_agora);
        }

        // Letras usadas é sempre a última das 3 mensagens de estado enviadas pelo servidor,
        // então é aqui que exibimos o estado completo de uma vez
        else if(strncmp(recvBuffer, "Letras ja usadas: ", 18) == 0){
            strcpy(letras_usadas, recvBuffer + 18);
            letras_usadas[strcspn(letras_usadas, "\n")] = '\0';
            separador();
            desenhar_forca(erros_agora);
            printf("\n  Palavra : %s\n", palavra_agora);
            printf("  Erros   : %d/6\n", erros_agora);
            if (strlen(letras_usadas) > 0)
                printf("  Letras  : %s\n", letras_usadas);
            else
                printf("  Letras  : (nenhuma ainda)\n");
        }

        // Servidor liberou a vez deste jogador
        else if(strcmp(recvBuffer, "SUA_VEZ") == 0){
            minha_vez = 1;
            separador();
            printf("   SUA VEZ, JOGADOR %d!\n", meu_numero);
            printf("----------------------------------------\n");
            printf("  [%dL<letra>]    Tentar letra  (ex: %dLa)\n", meu_numero, meu_numero);
            printf("  [%dC<palavra>]  Chutar palavra (ex: %dCgrafos)\n", meu_numero, meu_numero);
            printf("----------------------------------------\n");
            printf("  > ");
            fflush(stdout);

            // Lê a entrada do jogador
            char entrada[512];
            fgets(entrada, sizeof(entrada), stdin);
            entrada[strcspn(entrada, "\n")] = '\0'; // Remove o \n do final

            // Monta a mensagem no formato: <numero_jogador><L|C><dado>
            if (tolower((unsigned char)entrada[1]) == 'l' && entrada[2] != '\0') {
                char letra = tolower((unsigned char)entrada[2]);
                sprintf(sendBuffer, "%dL%c", meu_numero, letra);
            } else if (tolower((unsigned char)entrada[1]) == 'c' && entrada[2] != '\0') {
                // entrada+2 pula o prefixo "1C" e envia só a palavra do chute
                sprintf(sendBuffer, "%dC%s", meu_numero, entrada + 2);
            } else {
                printf("  [!] Formato invalido! Use %dL<letra> ou %dC<palavra>\n", meu_numero, meu_numero);
                // Envia mensagem inválida para o servidor tratar e não travar o fluxo
                sprintf(sendBuffer, "%dX", meu_numero);
            }

            send(clientSocket, sendBuffer, (int)strlen(sendBuffer) + 1, 0);
            minha_vez = 0;
        }

        else if (strcmp(recvBuffer, "ESPERE") == 0) {
            separador();
            printf("   Aguardando jogada do oponente...\n");
        }

        // Respostas de resultado de jogada
        else if (strcmp(recvBuffer, "ACERTO") == 0) {
            printf("  [+] Letra correta! Voce mantem a vez!\n");
        }
        else if (strcmp(recvBuffer, "ERRO") == 0) {
            printf("  [-] Letra errada! Vez do oponente...\n");
        }
        else if (strcmp(recvBuffer, "ACERTO_OPNENTE") == 0) {
            printf("  [+] O oponente acertou uma letra!\n");
        }
        else if (strcmp(recvBuffer, "ERRO_OPONENTE") == 0) {
            printf("  [-] O oponente errou! Agora e sua vez!\n");
        }
        else if (strcmp(recvBuffer, "LETRA_JA_USADA") == 0) {
            printf("  [!] Essa letra ja foi tentada! Voce mantem a vez.\n");
        }
        else if (strcmp(recvBuffer, "CHUTE_CERTO") == 0) {
            printf("  [+] VOCE ACERTOU NO CHUTE!\n");
        }
        else if (strcmp(recvBuffer, "CHUTE_ERRADO") == 0) {
            printf("  [-] Chute errado! Voce perdeu 2 vidas...\n");
        }
        else if (strcmp(recvBuffer, "CHUTE_OPONENTE_CERTO") == 0) {
            printf("  [+] O oponente acertou a palavra no chute!\n");
        }
        else if (strcmp(recvBuffer, "CHUTE_OPONENTE_ERRADO") == 0) {
            printf("  [-] O oponente errou o chute! Sua vez!\n");
        }
        else if (strcmp(recvBuffer, "INVALIDO") == 0) {
            printf("  [!] Comando invalido! Tente novamente.\n");
        }

        // Fim de jogo: vitória
        else if (strncmp(recvBuffer, "VITORIA:", 8) == 0) {
            char vencedor[10];
            strcpy(vencedor, recvBuffer + 8);
            separador();
            if ((meu_numero == 1 && strcmp(vencedor, "JG1") == 0) ||
                (meu_numero == 2 && strcmp(vencedor, "JG2") == 0)) {
                printf("   *** PARABENS! VOCE VENCEU! ***\n");
            } else {
                printf("   O oponente completou a palavra. Melhor sorte na proxima!\n");
            }
            printf("   Palavra: %s\n", palavra_agora);
        }

        // Fim de jogo: derrota de ambos
        else if (strncmp(recvBuffer, "Derrota:", 8) == 0){
            separador();
            printf("   FIM DE JOGO! NINGUEM GANHOU ESSA...\n");
            printf("   A palavra era: %s\n", recvBuffer + 8);
        }

        // Um jogador abandonou
        else if (strcmp(recvBuffer, "SAIR") == 0){
            separador();
            printf("   Um jogador saiu. Encerrando...\n");
            break;
        }

        // Pergunta de nova rodada vinda do servidor
        else if (strcmp(recvBuffer, "JOGAR_NOVAMENTE") == 0){
            separador();
            printf("   Deseja jogar novamente?\n");
            printf("   [1] Nova palavra\n");
            printf("   [ENTER] Sair\n");
            printf("----------------------------------------\n");
            printf("  > ");
            fflush(stdout);

            char escolha[10];
            fgets(escolha, sizeof(escolha), stdin);
            escolha[strcspn(escolha, "\n")] = '\0';

            if (strcmp(escolha, "1") == 0) {
                send(clientSocket, "SIM", 4, 0);
                printf("   Aguardando o oponente decidir...\n");
            } else {
                send(clientSocket, "NAO", 4, 0);
                printf("   Saindo do jogo...\n");
            }
        }

        // Servidor confirmou nova rodada
        else if (strcmp(recvBuffer, "NOVA_RODADA") == 0){
            memset(palavra_agora, 0, sizeof(palavra_agora));
            memset(letras_usadas, 0, sizeof(letras_usadas));
            erros_agora = 0;
            separador();
            printf("   NOVA RODADA INICIANDO!\n");
        }

        // Servidor encerrou a sessão
        else if (strcmp(recvBuffer, "ENCERRAR") == 0){
            separador();
            printf("   Sessao encerrada. Obrigado por jogar!\n");
            break;
        }
    }

    printf("\nPressione ENTER para sair...\n");
// Finalizando socket/biblioteca
    getchar();
// Usar client/server
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}