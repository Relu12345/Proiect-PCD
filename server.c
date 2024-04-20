#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include "connection.h"

int main() {
    int status = remove(SOCKET_NAME);

    if( status == 0 )
        printf("%s file deleted successfully.\n", SOCKET_NAME);
    else
    {
        perror("can't delete file ");
    }

    int serv_sock, client_sock;
    serv_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        perror('socket failed');
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    if (bind(serv_sock, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serv_sock, 5) == -1) {
        perror('listen failed');
        exit(EXIT_FAILURE);
    }
    for (;;) {
        client_sock = accept(serv_sock, NULL, NULL);
        if (client_sock == -1) {
            perror('accept failed');
            exit(EXIT_FAILURE);
        }

        for (;;) {
            
            char message[200];
            char msg[] = "Hello\n";
            if (recv(client_sock, message, sizeof(message), 0) == -1) {
                perror('recieve failed');
                exit(EXIT_FAILURE);
            }
            printf("mesaj de la client: %s\n", message);

            memset(message, 0, sizeof(message));
        }
        close(client_sock);
    }
    close(serv_sock);
}