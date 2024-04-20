#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include "connection.h"

void *client_handler(void *arg) {
    int client_sock = *((int *)arg);
    char message[MESSAGE_SIZE];

    for (;;) {
        ssize_t bytes_received = recv(client_sock, message, sizeof(message), 0);
        if (bytes_received == -1) {
            perror("receive failed");
            close(client_sock);
            pthread_exit(NULL);
        } else if (bytes_received == 0) {
            printf("Client disconnected.\n");
            close(client_sock);
            pthread_exit(NULL);
        } else {
            printf("Message from client: %s\n", message);
            // Handle the received message here as needed
        }
    }
}

int main() {
    int status = remove(SOCKET_NAME);
    if (status == 0)
        printf("%s file deleted successfully.\n", SOCKET_NAME);

    int serv_sock, client_sock;
    serv_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    if (bind(serv_sock, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serv_sock, SOCKETS_AVAILABLE) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    pthread_t thread_id;
    for (;;) {
        client_sock = accept(serv_sock, NULL, NULL);
        if (client_sock == -1) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&thread_id, NULL, client_handler, (void *)&client_sock) != 0) {
            perror("pthread_create failed");
            close(client_sock);
        }

        if (pthread_detach(thread_id) != 0) {
            perror("pthread_detach failed");
            close(client_sock);
        }
    }

    close(serv_sock);

    return 0;
}