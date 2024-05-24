#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "connection.h"

int main() {
    int sock;
    struct sockaddr_un addr;
    char buffer[CHUNK_SIZE];
    
    // Create socket
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    
    // Set up address
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
    
    // Connect to the server
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect error");
        close(sock);
        exit(EXIT_FAILURE);
    }
    
    // Send admin password
    if (send(sock, ADMIN_PASSWORD, strlen(ADMIN_PASSWORD), 0) == -1) {
        perror("send error");
        close(sock);
        exit(EXIT_FAILURE);
    }

    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Server response: %s\n", buffer);
        if (strcmp(buffer, "Admin already connected") == 0) {
            close(sock);
            exit(EXIT_FAILURE);
        }
    } else {
        if (bytes_received == 0) {
            printf("Server closed the connection\n");
        } else {
            perror("recv error");
        }
        exit(EXIT_FAILURE);
    }

    // Interaction with the server
    char recv_message[CHUNK_SIZE];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        printf("1. List of users\n2. Users posts\n3. Delete post\n4. Ban user\n");
        printf("Enter option to send to the server (or 'exit' to quit): ");
        
        fgets(buffer, sizeof(buffer), stdin);
        
        buffer[strcspn(buffer, "\n")] = 0;
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        
        if (send(sock, buffer, strlen(buffer), 0) == -1) {
            perror("send error");
            break;
        }
        // printf("\nHElloo");
        int choice = atoi(buffer);
        int bytes_received;
        switch(choice) {
            case 1:
                bytes_received = recv(sock, recv_message, sizeof(recv_message), 0);
                if (bytes_received > 0) {
                    printf ("%s",recv_message);
                } else {
                    if (bytes_received == 0) {
                        printf("Server closed the connection\n");
                    } else {
                        perror("recv error");
                    }
                    break;
                }
                
                break;
            case 2:
                memset(buffer, 0, sizeof(buffer));
                printf ("\nChoose a user id: ");
                fgets(buffer, sizeof(buffer), stdin);
                send(sock, buffer, strlen(buffer), 0);
                bytes_received = recv(sock, recv_message, sizeof(recv_message), 0);
                if (bytes_received > 0) {
                    printf ("%s",recv_message);
                } else {
                    if (bytes_received == 0) {
                        printf("Server closed the connection\n");
                    } else {
                        perror("recv error");
                    }
                    break;
                }
                break;
            case 3:
                memset(buffer, 0, sizeof(buffer));
                printf ("\nChoose a post: ");
                fgets(buffer, sizeof(buffer), stdin);
                send(sock, buffer, strlen(buffer), 0);
                bytes_received = recv(sock, recv_message, sizeof(recv_message), 0);
                if (bytes_received > 0) {
                    printf ("%s",recv_message);
                } else {
                    if (bytes_received == 0) {
                        printf("Server closed the connection\n");
                    } else {
                        perror("recv error");
                    }
                    break;
                }
                break;
            case 4:
                break;
            default:
                break;
        }
    }

    // Close socket
    close(sock);
    return 0;
}
