// gcc -Wall cliente.c -o cliente
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

void receber_resposta_completa(int sock, FILE *saida) {
    char buffer[256];
    int bytes_recebidos;
    fd_set readfds;
    int done = 0;

    while (!done) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        int ret = select(sock + 1, &readfds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(sock, &readfds)) {
            bytes_recebidos = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes_recebidos > 0) {
                buffer[bytes_recebidos] = '\0';
                fputs(buffer, saida);
                fflush(saida);
                if (strstr(buffer, "-------------------------------") != NULL) {
                    done = 1;
                }
            } else if (bytes_recebidos == 0) {
                // Conexão fechada
                done = 1;
            } else {
                perror("recv");
                done = 1;
            }
        }
    }
}

void processar_entrada_e_resposta(int sock1, int sock2, FILE *entrada, FILE *saida) {
    char buffer[256];

    // Receber e gravar resposta completa do servidor 1 usando select()
    receber_resposta_completa(sock1, saida);

    // Enviar dados para o servidor 1
    fseek(entrada, 0, SEEK_SET);
    while (fgets(buffer, sizeof(buffer), entrada)) {
        send(sock1, buffer, strlen(buffer), 0);
        fputs(buffer, saida);
    }
    fputs("\n-------------------------------\n", saida);
    fflush(saida);

    // Indicar que não enviaremos mais dados para o servidor 1
    shutdown(sock1, SHUT_WR);

    // Receber e gravar resposta completa do servidor 2 usando select()
    receber_resposta_completa(sock2, saida);

    // Enviar dados para o servidor 2
    fseek(entrada, 0, SEEK_SET);
    while (fgets(buffer, sizeof(buffer), entrada)) {
        send(sock2, buffer, strlen(buffer), 0);
        fputs(buffer, saida);
    }
    fflush(saida);

    // Indicar que não enviaremos mais dados para o servidor 2
    shutdown(sock2, SHUT_WR);
}

int conectar_servidor(char *ip, int porta) {
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(porta);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao conectar ao servidor");
        exit(1);
    }

    return sock;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Uso: %s <IP> <porta1> <porta2> <arquivo_entrada> <arquivo_saida>\n", argv[0]);
        exit(1);
    }

    char *ip = argv[1];
    int porta1 = atoi(argv[2]);
    int porta2 = atoi(argv[3]);
    char *arquivo_entrada = argv[4];
    char *arquivo_saida = argv[5];

    FILE *entrada = fopen(arquivo_entrada, "r");
    FILE *saida = fopen(arquivo_saida, "w");

    if (entrada == NULL || saida == NULL) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }

    int sock1 = conectar_servidor(ip, porta1);
    int sock2 = conectar_servidor(ip, porta2);

    processar_entrada_e_resposta(sock1, sock2, entrada, saida);

    fclose(entrada);
    fclose(saida);
    close(sock1);
    close(sock2);

    return 0;
}
