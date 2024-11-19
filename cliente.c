// client.c
// Cliente para o serviço de bate-papo com notificações de status
// Compilar com: gcc -Wall -o cliente cliente.c -pthread
// Executar com: ./client <IP_DO_SERVIDOR> <PORTA_TCP>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>

// Definições de porta e buffer
#define BUFFER_SIZE 1024

// Nickname do usuário
char nickname[50];

// Lista de usuários conectados
char user_list[BUFFER_SIZE];

// Variável global para sinalizar o encerramento
volatile sig_atomic_t running = 1;

// Função para receber mensagens TCP (bate-papo)
void *receive_tcp_messages(void *arg) {
    int tcp_sock = *((int *)arg);
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (running) {
        bytes_received = recv(tcp_sock, buffer, BUFFER_SIZE, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("%s\n", buffer);
        } else if (bytes_received == 0) {
            // Conexão fechada pelo servidor
            printf("Conexão encerrada pelo servidor.\n");
            running = 0;
            break;
        } else {
            // Verifica se o erro é devido ao socket fechado
            if (errno == EBADF || errno == ECONNRESET || errno == ENOTCONN) {
                // Socket fechado, encerra a thread
                break;
            } else {
                perror("recv");
            }
        }
    }

    pthread_exit(NULL);
}

// Função para receber notificações UDP (status)
void *receive_udp_notifications(void *arg) {
    int udp_sock = *((int *)arg);
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int bytes_received;

    while (running) {
        bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0,
                                  (struct sockaddr *)&server_addr, &addr_len);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("[NOTIFICAÇÃO]: %s\n", buffer);

            // Atualiza a lista de usuários se for uma mensagem de atualização
            if (strstr(buffer, "Usuários conectados:\n") != NULL) {
                strcpy(user_list, buffer);
            }
        } else if (bytes_received == 0) {
            // Não é comum em UDP, mas por segurança
            break;
        } else {
            // Verifica se o erro é devido ao socket fechado
            if (errno == EBADF || errno == ECONNRESET || errno == ENOTCONN) {
                // Socket fechado, encerra a thread
                break;
            } else {
                perror("recvfrom");
            }
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int tcp_sock, udp_sock;
    struct sockaddr_in tcp_server_addr;
    pthread_t tcp_thread, udp_thread;
    char message[BUFFER_SIZE];

    if (argc != 3) {
        printf("Uso: %s <IP_DO_SERVIDOR> <PORTA_TCP>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Solicita o nickname do usuário
    printf("Digite seu nickname: ");
    fgets(nickname, sizeof(nickname), stdin);
    nickname[strcspn(nickname, "\n")] = '\0'; // Remove o newline

    // Cria socket TCP
    if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket TCP");
        exit(EXIT_FAILURE);
    }

    // Configurações do servidor TCP
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_port = htons(atoi(argv[2]));
    tcp_server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    // Conecta ao servidor TCP
    if (connect(tcp_sock, (struct sockaddr *)&tcp_server_addr, sizeof(tcp_server_addr)) < 0) {
        perror("Erro ao conectar ao servidor TCP");
        exit(EXIT_FAILURE);
    }

    // Cria socket UDP
    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket UDP");
        exit(EXIT_FAILURE);
    }

    // Configurações do socket UDP para bind em qualquer porta (porta 0)
    struct sockaddr_in udp_client_addr;
    udp_client_addr.sin_family = AF_INET;
    udp_client_addr.sin_port = htons(0); // Porta 0 para o sistema escolher
    udp_client_addr.sin_addr.s_addr = INADDR_ANY;

    // Faz o bind do socket UDP
    if (bind(udp_sock, (struct sockaddr *)&udp_client_addr, sizeof(udp_client_addr)) < 0) {
        perror("Erro ao fazer bind no socket UDP");
        exit(EXIT_FAILURE);
    }

    // Obtém a porta UDP atribuída
    socklen_t udp_len = sizeof(udp_client_addr);
    if (getsockname(udp_sock, (struct sockaddr *)&udp_client_addr, &udp_len) == -1) {
        perror("Erro ao obter informações do socket UDP");
        exit(EXIT_FAILURE);
    }
    int udp_port = ntohs(udp_client_addr.sin_port);

    // Envia o nickname e a porta UDP ao servidor
    char initial_info[BUFFER_SIZE];
    sprintf(initial_info, "%s:%d", nickname, udp_port);
    if (send(tcp_sock, initial_info, strlen(initial_info), 0) < 0) {
        perror("Erro ao enviar informações iniciais ao servidor");
        exit(EXIT_FAILURE);
    }

    // Inicia thread para receber mensagens TCP
    if (pthread_create(&tcp_thread, NULL, receive_tcp_messages, (void *)&tcp_sock) != 0) {
        perror("Erro ao criar thread TCP");
        exit(EXIT_FAILURE);
    }

    // Inicia thread para receber notificações UDP
    if (pthread_create(&udp_thread, NULL, receive_udp_notifications, (void *)&udp_sock) != 0) {
        perror("Erro ao criar thread UDP");
        exit(EXIT_FAILURE);
    }

    // Loop para enviar mensagens ao servidor
    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        message[strcspn(message, "\n")] = '\0'; // Remove o newline

        // Verifica se o usuário deseja sair
        if (strcmp(message, "/sair") == 0) {
            printf("Você saiu do chat.\n");
            running = 0; // Sinaliza para as threads que o programa está encerrando

            // Encerra a conexão TCP
            shutdown(tcp_sock, SHUT_RDWR);
            close(tcp_sock);

            // Encerra o socket UDP
            close(udp_sock);

            // Espera as threads terminarem
            pthread_join(tcp_thread, NULL);
            pthread_join(udp_thread, NULL);

            exit(0);
        }

        // Envia a mensagem ao servidor TCP
        if (send(tcp_sock, message, strlen(message), 0) < 0) {
            perror("Erro ao enviar mensagem ao servidor");
        }
    }

    // Fecha os sockets
    close(tcp_sock);
    close(udp_sock);

    return 0;
}
