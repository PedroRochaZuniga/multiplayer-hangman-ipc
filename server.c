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




//MEMÓRIA COMPARTILHADA
//EstadoJogo é uma variável GLOBAL acessada pelas duas threads simultaneamente

#define MAX_ERROS 6

const char *palavras[] = {
    "computador", "operacional", "clusters", "processo",
    "deadlock", "sistema", "memoria", "registrador", "grafos"
};
#define NUM_PALAVRAS (sizeof(palavras) / sizeof(palavras[0]))

//Estado global do jogo
typedef struct {
    const char *palavra;
    char palavra_secreta[100];
    char letras_usadas[27];
    int  num_letras_usadas;
    int  erros;
    int  turno;       
    int  jogo_ativo;
} EstadoJogo;

EstadoJogo estado;          
SOCKET     jogador1, jogador2;

//Exclusão mútua
//MUTEX - só uma thread acessa estado
HANDLE mutex;                


// vez[0] = evento da thread do jogador 1
// vez[1] = evento da thread do jogador 2
// A thread do jogador N fica em WaitForSingleObject(vez[N-1]) até ser sua vez
HANDLE vez[2];


//Cada thread sinaliza resp[N-1] após receber SIM/NAO do cliente para ver se reinicia uma nova rodada
//1 = SIM e 0 para não, (espaço)
HANDLE resp[2];             
int    quer_jogar[2];       
volatile int encerrar = 0;  


//Função auxiliares
void send_msg(SOCKET s, const char *msg) {
    send(s, msg, (int)strlen(msg) + 1, 0);
}

void broadcast(const char *msg) {
    send_msg(jogador1, msg);
    send_msg(jogador2, msg);
}


//Lógica do jogo
void inicializar_jogo(void) {
    memset(&estado, 0, sizeof(EstadoJogo));
    estado.palavra = palavras[rand() % NUM_PALAVRAS];
    int n = (int)strlen(estado.palavra);
    for (int i = 0; i < n; i++) estado.palavra_secreta[i] = '_';
    estado.palavra_secreta[n] = '\0';
    estado.letras_usadas[0]   = '\0';
    estado.erros              = 0;
    estado.turno              = 1;
    estado.jogo_ativo         = 1;
    estado.num_letras_usadas  = 0;
}

//Envia os dados do jogo na rodada, travado pelo mutex
void enviar_estado(void) {
    char buf[512];
    sprintf(buf, "Palavra: %s",          estado.palavra_secreta);  broadcast(buf);
    sprintf(buf, "Erros: %d",            estado.erros);             broadcast(buf);
    sprintf(buf, "Letras ja usadas: %s", estado.letras_usadas);     broadcast(buf);
}

//Verifica se palavra_secreta já foi descoberta
int palavra_descoberta(void) {
    return strchr(estado.palavra_secreta, '_') == NULL;
}

//Retorna: -1 letra já usada , 0 errou , 1 acertou
int processar_letra(char letra) {
    letra = tolower((unsigned char)letra);
    for (int i = 0; i < estado.num_letras_usadas; i++)
        if (estado.letras_usadas[i] == letra) return -1;

    estado.letras_usadas[estado.num_letras_usadas++] = letra;
    estado.letras_usadas[estado.num_letras_usadas]   = '\0';

    int acertou = 0;
    for (int i = 0; i < (int)strlen(estado.palavra); i++) {
        if (estado.palavra[i] == letra) {
            estado.palavra_secreta[i] = letra;
            acertou = 1;
        }
    }
    if (!acertou) estado.erros++;
    return acertou;
}

//Retorna: 1 acertou, 0 errou (chute errado custa 2 erros)
int processar_chute(const char *chute) {
    char buf[100];
    strncpy(buf, chute, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    for (int i = 0; buf[i]; i++)
        buf[i] = tolower((unsigned char)buf[i]);

    if (strcmp(buf, estado.palavra) == 0) {
        strcpy(estado.palavra_secreta, estado.palavra);
        return 1;
    }
    estado.erros += 2;
    if (estado.erros > MAX_ERROS) estado.erros = MAX_ERROS;
    return 0;
}


//Estrutura para as thread
typedef struct {
    int    numero;   // 1 ou 2
    SOCKET socket;
    SOCKET outro;   
} DadosThread;


//Função da thread - uma instancia por jogador
DWORD WINAPI thread_jogador(LPVOID arg) {
    DadosThread *d   = (DadosThread *)arg;
    int  meu_num     = d->numero;   // 1 ou 2
    SOCKET meu_sock  = d->socket;
    SOCKET outro_sock= d->outro;
    int  idx         = meu_num - 1; // índice 0-based para arrays vez[] e resp[]

    char recvBuffer[512];
    char sendBuffer[512];
    int  bytesReceived;

    //Loop para partidas, quando acaba o jogo
    while (!encerrar) {

        //Troca dos turnos e rodadas
        while (!encerrar) {

            //BLOQUEIO DE TURNO 
            //A thread fica aqui parada até o evento vez[idx] ser sinalizado
            //só a thread da vez avança
            WaitForSingleObject(vez[idx], INFINITE);
            if (encerrar) goto fim_thread;

            //Le mutex pra ver se jogo ainda tá ativo
            WaitForSingleObject(mutex, INFINITE);
            int ativo = estado.jogo_ativo;
            ReleaseMutex(mutex);
            if (!ativo) break;

            
            send_msg(meu_sock,  "SUA_VEZ");
            send_msg(outro_sock,"ESPERE");

            //Recebe jogada do cliente (fora do mutex — recv pode bloquear)
            bytesReceived = recv(meu_sock, recvBuffer, sizeof(recvBuffer) - 1, 0);
            if (bytesReceived <= 0) {
                printf("[Servidor] Jogador %d desconectou.\n", meu_num);
                encerrar = 1;
                broadcast("SAIR");
                // Libera a outra thread que pode estar esperando em vez[]
                SetEvent(vez[1 - idx]);
                goto fim_thread;
            }
            recvBuffer[bytesReceived] = '\0';
            printf("[Servidor] JG%d enviou: '%s'\n", meu_num, recvBuffer);

            char id   = recvBuffer[0];
            char tipo = toupper((unsigned char)recvBuffer[1]);
            const char *dado = recvBuffer + 2;

            //SEÇÃO CRÍTICA: acesso exclusivo ao estado 
            // penas UMA thread por vez entra aqui
            WaitForSingleObject(mutex, INFINITE);

            int troca_turno = 1;

            //Verifica turno correto
            if (id - '0' != meu_num || id - '0' != estado.turno) {
                send_msg(meu_sock, "INVALIDO");
                troca_turno = 0;
            }
            //Verificar letra
            else if (tipo == 'L') {
                if (dado[0] == '\0') {
                    send_msg(meu_sock, "INVALIDO");
                    troca_turno = 0;
                } else {
                    char letra = dado[0];
                    printf("[Servidor] JG%d tentou letra '%c'\n", meu_num, letra);
                    int res = processar_letra(letra);

                    if (res == -1) {
                        send_msg(meu_sock, "LETRA_JA_USADA");
                        troca_turno = 0;
                    } else if (res == 1) {
                        send_msg(meu_sock,   "ACERTO");
                        send_msg(outro_sock, "ACERTO_OPONENTE");
                        troca_turno = 0; // mantém a vez
                    } else {
                        send_msg(meu_sock,   "ERRO");
                        send_msg(outro_sock, "ERRO_OPONENTE");
                        troca_turno = 1; // passa a vez
                    }
                }
            }
            //Verificar chute
            else if (tipo == 'C') {
                if (dado[0] == '\0') {
                    send_msg(meu_sock, "INVALIDO");
                    troca_turno = 0;
                } else {
                    printf("[Servidor] JG%d chutou: '%s'\n", meu_num, dado);
                    int res = processar_chute(dado);

                    if (res == 1) {
                        send_msg(meu_sock,   "CHUTE_CERTO");
                        send_msg(outro_sock, "CHUTE_OPONENTE_CERTO");
                        troca_turno = 0;
                    } else {
                        send_msg(meu_sock,   "CHUTE_ERRADO");
                        send_msg(outro_sock, "CHUTE_OPONENTE_ERRADO");
                        troca_turno = 1;
                    }
                }
            }
            else {
                send_msg(meu_sock, "INVALIDO");
                troca_turno = 0;
            }

            //Ve o fim de jogo, ainda no mutex
            if (palavra_descoberta()) {
                enviar_estado();
                sprintf(sendBuffer, "VITORIA:JG%d", meu_num);
                broadcast(sendBuffer);
                printf("[Servidor] Jogador %d venceu!\n", meu_num);
                estado.jogo_ativo = 0;
                ReleaseMutex(mutex);
                //Libera a outra thread (que está em WaitForSingleObject)
                SetEvent(vez[1 - idx]);
                break;
            }
            else if (estado.erros >= MAX_ERROS) {
                enviar_estado();
                sprintf(sendBuffer, "Derrota:%s", estado.palavra);
                broadcast(sendBuffer);
                printf("[Servidor] Ambos perderam! Palavra: %s\n", estado.palavra);
                estado.jogo_ativo = 0;
                ReleaseMutex(mutex);
                SetEvent(vez[1 - idx]);
                break;
            }
            else {
                //Jogo continua: envia estado e decide troca de turno
                enviar_estado();
                if (troca_turno) {
                    estado.turno = (estado.turno == 1) ? 2 : 1;
                    ReleaseMutex(mutex);
                    //Sinaliza a thread do oponente para jogar
                    SetEvent(vez[1 - idx]);
                    //NÃO sinaliza vez[idx] — esta thread volta ao topo e bloqueia
                } else {
                    //Mantém a vez
                    ReleaseMutex(mutex);
                    SetEvent(vez[idx]);
                }
            }
        } //fim loop de turnos

        //Aguarda resposta de nova rodada do cliente
        //O cliente pergunta sozinho (ao receber VITORIA/Derrota) e envia SIM/NAO.
        printf("[Servidor] Aguardando resposta do jogador %d...\n", meu_num);
        bytesReceived = recv(meu_sock, recvBuffer, sizeof(recvBuffer) - 1, 0);
        if (bytesReceived <= 0) { encerrar = 1; SetEvent(resp[1-idx]); goto fim_thread; }
        recvBuffer[bytesReceived] = '\0';

        //Salva resposta e sinaliza que esta thread já respondeu
        quer_jogar[idx] = (strcmp(recvBuffer, "SIM") == 0);
        SetEvent(resp[idx]);


        if (meu_num == 1) {
            WaitForSingleObject(resp[1], INFINITE); // espera jogador 2 responder
            if (encerrar) goto fim_thread;

            if (quer_jogar[0] && quer_jogar[1]) {
                //Reinicia o estado
                WaitForSingleObject(mutex, INFINITE);
                inicializar_jogo();
                printf("[Servidor] Palavra sorteada: %s (%d letras)\n",
                       estado.palavra, (int)strlen(estado.palavra));
                ReleaseMutex(mutex);

                broadcast("NOVA_RODADA");
                printf("[Servidor] Iniciando nova rodada!\n");

                //Reseta eventos de resposta para a próxima rodada
                ResetEvent(resp[0]);
                ResetEvent(resp[1]);

                //Informa novo número de jogador e estado inicial
                send_msg(jogador1, "VOCE_JG1");
                send_msg(jogador2, "VOCE_JG2");
                WaitForSingleObject(mutex, INFINITE);
                enviar_estado();
                ReleaseMutex(mutex);

                //Sinaliza thread 1 para começar (turno sempre começa no JG1)
                SetEvent(vez[0]);
                //hread 2 será liberada quando thread 1 passar a vez ou encerrar
            } else {
                broadcast("ENCERRAR");
                printf("[Servidor] Encerrando sessao.\n");
                encerrar = 1;
                //Libera thread 2 caso esteja bloqueada em vez[]
                SetEvent(vez[1]);
                goto fim_thread;
            }
        } else {
        }

    } //fim loop de rodadas

fim_thread:
    free(d);
    return 0;
}

//Inicializa tudo e lança as duas thread

int main(void) {
    WSADATA wsd;
    if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
        printf("WSAStartup falhou\n"); return 1;
    }
    printf("[Servidor] Winsock iniciado.\n");

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("Erro ao criar socket: %d\n", WSAGetLastError());
        WSACleanup(); return 1;
    }

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

    struct sockaddr_in ca; int cal = sizeof(ca);
    jogador1 = accept(sock, (struct sockaddr*)&ca, &cal);
    printf("[Servidor] Jogador 1 conectado.\n");
    jogador2 = accept(sock, (struct sockaddr*)&ca, &cal);
    printf("[Servidor] Jogador 2 conectado.\n");

    if (jogador1 == INVALID_SOCKET || jogador2 == INVALID_SOCKET) {
        printf("Erro ao aceitar conexao: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup(); return 1;
    }

    srand((unsigned int)time(NULL));


//MUTEX para estadodojogo
    mutex  = CreateMutex(NULL, FALSE, NULL);

//EVENTOS de turno: manual-reset=FALSE (auto-reset), estado inicial=FALSE
//Auto-reset garante que apenas UMA thread seja liberada por SetEvent()
    vez[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
    vez[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

//EVENTOS de resposta de nova rodada
    resp[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
    resp[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

 //Inicializa o estado do jogo
    inicializar_jogo();
    printf("[Servidor] Palavra sorteada: %s (%d letras)\n",
           estado.palavra, (int)strlen(estado.palavra));

    send_msg(jogador1, "VOCE_JG1");
    send_msg(jogador2, "VOCE_JG2");
    enviar_estado();

//Lança as duas threads
    DadosThread *d1 = malloc(sizeof(DadosThread));
    d1->numero = 1; 
    d1->socket = jogador1; 
    d1->outro = jogador2;

    DadosThread *d2 = malloc(sizeof(DadosThread));
    d2->numero = 2; 
    d2->socket = jogador2; 
    d2->outro = jogador1;

    HANDLE threads[2];
    threads[0] = CreateThread(NULL, 0, thread_jogador, d1, 0, NULL);
    threads[1] = CreateThread(NULL, 0, thread_jogador, d2, 0, NULL);

    //Sinaliza thread do jogador 1 para começar
    SetEvent(vez[0]);

    //Fecha tudo
    WaitForMultipleObjects(2, threads, TRUE, INFINITE);

    CloseHandle(threads[0]);
    CloseHandle(threads[1]);
    CloseHandle(mutex);
    CloseHandle(vez[0]);  CloseHandle(vez[1]);
    CloseHandle(resp[0]); CloseHandle(resp[1]);

    closesocket(jogador1);
    closesocket(jogador2);
    closesocket(sock);
    WSACleanup();

    printf("[Servidor] Encerrado. Pressione ENTER para fechar.\n");
    getchar();
    return 0;
}