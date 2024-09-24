// Para compilar o arquivo, utilize: gcc -Wall cliente.c -o cliente

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXLINE 4096
#define BUFFER_SIZE 8192

int main(int argc, char **argv) {
    int    sockfd, n;
    char   recvline[MAXLINE + 1];
    char   error[MAXLINE + 1];
    struct sockaddr_in servaddr;
    socklen_t len;
    char buffer[BUFFER_SIZE] = {0};

    if (argc != 2) {
        strcpy(error,"uso: ");
        strcat(error,argv[0]);
        strcat(error," <IPaddress>");
        perror(error);
        exit(1);
    }

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(1);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // Valor escolhido para porta no servidor
    servaddr.sin_port   = htons(53348);
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton error");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(1);
    }

    len = sizeof(servaddr);
    // Pega o endereço IP e porta utilizado pelo servidor
    if (getsockname(sockfd, (struct sockaddr *)&servaddr, &len) == -1) {
        perror("getsockname");
        exit(1);
    }

    // Imprime endereço e porta
    printf("Servidor rodando no endereço %s\n", inet_ntoa(servaddr.sin_addr));
    printf("Servidor rodando na porta %d\n", ntohs(servaddr.sin_port));
    
    // Entrar no loop e esperar mensagem do servidor
    while(1) {
        // Limpar o buffer antes de ler nova mensagem
        memset(buffer, 0, sizeof(buffer));

        // Receber a tarefa do servidor
        read(sockfd, buffer, sizeof(buffer));
        buffer[strcspn(buffer, "\r\n")] = 0;  // Limpar nova linha
        printf("Tarefa recebida: %s\n", buffer);

        // Verificar se a mensagem é "ENCERRAR"
        if (strcmp(buffer, "ENCERRAR") == 0) {
            printf("Encerrando conexão...\n");
            // Enviar resposta ao servidor
            char connection_closed[MAXLINE];
            strcpy(connection_closed, "Conexão encerrada");
            send(sockfd, connection_closed, strlen(connection_closed), 0);
            break;
        }

        // Simular execucao da tarefa com sleep
        sleep(5);

        // Guardar o valor anterior do buffer
        char previous_task[MAXLINE];
        strcpy(previous_task, buffer);  // Copiar o valor atual do buffer para 'previous_task'
        snprintf(buffer, sizeof(buffer), "Tarefa %s concluída", previous_task);
        
        // Enviar resposta ao servidor
        send(sockfd, buffer, strlen(buffer), 0);
    }

    while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;
        if (fputs(recvline, stdout) == EOF) {
            perror("fputs error");
            exit(1);
        }
    }

    if (n < 0) {
        perror("read error");
        exit(1);
    }

    exit(0);
}
