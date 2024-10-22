// Para compilar o arquivo, utilize: gcc -Wall servidor.c -o servidor

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <signal.h>       // Para capturar SIGCHLD
#include <sys/wait.h>     // Para usar waitpid()

#define BACKLOG 0
#define MAXDATASIZE 100
#define BUFFER_SIZE 1024

// Função tratadora para SIGCHLD
void sigchld_handler(int signo) {
    // Espera qualquer processo filho que tenha terminado
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main (int argc, char **argv) {
    int    listenfd, connfd, backlog;
    struct sockaddr_in servaddr, clientaddr;
    socklen_t len, len_client;
    char buffer[BUFFER_SIZE] = {0};
    char message_received[BUFFER_SIZE];
    char message_sent[BUFFER_SIZE];


    // Configurar tratador de sinal para SIGCHLD
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;    // Define a função tratadora
    sigemptyset(&sa.sa_mask);           // Limpa a máscara de sinais
    sa.sa_flags = SA_RESTART;           // Reinicia chamadas interrompidas
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    

    // Verifica se o backlog foi passado como argumento
    if (argc > 1) {
        backlog = atoi(argv[1]);
        // printf("Argumento recebido: %d \n", backlog);
    } else {
        backlog = BACKLOG;
    }


    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // Utilizando a porta 53348
    // Para automatizar a escolha de uma porta utilizar `htons(0)`
    servaddr.sin_port        = htons(53348);   

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    len = sizeof(servaddr);
    // Pega o endereço da conexão
    if (getsockname(listenfd, (struct sockaddr *)&servaddr, &len) == -1) {
        perror("getsockname");
        exit(1);
    }

    // Imprime a porta utilizada pelo servidor
    printf("Servidor rodando na porta %d\n", ntohs(servaddr.sin_port));
    // int opt = 1;  // Nova variável

    if (listen(listenfd, backlog) < 0) {
        perror("listen");
        exit(1);
    }

    FILE *log = fopen("log.txt", "a");
    fprintf(log, "[PID %d] Processo pai iniciado\n", getpid());
    fclose(log);

    // Remova o comentário para testar com o script `multiplosClientes.sh`
    // sleep(10);

    for ( ; ; ) {
        len_client = sizeof(clientaddr);
        if ((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &len_client)) == -1 ) {
            perror("accept");
            exit(1);
        }
        
        // Pega informações de IP e porta do cliente
        if (getpeername(connfd, (struct sockaddr *)&clientaddr, &len_client) < 0) {
            perror("getpeername");
            exit(EXIT_FAILURE);
        }
        
        printf("Cliente conectado: %s:%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        // Criar um processo filho para cada cliente
        if (fork() == 0) {

            // Log de nova conexao
            FILE *log = fopen("log.txt", "a");
            fprintf(log, "[%s:%d][PID %d] Conectado\n",
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), getpid());
            fclose(log);
        
            // Remova o comentário para testar com o script `multiplosClientes.sh`
            // sleep(10);

            for(int i=0; i<3; i++) {
                // Enviar tarefa para o cliente
                strcpy(buffer, "LIMPEZA");
                send(connfd, buffer, strlen(buffer), 0);
                strcpy(message_sent, buffer);

                // Receber resposta do cliente
                read(connfd, buffer, sizeof(buffer));
                strcpy(message_received, buffer);

                // Log das interações em arquivo
                FILE *log = fopen("log.txt", "a");
                fprintf(log, "[%s:%d][PID %d] Tarefa Enviada: %s | Resposta: %s\n",
                        inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), getpid(), message_sent, message_received);
                fclose(log);
            }

            // Encerrar
            strcpy(buffer, "ENCERRAR");
            send(connfd, buffer, strlen(buffer), 0);

            strcpy(message_received, "ENCERRAR");

            // Limpar o buffer antes de ler nova mensagem
            memset(buffer, 0, sizeof(buffer));

            // Receber resposta do cliente
            read(connfd, buffer, sizeof(buffer));

            // Log das interações em arquivo
            FILE *logEnd = fopen("log.txt", "a");
            fprintf(logEnd, "[%s:%d][PID %d] Tarefa Enviada: %s | Resposta: %s\n",
                    inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), getpid(), message_received, buffer);
            fclose(logEnd);

            // Fechar conexão após finalizar
            close(connfd);
            exit(0);
        }
     close(connfd);
       
    }
     close(listenfd);
    return(0);
}
