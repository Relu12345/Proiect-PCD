#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include "connection.h"
#include "opencv_wrapper.h"
#include "login.h"

#define MAX_IMAGE_SIZE 100000000

int main() {
    int server_sock;
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);

    if (connect(server_sock, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    setSocket(server_sock);

    createLoginScreen();

    char response[10] = {0};

    recv(server_sock, response, sizeof(char) * 10, 0);
    printf("\n%s\n", response);

    if(strcmp(response, "FAIL") == 0)
    {
        printf("Invalid user or password!\n");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    int id;
    char name[100];

    // Receive user id from server
    recv(server_sock, &id, sizeof(id), 0);

    // Receive user name from server
    recv(server_sock, name, sizeof(name), 0);

    // Receive the number of posts
    int num_received_posts;
    recv(server_sock, &num_received_posts, sizeof(int), 0);

    if (num_received_posts == 0) {
        printf("No posts received\n");
        // Set posts to NULL and imageSizes to NULL
        struct Post* posts = NULL;
        size_t* imageSizes = NULL;
        setUser(id, name);
        setPosts(posts, 0, imageSizes);
    }
    else {
        // Allocate memory to hold the received posts
        struct Post* posts = malloc(sizeof(struct Post) * num_received_posts);
        if (posts == NULL) {
            perror("Memory error");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        size_t *imageSizes = malloc(sizeof(size_t) * num_received_posts);
        recv(server_sock, imageSizes, sizeof(size_t) * num_received_posts, 0);

        // Receive and print each post
        for (int i = 0; i < num_received_posts; i++) {
            recv(server_sock, &(posts[i].id), sizeof(int), 0);
            recv(server_sock, &(posts[i].userId), sizeof(int), 0);
            size_t imageSize;
            recv(server_sock, &imageSize, sizeof(size_t), 0);
            posts[i].image = malloc(imageSize);
            if (posts[i].image == NULL) {
                perror("Memory allocation failed for image data");
                exit(EXIT_FAILURE);
            }
            size_t bytesReceived = 0;
            while (bytesReceived < imageSize) {
                size_t remainingBytes = imageSize - bytesReceived;
                size_t bytesToReceive = remainingBytes > CHUNK_SIZE ? CHUNK_SIZE : remainingBytes;
                int received = recv(server_sock, posts[i].image + bytesReceived, bytesToReceive, 0);
                if (received <= 0) {
                    if (received < 0) {
                        perror("Failed to receive image data");
                    } else {
                        printf("Connection closed by client\n");
                    }
                    close(server_sock);
                    return 1;
                }
                bytesReceived += received;
            }
            //recv(server_sock, posts[i].image, imageSize, 0);
            int description_length;
            recv(server_sock, &description_length, sizeof(int), 0);
            posts[i].description = malloc(description_length);
            recv(server_sock, posts[i].description, description_length, 0);
            int username_length;
            recv(server_sock, &username_length, sizeof(int), 0);
            posts[i].userName = malloc(username_length);
            recv(server_sock, posts[i].userName, username_length, 0);
            recv(server_sock, &(posts[i].likeCount), sizeof(int), 0);
            recv(server_sock, &(posts[i].liked), sizeof(int), 0);
        }

        setUser(id, name);
        setPosts(posts, num_received_posts, imageSizes);
    }

    mainScreen();

    close(server_sock);
    return 0;
}
