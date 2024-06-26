#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <ctype.h>
#include <inttypes.h>
#include "connection.h"
#include "interface_wrapper.h"
#include "image_prc_wrapper.h"
#include "login.h"
#include "fistic_wrapper.h"

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
                    close(client_sock);
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

size_t getImageArraySize(const unsigned char *array) {
    size_t size = 0;
    while (array[size] != '\0') {
        size++;
    }
    return size;
}

void writeImageToFile(const char* filename, unsigned char* imageData, size_t imageSize) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    size_t bytesWritten = fwrite(imageData, sizeof(unsigned char), imageSize, file);
    if (bytesWritten != imageSize) {
        fprintf(stderr, "Error writing to file\n");
        fclose(file);
        return;
    }

    fclose(file);
    printf("Image data written to file successfully.\n");
}

void sendImage(int client_sock, unsigned char* image, size_t imageSize) {
    // Send the image size
    send(client_sock, &imageSize, sizeof(size_t), 0);

    // Send the image data in chunks
    size_t bytesSent = 0;
    while (bytesSent < imageSize) {
        size_t bytesToSend = (CHUNK_SIZE < imageSize - bytesSent) ? CHUNK_SIZE : (imageSize - bytesSent);
        ssize_t sent = send(client_sock, image + bytesSent, bytesToSend, 0);
        if (sent < 0) {
            perror("Failed to send image data");
            close(client_sock);
        }
        bytesSent += sent;
    }
}

unsigned char* receiveImage(int client_sock, size_t imageSize) {
    // Allocate memory for the image
    unsigned char* image = (unsigned char*)malloc(imageSize);
    if (image == NULL) {
        perror("Failed to allocate memory for image");
        close(client_sock);
    }

    // Receive the image data in chunks
    size_t bytesReceived = 0;
    while (bytesReceived < imageSize) {
        size_t bytesToReceive = (CHUNK_SIZE < imageSize - bytesReceived) ? CHUNK_SIZE : (imageSize - bytesReceived);
        ssize_t received = recv(client_sock, image + bytesReceived, bytesToReceive, 0);
        if (received < 0) {
            perror("Failed to receive image data");
            free(image);
            close(client_sock);
        }
        bytesReceived += received;
    }
    
    return image;
}

void *client_handler(void *arg) {
    PGconn *conn = NULL;

    conn = PQconnectdb(DB_CONNECTION);

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
            // Invalid login or banned account, we send a signal to the client to say this
            if (loginResult == 2) {
                send(client_sock, "BAN", 7, 0);
            } else {
                send(client_sock, "FAIL", 7, 0);
            }

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

            uint64_t image_size;
            if (recv(client_sock, &image_size, sizeof(uint64_t), 0) <= 0) {
                perror("Failed to receive image data size from client");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            image_size = be64toh(image_size); // Convert from big-endian to host byte order
            printf("image size: %" PRIu64 "\n", image_size);

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

            if (recv(client_sock, &userId, sizeof(userId), 0) <= 0) {
                perror("Failed to receive user id from client");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            if (recv(client_sock, &postId, sizeof(postId), 0) <= 0) {
                perror("Failed to receive post id from client");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            like_or_remove_like(conn, userId, postId);
        }
        else if (strcmp(buffer, "F") == 0) {
            int filter;
            if (recv(client_sock, &filter, sizeof(filter), 0) <= 0) {
                perror("Failed to receive filter from client");
                printf("Client %d disconnected.\n", id);
                close(client_sock);
                free(client_info);
                pthread_exit(NULL);
            }

            printf("Filter: %d\n", filter);

            size_t receivedImageSize;
            // Receive the image size
            if (recv(client_sock, &receivedImageSize, sizeof(size_t), 0) <= 0) {
                perror("Failed to receive image Size from client");
                close(client_sock);
                pthread_exit(NULL);
            }

            printf("Received image size: %zu\n", receivedImageSize);
            unsigned char* receivedImage = receiveImage(client_sock, receivedImageSize);
            ImageData processedImage;

            // Apply the corresponding filter
            switch (filter) {
                case 0: 
                    if (receivedImage != NULL)
                    {
                        processedImage = applyNegative(receivedImage, receivedImageSize); 
                    }
                    printf("Negative button pressed!\n");
                    break;
                case 1: 
                    if (receivedImage != NULL)
                    {
                        processedImage = applySepia(receivedImage, receivedImageSize); 
                    }
                    printf("Sepia button pressed!\n");
                    break;
                case 2: 
                    if (receivedImage != NULL)
                    {
                        processedImage = applyBlackAndWhite(receivedImage, receivedImageSize);
                    }
                    printf("B&W button pressed!\n"); 
                    break;
                case 3: 
                    if (receivedImage != NULL)
                    { 
                        processedImage = applyBlur(receivedImage, receivedImageSize); 
                    }
                    printf("Blur button pressed!\n");
                    break;
                case 4: 
                    if (receivedImage != NULL)
                    {
                        processedImage = applyCartoonEffect(receivedImage, receivedImageSize); 
                    } 
                    printf("Cartoon button pressed!\n");
                    break;
                case 5: 
                    if (receivedImage != NULL)
                    {
                        processedImage = applyPencilSketch(receivedImage, receivedImageSize); 
                    }
                    printf("Pencil button pressed!\n");
                    break;
                case 6: 
                    if (receivedImage != NULL)
                    {
                        processedImage = applyThermalVision(receivedImage, receivedImageSize); 
                    }
                    printf("Thermal button pressed!\n");
                    break;
                case 7: 
                    if (receivedImage != NULL)
                    {
                        processedImage = applyEdgeDetection(receivedImage, receivedImageSize); 
                    }
                    printf("Edge button pressed!\n");
                    break;
            }

            unsigned char* imageDataPtr = processedImage.dataPtr;
            size_t processedImageSize = processedImage.dataSize;

            writeImageToFile("output.txt", imageDataPtr, processedImageSize);

            printf("image size: %zu\n", processedImageSize);

            sendImage(client_sock, imageDataPtr, processedImageSize);
        }
    }

    // Close socket and free resources
    printf("Client %d disconnected.\n", id);
    free(client_info);
    pthread_exit(NULL);
}

void *admin_handler(void *arg) {
    ClientInfo *client_info = (ClientInfo *)arg;
    struct Post* posts = NULL;
    struct User* users = NULL;
    int client_sock = client_info->sock_fd;
    int id = client_info->id;
    PGconn *conn = NULL;
    printf("Admin client connected.\n");

    int users_count = 0;

    conn = PQconnectdb(DB_CONNECTION);
    users = get_all_users(conn, &users_count);

    char recv_buffer[CHUNK_SIZE];
    char send_buffer[CHUNK_SIZE];
    int ok;
    while (1) {
        memset(recv_buffer, 0, sizeof(recv_buffer));
        memset(send_buffer, 0, sizeof(send_buffer));
        fflush(stdout);

        int received = recv(client_sock, recv_buffer, sizeof(recv_buffer), 0);
        if (received <= 0) {
            printf("Admin client disconnected.\n");
            close(client_sock);
            admin_connected = 0;
            free(client_info);
            pthread_exit(NULL);
        }
        printf("Admin: %s\n", recv_buffer);
        int choice = atoi(recv_buffer);
        switch(choice) {
            case 1:
                memset(recv_buffer, 0, sizeof(recv_buffer));
                memset(send_buffer, 0, sizeof(send_buffer));
                int offset = 0;
                for (int i = 0; i < users_count; i++) {
                    offset += snprintf(send_buffer + offset, sizeof(send_buffer) - offset, "%s %d\n", users[i].name, users[i].id);
                    printf("user name[%d]: %s\n", users[i].id, users[i].name);
                }
                send(client_sock, send_buffer, offset, 0);
                
                break;
            case 2:
                ok = 0;
                memset(recv_buffer, 0, sizeof(recv_buffer));
                memset(send_buffer, 0, sizeof(send_buffer));
                if (recv(client_sock, recv_buffer, sizeof(recv_buffer), 0) <= 0) {
                    perror("Failed to receive user ID from client");
                    break;
                }

                int user_id = atoi(recv_buffer);
                printf("received user_id: %d\n", user_id);
                posts = get_all_user_posts(conn, user_id);
                int posts_count = get_user_posts_counts(conn, user_id);
                printf("posts count: %d\n", posts_count);

                if (posts_count > 0) {
                    snprintf(send_buffer, sizeof(send_buffer), "User id:%d\nUser name:%s\n\n", posts[0].userId, posts[0].userName);
                    for (int j = 0; j < posts_count; j++) {
                        char chunk[CHUNK_SIZE];
                        snprintf(chunk, sizeof(chunk), "Id:%d\nDescription:%s\n", posts[j].id, posts[j].description);
                        strcat(send_buffer, chunk);
                        strcat(send_buffer, "----------\n");
                    }
                    printf ("%s" ,send_buffer);
                    send(client_sock, send_buffer, sizeof(send_buffer), 0);
                } else {
                    char not_found[] = "User has no posts or user does not exist";
                    send(client_sock, not_found, sizeof(not_found), 0);
                }
                break;
            case 3:
                ok = 0;
                memset(recv_buffer, 0, sizeof(recv_buffer));
                memset(send_buffer, 0, sizeof(send_buffer));

                recv(client_sock, recv_buffer, sizeof(recv_buffer), 0);
                posts = get_posts(conn);
                int postsCount = get_posts_counts(conn);
                for (int i = 0; i < postsCount; i++) {
                    if (posts[i].id == atoi(recv_buffer) && ok == 0) {
                        deletePost(conn, posts[i].id);
                        ok = 1;
                        break;
                    }
                }
                if (ok == 1) {
                    char post_deleted[] = "Post has been deleted\n";
                    send(client_sock, post_deleted, sizeof(post_deleted), 0);
                }
                else {
                    char not_found[] = "Post not found\n";
                    send(client_sock, not_found, sizeof(not_found), 0);
                }
                break;
            case 4:
                memset(recv_buffer, 0, sizeof(recv_buffer));
                if (recv(client_sock, recv_buffer, sizeof(recv_buffer), 0) <= 0) {
                    perror("Failed to receive user ID from client");
                    break;
                }
                
                int user_id_block = atoi(recv_buffer);

                bool is_blocked = is_user_blocked(conn, user_id_block);

                char status_message[100];
                snprintf(status_message, sizeof(status_message), "User is currently: %s\nDo you want to change the status (Y/N)?", is_blocked ? "Blocked" : "Unblocked");
                send(client_sock, status_message, sizeof(status_message), 0);

                char admin_choice;
                if (recv(client_sock, &admin_choice, sizeof(admin_choice), 0) <= 0) {
                    perror("Failed to receive admin's choice");
                    break;
                }

                if (admin_choice == 'Y' || admin_choice == 'y') {
                    // Toggle the blocked status of the user
                    bool success = block_user(conn, user_id_block);

                    is_blocked = !is_blocked;

                    if (success) {
                        const char* status = is_blocked ? "Blocked" : "Unblocked";
                        
                        char success_message[100];
                        snprintf(success_message, sizeof(success_message), "User is now: %s", status);
                        send(client_sock, success_message, sizeof(success_message), 0);
                    } else {
                        send(client_sock, "Failed to update user status.", sizeof("Failed to update user status."), 0);
                    }
                }
                else {
                    send(client_sock, "Operation canceled.", sizeof("Operation canceled."), 0);
                }
                break;
            default:
                break;
        }
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

            char buffer[CHUNK_SIZE] = {0};
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

void *la_tzanki(void *arg) {

    functie();
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
    pthread_t unix_thread_id, inet_thread_id, la_tzanki_id;

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

    
    if (pthread_create(&la_tzanki_id, NULL, la_tzanki, NULL) != 0) {
        perror("pthread_create for INET server failed");
        exit(EXIT_FAILURE);
    }

     if (pthread_detach(la_tzanki_id) != 0) {
        perror("pthread_detach for REST server failed");
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