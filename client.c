#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <ctype.h>

#pragma comment(lib, "Ws2_32.lib")

// gcc client.c -o client -lws2_32
// ./client  (abrir em dois terminais separados)

// ─── helpers visuais ─────────────────────────────────────────────────────────

void separador() {
    printf("\n========================================\n");
}

// Desenha a forca: erros vai de 0 a 6
void desenhar_forca(int erros) {
    if (erros > 6) erros = 6;
    printf("  +---+\n");
    printf("  |   |\n");
    printf("  %s   |\n",                         erros >= 1 ? "O" : " ");
    printf(" %s%s%s  |\n",  erros >= 3 ? "/" : " ",
                             erros >= 2 ? "|" : " ",
                             erros >= 4 ? "\\" : " ");
    printf(" %s %s  |\n",  erros >= 5 ? "/" : " ",
                             erros >= 6 ? "\\" : " ");
    printf("       |\n");
    printf("=========\n");
}

// ─── leitura de linha limpa ──────────────────────────────────────────────────

// Lê uma linha do stdin, remove \n e descarta o restante se passar de max
void ler_linha(char *buf, int max) {
    if (fgets(buf, max, stdin) == NULL) { buf[0] = '\0'; return; }
    int len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        // linha maior que o buffer: descarta o restante
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
}

// ─── main ────────────────────────────────────────────────────────────────────

int main(void) {
    WSADATA wsd;
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("Falha ao inicializar Winsock\n");
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Erro ao criar socket: %d\n", WSAGetLastError());
        WSACleanup(); return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port        = htons(5555);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Erro ao conectar ao servidor: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup(); return 1;
    }

    separador();
    printf("   BEM-VINDO AO JOGO DA FORCA!\n");
    printf("   Conectado! Aguardando o outro jogador...\n");

    // Estado local do cliente
    char palavra_atual[100] = "";
    char letras_usadas[64]  = "";
    int  erros_atual        = 0;
    int  meu_numero         = 0;

    // Buffer de recepcao com acumulador (evita mensagens coladas)
    char  acum[4096] = "";
    int   acum_len   = 0;
    char  recvBuffer[1024];
    char  sendBuffer[512];
    int   bytesReceived;

    // ── loop principal ────────────────────────────────────────────────────────
    while (1) {
        // Recebe dados e acumula
        bytesReceived = recv(sock, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if (bytesReceived <= 0) {
            printf("\nConexao encerrada pelo servidor.\n");
            break;
        }

        // Adiciona ao acumulador (mensagens separadas por '\0')
        if (acum_len + bytesReceived < (int)sizeof(acum)) {
            memcpy(acum + acum_len, recvBuffer, bytesReceived);
            acum_len += bytesReceived;
        }

        // Processa todas as mensagens completas (terminadas em '\0') do acumulador
        int pos = 0;
        while (pos < acum_len) {
            // Encontra o próximo '\0'
            int end = pos;
            while (end < acum_len && acum[end] != '\0') end++;
            if (end >= acum_len) break; // mensagem incompleta, aguarda mais dados

            char *msg = acum + pos;
            pos = end + 1;

            // ── despacha mensagem ─────────────────────────────────────────

            // Identificação do jogador
            if (strcmp(msg, "VOCE_JG1") == 0) {
                meu_numero = 1;
                separador();
                printf("   Voce eh o JOGADOR 1!\n");
            }
            else if (strcmp(msg, "VOCE_JG2") == 0) {
                meu_numero = 2;
                separador();
                printf("   Voce eh o JOGADOR 2!\n");
            }

            // Estado do jogo: palavra
            else if (strncmp(msg, "Palavra: ", 9) == 0) {
                strncpy(palavra_atual, msg + 9, sizeof(palavra_atual) - 1);
                palavra_atual[sizeof(palavra_atual) - 1] = '\0';
            }
            // Estado do jogo: erros
            else if (strncmp(msg, "Erros: ", 7) == 0) {
                sscanf(msg + 7, "%d", &erros_atual);
            }
            // Estado do jogo: letras (sempre a última das 3 — exibe tudo junto)
            else if (strncmp(msg, "Letras ja usadas: ", 18) == 0) {
                strncpy(letras_usadas, msg + 18, sizeof(letras_usadas) - 1);
                letras_usadas[sizeof(letras_usadas) - 1] = '\0';
                separador();
                desenhar_forca(erros_atual);
                printf("\n  Palavra : %s\n", palavra_atual);
                printf("  Erros   : %d/6\n", erros_atual);
                if (strlen(letras_usadas) > 0)
                    printf("  Letras  : %s\n", letras_usadas);
                else
                    printf("  Letras  : (nenhuma ainda)\n");
            }

            // ── sua vez ───────────────────────────────────────────────────
            else if (strcmp(msg, "SUA_VEZ") == 0) {
                separador();
                printf("   SUA VEZ, JOGADOR %d!\n", meu_numero);
                printf("----------------------------------------\n");
                printf("  [%dL<letra>]    Tentar letra   ex: %dLa\n", meu_numero, meu_numero);
                printf("  [%dC<palavra>]  Chutar palavra ex: %dCgrafos\n", meu_numero, meu_numero);
                printf("----------------------------------------\n");
                printf("  > ");
                fflush(stdout);

                char entrada[128];
                ler_linha(entrada, sizeof(entrada));

                // Valida formato: deve começar com o numero do jogador
                char tipo = (strlen(entrada) >= 2) ? toupper((unsigned char)entrada[1]) : 0;

                if ((tipo == 'L' || tipo == 'C') && entrada[2] != '\0') {
                    if (tipo == 'L') {
                        char letra = tolower((unsigned char)entrada[2]);
                        sprintf(sendBuffer, "%dL%c", meu_numero, letra);
                    } else {
                        // Normaliza o chute para minúsculo
                        char chute[100];
                        strncpy(chute, entrada + 2, sizeof(chute) - 1);
                        chute[sizeof(chute) - 1] = '\0';
                        for (int i = 0; chute[i]; i++)
                            chute[i] = tolower((unsigned char)chute[i]);
                        sprintf(sendBuffer, "%dC%s", meu_numero, chute);
                    }
                } else {
                    printf("  [!] Formato invalido! Use %dL<letra> ou %dC<palavra>\n",
                           meu_numero, meu_numero);
                    sprintf(sendBuffer, "%dX", meu_numero); // inválido; servidor responde INVALIDO
                }

                send(sock, sendBuffer, (int)strlen(sendBuffer) + 1, 0);
            }

            // ── espera ────────────────────────────────────────────────────
            else if (strcmp(msg, "ESPERE") == 0) {
                separador();
                printf("   Aguardando jogada do oponente...\n");
            }

            // ── resultados de jogada ──────────────────────────────────────
            else if (strcmp(msg, "ACERTO") == 0) {
                printf("\n  [+] Letra correta! Voce mantem a vez!\n");
            }
            else if (strcmp(msg, "ERRO") == 0) {
                printf("\n  [-] Letra errada! Vez do oponente.\n");
            }
            else if (strcmp(msg, "ACERTO_OPONENTE") == 0) {
                printf("\n  [+] O oponente acertou uma letra e mantem a vez!\n");
            }
            else if (strcmp(msg, "ERRO_OPONENTE") == 0) {
                printf("\n  [-] O oponente errou! Agora e sua vez!\n");
            }
            else if (strcmp(msg, "LETRA_JA_USADA") == 0) {
                printf("\n  [!] Essa letra ja foi tentada! Voce mantem a vez.\n");
            }
            else if (strcmp(msg, "CHUTE_CERTO") == 0) {
                printf("\n  [+] VOCE ACERTOU NO CHUTE!\n");
            }
            else if (strcmp(msg, "CHUTE_ERRADO") == 0) {
                printf("\n  [-] Chute errado! Voce perdeu 2 vidas.\n");
            }
            else if (strcmp(msg, "CHUTE_OPONENTE_CERTO") == 0) {
                printf("\n  [+] O oponente acertou a palavra no chute!\n");
            }
            else if (strcmp(msg, "CHUTE_OPONENTE_ERRADO") == 0) {
                printf("\n  [-] O oponente errou o chute e perdeu 2 vidas! Sua vez!\n");
            }
            else if (strcmp(msg, "INVALIDO") == 0) {
                printf("\n  [!] Comando invalido! Tente novamente.\n");
            }

            // ── fim de jogo: vitoria ──────────────────────────────────────
            else if (strncmp(msg, "VITORIA:", 8) == 0) {
                const char *vencedor = msg + 8;
                separador();
                desenhar_forca(erros_atual);
                printf("\n  Palavra : %s\n", palavra_atual);
                printf("  Erros   : %d/6\n", erros_atual);
                separador();
                int eu_venci = (meu_numero == 1 && strcmp(vencedor, "JG1") == 0) ||
                               (meu_numero == 2 && strcmp(vencedor, "JG2") == 0);
                if (eu_venci)
                    printf("   *** PARABENS! VOCE VENCEU! ***\n");
                else
                    printf("   O oponente completou a palavra. Melhor sorte!\n");

                // Pergunta nova rodada direto aqui, sem esperar JOGAR_NOVAMENTE
                separador();
                printf("   Deseja jogar novamente?\n");
                printf("   [1] Nova partida\n");
                printf("   [ENTER] Sair\n");
                printf("  > ");
                fflush(stdout);

                char escolha[10];
                ler_linha(escolha, sizeof(escolha));

                if (strcmp(escolha, "1") == 0) {
                    send(sock, "SIM", 4, 0);
                    printf("\n   Aguardando decisao do oponente...\n");
                } else {
                    send(sock, "NAO", 4, 0);
                    printf("\n   Saindo do jogo...\n");
                }
            }

            // ── fim de jogo: derrota ──────────────────────────────────────
            else if (strncmp(msg, "Derrota:", 8) == 0) {
                const char *palavra_real = msg + 8;
                separador();
                desenhar_forca(6); // forca completa na derrota
                printf("\n  FIM DE JOGO! NINGUEM GANHOU ESSA...\n");
                printf("  A palavra era: %s\n", palavra_real);

                // Pergunta nova rodada direto aqui, sem esperar JOGAR_NOVAMENTE
                separador();
                printf("   Deseja jogar novamente?\n");
                printf("   [1] Nova partida\n");
                printf("   [ENTER] Sair\n");
                printf("  > ");
                fflush(stdout);

                char escolha[10];
                ler_linha(escolha, sizeof(escolha));

                if (strcmp(escolha, "1") == 0) {
                    send(sock, "SIM", 4, 0);
                    printf("\n   Aguardando decisao do oponente...\n");
                } else {
                    send(sock, "NAO", 4, 0);
                    printf("\n   Saindo do jogo...\n");
                }
            }

            // ── nova rodada confirmada ────────────────────────────────────
            else if (strcmp(msg, "NOVA_RODADA") == 0) {
                memset(palavra_atual, 0, sizeof(palavra_atual));
                memset(letras_usadas, 0, sizeof(letras_usadas));
                erros_atual = 0;
                separador();
                printf("   NOVA RODADA INICIANDO!\n");
            }

            // ── encerramento ──────────────────────────────────────────────
            else if (strcmp(msg, "ENCERRAR") == 0) {
                separador();
                printf("   Sessao encerrada. Obrigado por jogar!\n");
                goto fim;
            }
            else if (strcmp(msg, "SAIR") == 0) {
                separador();
                printf("   Um jogador saiu. Encerrando...\n");
                goto fim;
            }

            // Mensagem desconhecida (debug)
            else {
                printf("  [debug] msg desconhecida: '%s'\n", msg);
            }

        } // fim while mensagens no acumulador

        // Move bytes não processados para o início do acumulador
        int restante = acum_len - pos;
        if (restante > 0)
            memmove(acum, acum + pos, restante);
        acum_len = restante;

    } // fim loop principal

fim:
    printf("\nPressione ENTER para fechar...\n");
    getchar();
    closesocket(sock);
    WSACleanup();
    return 0;
}