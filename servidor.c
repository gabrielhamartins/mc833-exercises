// gcc -Wall servidor.c -o servidor
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

void enviar_monitoramento(int client_sock, char* ip, int porta) {
    char buffer[256];

    // Gera horário formatado com o tempo atual
    time_t agora = time(NULL);
    struct tm *horario = localtime(&agora);
    char horario_str[100];
    strftime(horario_str, sizeof(horario_str), "%a %b %d %H:%M:%S %Y", horario);

    // Inicializa o gerador de números aleatórios com uma semente única
    srand(time(NULL) ^ client_sock ^ getpid());

    // Gera valores aleatórios para CPU, memória e status
    int cpu = rand() % 101;       // CPU aleatório entre 0 e 100
    int memoria = rand() % 101;    // Memória aleatória entre 0 e 100
    char* status = (rand() % 2 == 0) ? "Ativo" : "Inativo";

    // Formata a string de monitoramento com valores específicos para cada conexão
    snprintf(buffer, sizeof(buffer),
        "Monitoramento do servidor:\n"
        "IP: %s\n"
        "Porta: %d\n"
        "Horário: %s\n"
        "CPU: %d%%\n"
        "Memória: %d%%\n"
        "Status: %s\n"
        "-------------------------------\n",
        ip, porta, horario_str, cpu, memoria, status);

    // Envia o monitoramento para o cliente
    send(client_sock, buffer, strlen(buffer), 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(1);
    }

    int porta = atoi(argv[1]);
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Cria o socket do servidor
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(porta);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("Servidor iniciado na porta %d\n", porta);

    while (1) {
        // Aceita uma nova conexão de cliente
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);

        if (client_sock >= 0) {
            // Obtém o IP do cliente
            char *ip_cliente = inet_ntoa(client_addr.sin_addr);
            
            // Gera e envia o monitoramento específico para essa conexão
            enviar_monitoramento(client_sock, ip_cliente, porta);

            char buffer[256];
            int bytes_recebidos;

            // Recebe e ecoa mensagens do cliente até a desconexão
            while ((bytes_recebidos = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
                buffer[bytes_recebidos] = '\0';
                send(client_sock, buffer, bytes_recebidos, 0); // Envia o eco ao cliente
            }

            // Fecha a conexão com o cliente
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}
