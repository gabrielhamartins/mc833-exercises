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


#define LISTENQ 10
#define MAXDATASIZE 100
#define BUFFER_SIZE 1024

int main (int argc, char **argv) {
    int    listenfd, connfd; //valread;
    struct sockaddr_in servaddr, clientaddr;
    // char   buf[MAXDATASIZE];
    // time_t ticks;
    socklen_t len, len_client;
    char buffer[BUFFER_SIZE] = {0};
    char message_received[BUFFER_SIZE];
    char message_sent[BUFFER_SIZE];


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

    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen");
        exit(1);
    }

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

        // Log de nova conexao
        FILE *log = fopen("log.txt", "a");
        fprintf(log, "[%s:%d] Conectado\n",
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
        fclose(log);

        // Criar um processo filho para cada cliente
        if (fork() == 0) {

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
                fprintf(log, "[%s:%d] Tarefa Enviada: %s | Resposta: %s\n",
                        inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), message_sent, message_received);
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
            FILE *log = fopen("log.txt", "a");
            fprintf(log, "[%s:%d] Tarefa Enviada: %s | Resposta: %s\n",
                    inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), message_received, buffer);
            fclose(log);

            // Fechar conexão após finalizar
            close(connfd);
            exit(0);
        }

        close(connfd);
    }
    return(0);
}
