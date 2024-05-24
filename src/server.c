#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <ctype.h>
#include "connection.h"
#include "interface_wrapper.h"
#include "login.h"


typedef struct {
    int id;
    int sock_fd;
} ClientInfo;

int client_id = 0;
int admin_connected = 0;

void setPostServer(PGconn* conn, int client_sock, struct Post* posts, int id) {
    // Get the number of posts
    int num_posts = get_posts_counts(conn);

    // Send the number of posts
    send(client_sock, &num_posts, sizeof(int), 0);

    if (num_posts != 0) {
        size_t *imageSizes = malloc(sizeof(size_t) * num_posts);
        struct Post* current_post = posts;
        for (int i = 0; i < num_posts; i++) {
            imageSizes[i] = (strlen(current_post->image) + 1);
            current_post++;
        }

        send(client_sock, imageSizes, sizeof(size_t) * num_posts, 0);

        current_post = posts;
        // Iterate through the posts and send each post individually
        for (int i = 0; i < num_posts; i++) {
            send(client_sock, &(current_post->id), sizeof(int), 0);
            send(client_sock, &(current_post->userId), sizeof(int), 0);
            size_t imageSize = (strlen(current_post->image) + 1);
            send(client_sock, &imageSize, sizeof(size_t), 0);
            printf("sent image size: %zu\n", imageSize);
            size_t bytesSent = 0;
            while (bytesSent < imageSize) {
                size_t bytesToSend = (CHUNK_SIZE < imageSize - bytesSent) ? CHUNK_SIZE : (imageSize - bytesSent);
                int sent = send(client_sock, current_post->image + bytesSent, bytesToSend, 0);
                if (sent < 0) {
                    perror("Failed to send image data");
                    printf("Client %d disconnected.\n", id);
                    exit(EXIT_FAILURE);
                }
                bytesSent += sent;
            }
            // send(client_sock, current_post->image, imageSize, 0);
            int description_length = strlen(current_post->description) + 1; // Include null terminator
            send(client_sock, &description_length, sizeof(int), 0);
            send(client_sock, current_post->description, description_length, 0);
            int username_length = strlen(current_post->userName) + 1; // Include null terminator
            send(client_sock, &username_length, sizeof(int), 0);
            send(client_sock, current_post->userName, username_length, 0);
            send(client_sock, &(current_post->likeCount), sizeof(int), 0);
            send(client_sock, &(current_post->liked), sizeof(int), 0);
            current_post++;
        }
    }
}

void *client_handler(void *arg) {

    const char *connstring = "host=dpg-cohr28ol5elc73csm2i0-a.frankfurt-postgres.render.com port=5432 dbname=pcd user=pcd_user password=OAGPeU3TKCHQ3hePtl69HSQNb8DiBbls";
    PGconn *conn = NULL;

    conn = PQconnectdb(connstring);

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

    // Primim user-ul si parola de la client
    recv(client_sock, message, 205, 0);

    processClientInfo(message, username, password);

    printf("user and pass \n\n%s\n%s\n\n", username, password);

    struct Post* posts = NULL;
    struct User user;

    // 0 means register, 1 means login
    if (strcmp(char_choice, "0") == 0) {
        int registerResult = ps_register(conn, username, password);
        if (registerResult == true) {
            int loginResult = 1;
            // Successful register, we send a signal to the client to say this
            user = login(conn, username, password, &loginResult);
            posts = get_all_posts(conn, user.id);
            //setDatabase(conn, user.name, user.id);
            if (loginResult == 0) {
                // Successful login, we send a signal to the client to say this
                send(client_sock, "SUCCESS", 7, 0);
                printf("Successful login\n");
            } else {
                // Invalid login, we send a signal to the client to say this
                send(client_sock, "FAIL", 4, 0);
                printf("Invalid login\n");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }
        }
        else {
            // Invalid register, we send a signal to the client to say this
            send(client_sock, "FAIL", 4, 0);
            printf("Invalid register\n");
            printf("Client %d disconnected.\n", id);
            close(client_sock);
            free(client_info);
            pthread_exit(NULL);
        }
    }
    else if (strcmp(char_choice, "1") == 0) {
        int loginResult = 1;
        user = login(conn, username, password, &loginResult);
        posts = get_all_posts(conn, user.id);
        if (loginResult == 0) {
            // Successful login, we send a signal to the client to say this
            send(client_sock, "SUCCESS", 7, 0);
            printf("Successful login\n");
        } else {
            // Invalid login, we send a signal to the client to say this
            send(client_sock, "FAIL", 7, 0);
            printf("Invalid login\n");
            printf("Client %d disconnected.\n", id);
            close(client_sock);
            free(client_info);
            pthread_exit(NULL);
        }
    }

    // Send user id to client
    send(client_sock, &user.id, sizeof(user.id), 0);

    printf("user id: %d\n", user.id);

    // Send user name to client
    send(client_sock, user.name, sizeof(user.name), 0);

    printf("user name: %s\n", user.name);

    setPostServer(conn, client_sock, posts, id);

    char buffer[CHUNK_SIZE] = {0};
    char description[105] = {0};
    while (1) {
        memset(buffer, 0, sizeof(buffer));

        if (recv(client_sock, buffer, 1, 0) <= 0) {
            perror("Failed to receive signal header from client");
            printf("Client %d disconnected.\n", id);
            close(client_sock);
            free(client_info);
            pthread_exit(NULL);
        }
        printf("buffer: %s\n", buffer);

        if (strcmp(buffer, "P") == 0) {
            printf ("post signal received!\n");

            memset(buffer, 0, sizeof(buffer));
            memset(description, 0, sizeof(description));

            if (recv(client_sock, description, sizeof(description), 0) <= 0) {
                perror("Failed to receive description from client");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            printf("received message: %s\n", description);

            size_t image_size;
            if (recv(client_sock, &image_size, sizeof(size_t), 0) <= 0) {
                perror("Failed to receive image data size from client");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            printf("image size: %zu\n", image_size);

            // Receive image data (grayscale byte array) from the client
            unsigned char* imageData = (unsigned char*)malloc(image_size);
            if (imageData == NULL) {
                perror("Failed to allocate memory for image data");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            // Receive image data from client
            size_t bytesReceived = 0;
            while (bytesReceived < image_size) {
                size_t remainingBytes = image_size - bytesReceived;
                size_t bytesToReceive = remainingBytes > CHUNK_SIZE ? CHUNK_SIZE : remainingBytes;
                int received = recv(client_sock, imageData + bytesReceived, bytesToReceive, 0);
                if (received <= 0) {
                    if (received < 0) {
                        perror("Failed to receive image data");
                    } else {
                        printf("Connection closed by client\n");
                    }
                    printf("Client %d disconnected.\n", id);
                    free(imageData);
                    close(client_sock);
                    free(client_info);
                    pthread_exit(NULL);
                }
                bytesReceived += received;
            }

            if(!insertPost(conn, user.id, (void *)imageData, image_size, description)) {
                perror("erorrtiune");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            printf("Image inserted successfully!\n");

            free(imageData);
        }
        else if (strcmp(buffer, "G") == 0) {
            posts = get_all_posts(conn, user.id);
            setPostServer(conn, client_sock, posts, id);
        }
        else if (strcmp(buffer, "L") == 0) {
            int postId, userId;

            if (recv(client_sock, &postId, sizeof(postId), 0) <= 0) {
                perror("Failed to receive post id from client");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            if (recv(client_sock, &userId, sizeof(userId), 0) <= 0) {
                perror("Failed to receive user id from client");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            like_or_remove_like(conn, userId, postId);
        }
    }

    // Close socket and free resources
    printf("Client %d disconnected.\n", id);
    free(client_info);
    pthread_exit(NULL);
}

void *admin_handler(void *arg) {
    ClientInfo *client_info = (ClientInfo *)arg;
    int client_sock = client_info->sock_fd;
    int id = client_info->id;

    printf("Admin client connected.\n");

    char buffer[100];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            printf("Admin client disconnected.\n");
            close(client_sock);
            admin_connected = 0;
            free(client_info);
            pthread_exit(NULL);
        }
        printf("Admin: %s\n", buffer);
    }

    close(client_sock);
    admin_connected = 0;
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

            char buffer[100] = {0};
            recv(client_unix_sock, buffer, sizeof(buffer) - 1, 0);

            if (strcmp(buffer, ADMIN_PASSWORD) == 0) {
                if (admin_connected == 0) {
                    send(client_unix_sock, "Connected to server", strlen("Connected to server"), 0);
                    admin_connected = 1;
                    if (pthread_create(&thread_id, NULL, admin_handler, (void *)client_info) != 0) {
                        perror("pthread_create failed");
                        close(client_unix_sock);
                    }
                    if (pthread_detach(thread_id) != 0) {
                        perror("pthread_detach failed");
                        close(client_unix_sock);
                    }
                }
                else {
                    send(client_unix_sock, "Admin already connected", strlen("Admin already connected"), 0);
                    close(client_unix_sock);
                }
            } else {
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