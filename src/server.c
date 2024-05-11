#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <ctype.h>
#include "connection.h"
#include "opencv_wrapper.h"
#include "login.h"

typedef struct {
    int id;
    int sock_fd;
} ClientInfo;

int client_id = 0;

void *client_handler(void *arg) {
    ClientInfo *client_info = (ClientInfo *)arg;
    int client_sock = client_info->sock_fd;
    int id = client_info->id;

    printf("Client %d connected.\n", id);

    char username[100];
    char password[100];

    // User and password combined with a comma ","
    char message[205];

    char char_choice[2];

    // Reseting the variables
    memset(username, 0, sizeof(username));
    memset(password, 0, sizeof(password));
    memset(message, 0, sizeof(message));
    memset(char_choice, 0, sizeof(char_choice));

    // we get the choice from the client (if they are registering or logging in)
    recv(client_sock, char_choice, sizeof(char_choice) - 1, 0);
    char_choice[1] = '\0';

    int choice = atoi(char_choice);

    printf("Choice char: %s\n", char_choice);

    // Primim user-ul si parola de la client
    recv(client_sock, message, 205, 0);
    printf("Username and password from client: %s\n", message);

    processClientInfo(message, username, password);

    printf("Username: %s\nPassword: %s\nChoice: %s", username, password, char_choice);

    // 0 means register, 1 means login
    if (strcmp(char_choice, "0") == 0) {
        int registerResult = create_user(username, password);
        if (registerResult == 0) {
            // Successful register, we send a signal to the client to say this
            send(client_sock, "SUCCESS", 7, 0);
            printf("Successful register\n");
        }
        else {
            // Invalid register, we send a signal to the client to say this
            send(client_sock, "FAIL", 4, 0);
            printf("Invalid register\n");
            close(client_sock);
            free(client_info);
            pthread_exit(NULL);
        }
    }
    if (strcmp(char_choice, "1") == 0) {
        int loginResult = login(username, password);
        if (loginResult == 0) {
            // Successful login, we send a signal to the client to say this
            send(client_sock, "SUCCESS", 7, 0);
            printf("Successful login\n");
        } else {
            // Invalid login, we send a signal to the client to say this
            send(client_sock, "FAIL", 4, 0);
            printf("Invalid login\n");
            close(client_sock);
            free(client_info);
            pthread_exit(NULL);
        }
    }

    // Receive image data size from client
    long dataSize;
    if (recv(client_sock, &dataSize, sizeof(long), 0) <= 0) {
        perror("Failed to receive image data size from client");
        close(client_sock);
        free(client_info);
        pthread_exit(NULL);
    }

    printf("dataSize: %ld\n", dataSize);

    // Receive image data (grayscale byte array) from the client
    unsigned char* imageData = (unsigned char*)malloc(dataSize);
    if (imageData == NULL) {
        perror("Failed to allocate memory for image data");
        close(client_sock);
        free(client_info);
        pthread_exit(NULL);
    }

    // Receive image data from client
    if (recv(client_sock, imageData, dataSize, 0) <= 0) {
        perror("Failed to receive image data from client");
        free(imageData);
        close(client_sock);
        free(client_info);
        pthread_exit(NULL);
    }

    // Convert image data to grayscale
    int width, height;
    unsigned char* grayscaleData = convertBytesToGrayscale(imageData, dataSize, &width, &height);

    printf("Width: %d\nHeight: %d\n", width, height);
    
    // Send grayscale data back to client
    send(client_sock, &width, sizeof(int), 0);
    send(client_sock, &height, sizeof(int), 0);
    send(client_sock, grayscaleData, width * height * sizeof(unsigned char), 0);

    printf("Width: %d\nHeight: %d\n", width, height);

    // Free memory
    free(imageData);
    free(grayscaleData);

    // Close socket and free resources
    printf("Client %d disconnected.\n", id);
    free(client_info);
    pthread_exit(NULL);
}

void *unix_server_thread(void *arg) {
    int status = remove(SOCKET_NAME);

    if(status == 0)
        printf("%s file deleted successfully.\n", SOCKET_NAME);

    int serv_unix_sock, client_unix_sock;
    struct sockaddr_un unix_addr;
    pthread_t thread_id;

    serv_unix_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serv_unix_sock == -1) {
        perror("socket failed");
        pthread_exit(NULL);
    }

    memset(&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    strncpy(unix_addr.sun_path, SOCKET_NAME, sizeof(unix_addr.sun_path) - 1);
    if (bind(serv_unix_sock, (const struct sockaddr *)&unix_addr, sizeof(unix_addr)) == -1) {
        perror("bind failed");
        close(serv_unix_sock);
        pthread_exit(NULL);
    }

    if (listen(serv_unix_sock, SOCKETS_AVAILABLE) == -1) {
        perror("listen failed");
        close(serv_unix_sock);
        pthread_exit(NULL);
    }

    for (;;) {
        client_unix_sock = accept(serv_unix_sock, NULL, NULL);
        if (client_unix_sock != -1) {
            ClientInfo *client_info = (ClientInfo *)malloc(sizeof(ClientInfo));
            if (client_info == NULL) {
                perror("malloc failed");
                close(client_unix_sock);
                continue;
            }

            client_info->id = client_id++;
            client_info->sock_fd = client_unix_sock;

            if (pthread_create(&thread_id, NULL, client_handler, (void *)client_info) != 0) {
                perror("pthread_create failed");
                close(client_unix_sock);
            }

            if (pthread_detach(thread_id) != 0) {
                perror("pthread_detach failed");
                close(client_unix_sock);
            }
        }
    }

    close(serv_unix_sock);
    pthread_exit(NULL);
}

void *inet_server_thread(void *arg) {
    int serv_inet_sock, client_inet_sock;
    struct sockaddr_in inet_addr;
    pthread_t thread_id;

    serv_inet_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_inet_sock == -1) {
        perror("socket failed");
        pthread_exit(NULL);
    }

    memset(&inet_addr, 0, sizeof(inet_addr));
    inet_addr.sin_family = AF_INET;
    inet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_addr.sin_port = htons(INET_PORT);
    if (bind(serv_inet_sock, (const struct sockaddr *)&inet_addr, sizeof(inet_addr)) == -1) {
        perror("bind failed");
        close(serv_inet_sock);
        pthread_exit(NULL);
    }

    if (listen(serv_inet_sock, SOCKETS_AVAILABLE) == -1) {
        perror("listen failed");
        close(serv_inet_sock);
        pthread_exit(NULL);
    }

    for (;;) {
        client_inet_sock = accept(serv_inet_sock, NULL, NULL);
        if (client_inet_sock != -1) {
            ClientInfo *client_info = (ClientInfo *)malloc(sizeof(ClientInfo));
            if (client_info == NULL) {
                perror("malloc failed");
                close(client_inet_sock);
                continue;
            }

            client_info->id = client_id++;
            client_info->sock_fd = client_inet_sock;

            if (pthread_create(&thread_id, NULL, client_handler, (void *)client_info) != 0) {
                perror("pthread_create failed");
                close(client_inet_sock);
            }

            if (pthread_detach(thread_id) != 0) {
                perror("pthread_detach failed");
                close(client_inet_sock);
            }
        }
    }

    close(serv_inet_sock);
    pthread_exit(NULL);
}

int main() {
    pthread_t unix_thread_id, inet_thread_id;

    if (pthread_create(&unix_thread_id, NULL, unix_server_thread, NULL) != 0) {
        perror("pthread_create for UNIX server failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_detach(unix_thread_id) != 0) {
        perror("pthread_detach for UNIX server failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&inet_thread_id, NULL, inet_server_thread, NULL) != 0) {
        perror("pthread_create for INET server failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_detach(inet_thread_id) != 0) {
        perror("pthread_detach for INET server failed");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        sleep(1);
    }

    return 0;
}