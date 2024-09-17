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

        // ticks = time(NULL);
        // Pega informações de IP e porta do cliente
        if (getpeername(connfd, (struct sockaddr *)&clientaddr, &len_client) < 0) {
            perror("getpeername");
            exit(EXIT_FAILURE);
        }

        // Imprime informações IP e porta do cliente
        printf("IP remoto %s\n", inet_ntoa(clientaddr.sin_addr));
        printf("Porta remota %d\n", ntohs(clientaddr.sin_port));
        
        // snprintf(buf, sizeof(buf), "Hello from server!\nTime: %.24s\r\n", ctime(&ticks));

        // write(connfd, buf, strlen(buf));

        // Criar um processo filho para cada cliente
        if (fork() == 0) {

            for(int i=0; i<3; i++) {
                // Enviar tarefa para o cliente
                strcpy(buffer, "TAREFA: LIMPEZA\n");
                send(connfd, buffer, strlen(buffer), 0);
                printf("Enviou tarefa para o cliente: %s\n", buffer);
                // Receber resposta do cliente
                read(connfd, buffer, sizeof(buffer));
                printf("Resposta - Cliente IP %s, Porta %d: %s\n", 
                inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buffer);

                // Log das interações em arquivo
                FILE *log = fopen("log.txt", "a");
                fprintf(log, "Cliente IP %s, Porta %d: Tarefa Enviada: LIMPEZA, Resposta: %s\n",
                        inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buffer);
                fclose(log);
            }

            // Encerrar
            strcpy(buffer, "ENCERRAR\n");
            send(connfd, buffer, strlen(buffer), 0);



            // Fechar conexão após finalizar
            close(connfd);
            exit(0);
        }

        // Receber e imprimir a mensagem do cliente
        // valread = read(connfd, buffer, BUFFER_SIZE);
        // if (valread > 0) {
        //     printf("Mensagem recebida do cliente: %s\n", buffer);
        // }

        close(connfd);
    }
    return(0);
}
