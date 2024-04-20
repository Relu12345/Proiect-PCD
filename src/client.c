#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <string.h>
#include "connection.h"

int main() {
    int client_sock;
    client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_sock == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    if (connect(client_sock, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    char send_message[MESSAGE_SIZE];
    char recv_message[MESSAGE_SIZE];
    size_t len;   

    for (;;) {
        memset(send_message, 0, sizeof(send_message));
        memset(recv_message, 0, sizeof(recv_message));

        if (recv(client_sock, recv_message, sizeof(recv_message), 0) == -1) {
            perror("receive failed");
            exit(EXIT_FAILURE);
        }

        printf("%s\n", recv_message);

        fgets(send_message, sizeof(send_message), stdin);
        len = strlen(send_message);
        if (send_message[len - 1] == '\n')
        {
            send_message[len - 1] = '\0';
        }
        
        if (send(client_sock, send_message, strlen(send_message), 0) == -1) {
            perror("send failed");
            exit(EXIT_FAILURE);
        }
        
    }
    close(client_sock);
    return 0;
}