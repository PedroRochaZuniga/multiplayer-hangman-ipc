#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <time.h>
#include <ctype.h>

#pragma comment(lib, "Ws2_32.lib")

// gcc server.c -o server -lws2_32
// gcc client.c -o client -lws2_32
// ./server
// ./client  (abrir dois terminais com client)

#define MAX_ERROS 6

const char *palavras[] = {
    "computador", "operacional", "clusters", "processo",
    "deadlock", "sistema", "memoria", "registrador", "grafos"
};
#define NUM_PALAVRAS (sizeof(palavras) / sizeof(palavras[0]))

typedef struct {
    const char *palavra;
    char palavra_secreta[100];
    char letras_usadas[27];
    int  num_letras_usadas;
    int  erros;
    int  turno;       // 1 ou 2
    int  jogo_ativo;
} EstadoJogo;

// ─── helpers de envio ───────────────────────────────────────────────────────

// Envia string + '\0' para um socket
void send_msg(SOCKET s, const char *msg) {
    send(s, msg, (int)strlen(msg) + 1, 0);
}

// Envia para os dois jogadores
void broadcast(SOCKET j1, SOCKET j2, const char *msg) {
    send_msg(j1, msg);
    send_msg(j2, msg);
}

// ─── lógica do jogo ─────────────────────────────────────────────────────────

void inicializar_jogo(EstadoJogo *e) {
    memset(e, 0, sizeof(EstadoJogo));
    e->palavra = palavras[rand() % NUM_PALAVRAS];
    int n = (int)strlen(e->palavra);
    for (int i = 0; i < n; i++) e->palavra_secreta[i] = '_';
    e->palavra_secreta[n] = '\0';
    e->letras_usadas[0]   = '\0';
    e->erros              = 0;
    e->turno              = 1;
    e->jogo_ativo         = 1;
    e->num_letras_usadas  = 0;
}

// Envia estado atual (palavra, erros, letras) para ambos
void enviar_estado(SOCKET j1, SOCKET j2, EstadoJogo *e) {
    char buf[512];

    sprintf(buf, "Palavra: %s", e->palavra_secreta);
    broadcast(j1, j2, buf);

    sprintf(buf, "Erros: %d", e->erros);
    broadcast(j1, j2, buf);

    sprintf(buf, "Letras ja usadas: %s", e->letras_usadas);
    broadcast(j1, j2, buf);
}

int palavra_descoberta(EstadoJogo *e) {
    return strchr(e->palavra_secreta, '_') == NULL;
}

// Retorna: -1 letra já usada | 0 errou | 1 acertou
int processar_letra(EstadoJogo *e, char letra) {
    letra = tolower((unsigned char)letra);

    for (int i = 0; i < e->num_letras_usadas; i++)
        if (e->letras_usadas[i] == letra) return -1;

    e->letras_usadas[e->num_letras_usadas++] = letra;
    e->letras_usadas[e->num_letras_usadas]   = '\0';

    int acertou = 0;
    for (int i = 0; i < (int)strlen(e->palavra); i++) {
        if (e->palavra[i] == letra) {
            e->palavra_secreta[i] = letra;
            acertou = 1;
        }
    }
    if (!acertou) e->erros++;
    return acertou;
}

// Retorna: 1 acertou | 0 errou  (chute errado custa 2 erros)
int processar_chute(EstadoJogo *e, const char *chute) {
    char buf[100];
    strncpy(buf, chute, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    for (int i = 0; buf[i]; i++)
        buf[i] = tolower((unsigned char)buf[i]);

    if (strcmp(buf, e->palavra) == 0) {
        strcpy(e->palavra_secreta, e->palavra); // revela tudo
        return 1;
    }
    e->erros += 2;
    return 0;
}

// ─── main ───────────────────────────────────────────────────────────────────

int main(void) {
    WSADATA wsd;
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WSAStartup falhou\n");
        return 1;
    }
    printf("[Servidor] Winsock iniciado.\n");

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Erro ao criar socket: %d\n", WSAGetLastError());
        WSACleanup(); return 1;
    }

    // Permite reusar a porta imediatamente após reinicio
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in server;
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port        = htons(5555);

    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Erro no bind: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup(); return 1;
    }
    if (listen(sock, 2) == SOCKET_ERROR) {
        printf("Erro no listen: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup(); return 1;
    }
    printf("[Servidor] Aguardando 2 jogadores na porta 5555...\n");

    struct sockaddr_in ca;
    int cal = sizeof(ca);
    SOCKET jogador1 = accept(sock, (struct sockaddr*)&ca, &cal);
    printf("[Servidor] Jogador 1 conectado.\n");
    SOCKET jogador2 = accept(sock, (struct sockaddr*)&ca, &cal);
    printf("[Servidor] Jogador 2 conectado.\n");

    if (jogador1 == INVALID_SOCKET || jogador2 == INVALID_SOCKET) {
        printf("Erro ao aceitar conexao: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup(); return 1;
    }

    srand((unsigned int)time(NULL));

    char recvBuffer[512];
    char sendBuffer[512];
    int  bytesReceived;

    // ── loop externo: rodadas ──────────────────────────────────────────────
    while (1) {
        EstadoJogo estado;
        inicializar_jogo(&estado);

        printf("[Servidor] Palavra sorteada: %s (%d letras)\n",
               estado.palavra, (int)strlen(estado.palavra));

        send_msg(jogador1, "VOCE_JG1");
        send_msg(jogador2, "VOCE_JG2");

        enviar_estado(jogador1, jogador2, &estado);

        // ── loop interno: turnos ───────────────────────────────────────────
        while (estado.jogo_ativo) {
            SOCKET atual  = (estado.turno == 1) ? jogador1 : jogador2;
            SOCKET espera = (estado.turno == 1) ? jogador2 : jogador1;

            send_msg(atual,  "SUA_VEZ");
            send_msg(espera, "ESPERE");

            bytesReceived = recv(atual, recvBuffer, sizeof(recvBuffer) - 1, 0);
            if (bytesReceived <= 0) {
                printf("[Servidor] Jogador %d desconectou.\n", estado.turno);
                broadcast(jogador1, jogador2, "SAIR");
                goto encerrar;
            }
            recvBuffer[bytesReceived] = '\0';
            printf("[Servidor] Recebido: '%s'\n", recvBuffer);

            int troca_turno = 1;

            char id           = recvBuffer[0]; // '1' ou '2'
            char tipo         = toupper((unsigned char)recvBuffer[1]); // 'L' ou 'C'
            const char *dado  = recvBuffer + 2;

            // Verifica se é o turno correto
            if (id - '0' != estado.turno) {
                printf("[Servidor] Mensagem fora de turno ignorada.\n");
                send_msg(atual, "INVALIDO");
                troca_turno = 0;
            }
            // ── jogada de LETRA ───────────────────────────────────────────
            else if (tipo == 'L') {
                if (dado[0] == '\0') {
                    send_msg(atual, "INVALIDO");
                    troca_turno = 0;
                } else {
                    char letra = dado[0];
                    printf("[Servidor] JG%d tentou letra '%c'\n", estado.turno, letra);
                    int resultado = processar_letra(&estado, letra);

                    if (resultado == -1) {
                        send_msg(atual, "LETRA_JA_USADA");
                        troca_turno = 0; // não perde a vez
                    } else if (resultado == 1) {
                        send_msg(atual,  "ACERTO");
                        send_msg(espera, "ACERTO_OPONENTE");
                        troca_turno = 0; // mantém a vez no acerto
                    } else {
                        send_msg(atual,  "ERRO");
                        send_msg(espera, "ERRO_OPONENTE");
                        troca_turno = 1; // passa a vez no erro
                    }
                }
            }
            // ── jogada de CHUTE ───────────────────────────────────────────
            else if (tipo == 'C') {
                if (dado[0] == '\0') {
                    send_msg(atual, "INVALIDO");
                    troca_turno = 0;
                } else {
                    printf("[Servidor] JG%d chutou: '%s'\n", estado.turno, dado);
                    int resultado = processar_chute(&estado, dado);

                    if (resultado == 1) {
                        send_msg(atual,  "CHUTE_CERTO");
                        send_msg(espera, "CHUTE_OPONENTE_CERTO");
                        troca_turno = 0; // encerra logo abaixo
                    } else {
                        send_msg(atual,  "CHUTE_ERRADO");
                        send_msg(espera, "CHUTE_OPONENTE_ERRADO");
                        troca_turno = 1;
                    }
                }
            }
            // ── comando inválido ──────────────────────────────────────────
            else {
                send_msg(atual, "INVALIDO");
                troca_turno = 0;
            }

            // ── verifica fim de jogo ──────────────────────────────────────
            if (palavra_descoberta(&estado)) {
                enviar_estado(jogador1, jogador2, &estado);
                sprintf(sendBuffer, "VITORIA:JG%d", estado.turno);
                broadcast(jogador1, jogador2, sendBuffer);
                printf("[Servidor] Jogador %d venceu!\n", estado.turno);
                estado.jogo_ativo = 0;
            } else if (estado.erros >= MAX_ERROS) {
                // Garante que erros não ultrapasse MAX_ERROS na exibição
                if (estado.erros > MAX_ERROS) estado.erros = MAX_ERROS;
                enviar_estado(jogador1, jogador2, &estado);
                sprintf(sendBuffer, "Derrota:%s", estado.palavra);
                broadcast(jogador1, jogador2, sendBuffer);
                printf("[Servidor] Ambos perderam! Palavra: %s\n", estado.palavra);
                estado.jogo_ativo = 0;
            } else {
                enviar_estado(jogador1, jogador2, &estado);
                if (troca_turno)
                    estado.turno = (estado.turno == 1) ? 2 : 1;
            }
        } // fim loop de turnos

        // ── pergunta nova rodada ───────────────────────────────────────────
        // O cliente pergunta sozinho ao receber VITORIA/Derrota e envia SIM ou NAO.
        // O servidor só precisa aguardar as duas respostas.
        printf("[Servidor] Aguardando decisao dos jogadores...\n");

        bytesReceived = recv(jogador1, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if (bytesReceived <= 0) goto encerrar;
        recvBuffer[bytesReceived] = '\0';
        int j1_quer = (strcmp(recvBuffer, "SIM") == 0);

        bytesReceived = recv(jogador2, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if (bytesReceived <= 0) goto encerrar;
        recvBuffer[bytesReceived] = '\0';
        int j2_quer = (strcmp(recvBuffer, "SIM") == 0);

        if (j1_quer && j2_quer) {
            broadcast(jogador1, jogador2, "NOVA_RODADA");
            printf("[Servidor] Iniciando nova rodada!\n");
        } else {
            broadcast(jogador1, jogador2, "ENCERRAR");
            printf("[Servidor] Encerrando sessao.\n");
            break;
        }
    } // fim loop de rodadas

encerrar:
    closesocket(jogador1);
    closesocket(jogador2);
    closesocket(sock);
    WSACleanup();
    printf("[Servidor] Encerrado. Pressione ENTER para fechar.\n");
    getchar();
    return 0;
}