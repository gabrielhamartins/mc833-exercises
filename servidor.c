// servidor.c
// Servidor para o serviço de bate-papo com notificações de status utilizando select()
// Compilar com: gcc -Wall -o servidor servidor.c
// Executar com: ./servidor <PORTA_TCP>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>


#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

// Estrutura para armazenar informações do cliente
typedef struct {
    int sockfd;                    // Descriptor do socket TCP do cliente
    char nickname[50];             // Nickname do cliente
    struct sockaddr_in addr;       // Endereço do cliente (para UDP)
    int udp_port;                  // Porta UDP do cliente
} client_t;

// Lista de clientes conectados
client_t *clients[MAX_CLIENTS];
int client_count = 0;

// Função para adicionar um cliente à lista
void add_client(client_t *cl) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            client_count++;
            break;
        }
    }
}

// Função para remover um cliente da lista
void remove_client(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->sockfd == sockfd) {
                clients[i] = NULL;
                client_count--;
                break;
            }
        }
    }
}

// Função para enviar mensagens TCP para todos os clientes
void send_message_to_all(char *message, int sender_sockfd) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->sockfd != sender_sockfd) {
                if (send(clients[i]->sockfd, message, strlen(message), 0) < 0) {
                    perror("Erro ao enviar mensagem");
                }
            }
        }
    }
}

// Função para enviar notificações UDP para todos os clientes
void send_udp_notification(char *message) {
    int udp_sock;
    struct sockaddr_in client_addr;

    // Cria socket UDP
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket UDP");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            client_addr.sin_family = AF_INET;
            client_addr.sin_addr = clients[i]->addr.sin_addr;
            client_addr.sin_port = htons(clients[i]->udp_port);

            // Envia a mensagem UDP para o cliente
            if (sendto(udp_sock, message, strlen(message), 0,
                       (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
                perror("Erro ao enviar notificação UDP");
            }
        }
    }

    close(udp_sock);
}

// Função para obter a lista de usuários conectados
void get_user_list(char *list) {
    strcpy(list, "Usuários conectados:\n");
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            strcat(list, "- ");
            strcat(list, clients[i]->nickname);
            strcat(list, "\n");
        }
    }
}

// Função para registrar eventos no log
void log_event(char *event) {
    FILE *log_file = fopen("server.log", "a");
    if (!log_file) {
        perror("Erro ao abrir o arquivo de log");
        return;
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);

    // Remove o newline do timestamp
    timestamp[strcspn(timestamp, "\n")] = '\0';

    fprintf(log_file, "[%s] %s\n", timestamp, event);
    fclose(log_file);
}

int main(int argc, char *argv[]) {
    int tcp_sock, udp_sock, max_sd, activity;
    struct sockaddr_in tcp_server_addr, client_addr, udp_server_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE], message[BUFFER_SIZE];
    fd_set readfds;

    if (argc != 2) {
        printf("Uso: %s <PORTA_TCP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Inicializa o log
    FILE *log_file = fopen("server.log", "w");
    if (!log_file) {
        perror("Erro ao criar o arquivo de log");
        exit(EXIT_FAILURE);
    }
    fclose(log_file);

    // Cria socket TCP
    if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket TCP");
        exit(EXIT_FAILURE);
    }

    // Configurações do servidor TCP
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_port = htons(atoi(argv[1]));
    tcp_server_addr.sin_addr.s_addr = INADDR_ANY;

    // Faz o bind do socket TCP
    if (bind(tcp_sock, (struct sockaddr *)&tcp_server_addr, sizeof(tcp_server_addr)) < 0) {
        perror("Erro ao fazer bind no socket TCP");
        exit(EXIT_FAILURE);
    }

    // Inicia a escuta por conexões TCP
    if (listen(tcp_sock, 10) < 0) {
        perror("Erro ao iniciar listen");
        exit(EXIT_FAILURE);
    }

    // Cria socket UDP para receber notificações (opcional)
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket UDP");
        exit(EXIT_FAILURE);
    }

    // Configurações do servidor UDP (opcional)
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_port = htons(0); // Porta 0, não precisamos receber dados
    udp_server_addr.sin_addr.s_addr = INADDR_ANY;

    // Faz o bind do socket UDP (opcional)
    if (bind(udp_sock, (struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr)) < 0) {
        perror("Erro ao fazer bind no socket UDP");
        exit(EXIT_FAILURE);
    }

    printf("Servidor iniciado na porta %s.\n", argv[1]);

    // Inicializa o array de clientes
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = NULL;
    }

    while (1) {
        // Limpa o conjunto de descritores de leitura
        FD_ZERO(&readfds);

        // Adiciona o socket principal TCP ao conjunto
        FD_SET(tcp_sock, &readfds);
        max_sd = tcp_sock;

        // Adiciona os sockets dos clientes ao conjunto
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd;
            if (clients[i]) {
                sd = clients[i]->sockfd;
                FD_SET(sd, &readfds);

                if (sd > max_sd)
                    max_sd = sd;
            }
        }

        // Aguarda por atividade em um dos sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            perror("Erro no select");
        }

        // Verifica se há nova conexão no socket principal
        if (FD_ISSET(tcp_sock, &readfds)) {
            int new_sock;
            if ((new_sock = accept(tcp_sock, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
                perror("Erro ao aceitar conexão");
                continue;
            }

            // Recebe as informações iniciais do cliente (nickname e porta UDP)
            int bytes_received = recv(new_sock, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                printf("Não foi possível obter as informações iniciais do cliente.\n");
                close(new_sock);
                continue;
            }

            buffer[bytes_received] = '\0';

            // Configura o novo cliente
            client_t *cli = (client_t *)malloc(sizeof(client_t));
            cli->sockfd = new_sock;
            cli->addr = client_addr;

            // Divide o nickname e a porta UDP
            char *token = strtok(buffer, ":");
            if (token != NULL) {
                strcpy(cli->nickname, token);
                token = strtok(NULL, ":");
                if (token != NULL) {
                    cli->udp_port = atoi(token);
                } else {
                    printf("Porta UDP não fornecida pelo cliente.\n");
                    close(cli->sockfd);
                    free(cli);
                    continue;
                }
            } else {
                printf("Formato inválido das informações iniciais do cliente.\n");
                close(cli->sockfd);
                free(cli);
                continue;
            }

            // Atualiza o endereço UDP do cliente com a porta correta
            cli->addr.sin_family = AF_INET;
            cli->addr.sin_port = htons(cli->udp_port);
            // O endereço IP já está preenchido em cli->addr.sin_addr pelo accept()

            // Adiciona o cliente à lista
            add_client(cli);

            // Registra o evento de conexão
            char log_msg[BUFFER_SIZE];
            sprintf(log_msg, "Cliente %s conectado.", cli->nickname);
            log_event(log_msg);

            printf("%s se conectou.\n", cli->nickname);

            // Envia a lista de usuários conectados ao novo cliente
            char user_list[BUFFER_SIZE];
            get_user_list(user_list);
            if (send(cli->sockfd, user_list, strlen(user_list), 0) < 0) {
                perror("Erro ao enviar lista de usuários");
            }

            // Envia notificação de novo usuário para todos os clientes via UDP
            sprintf(message, "%s entrou no chat.", cli->nickname);
            send_udp_notification(message);
        }

        // Verifica se há atividade em algum socket de cliente
        for (int i = 0; i < MAX_CLIENTS; i++) {
            client_t *cli = clients[i];
            if (cli) {
                int sd = cli->sockfd;
                if (FD_ISSET(sd, &readfds)) {
                    // Recebe mensagem do cliente
                    int bytes_received = recv(sd, buffer, BUFFER_SIZE, 0);
                    if (bytes_received <= 0) {
                        // Cliente desconectou
                        char log_msg[BUFFER_SIZE];
                        sprintf(log_msg, "Cliente %s desconectado.", cli->nickname);
                        log_event(log_msg);

                        printf("%s se desconectou.\n", cli->nickname);

                        // Envia notificação de saída para todos os clientes via UDP
                        sprintf(message, "%s saiu do chat.", cli->nickname);
                        send_udp_notification(message);

                        close(sd);
                        remove_client(sd);
                        free(cli);
                        clients[i] = NULL;
                    } else {
                        buffer[bytes_received] = '\0';

                        // Prepara a mensagem para encaminhar
                        sprintf(message, "%s: %s", cli->nickname, buffer);

                        // Registra a mensagem no log
                        log_event(message);

                        // Encaminha a mensagem para todos os clientes
                        send_message_to_all(message, sd);
                    }
                }
            }
        }
    }

    // Fecha o socket principal
    close(tcp_sock);

    return 0;
}
