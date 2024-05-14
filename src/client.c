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

    char send_message[MESSAGE_SIZE];
    size_t len;

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
            recv(server_sock, posts[i].image, imageSize, 0);
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

    unsigned char* image = (unsigned char*)malloc(sizeof(unsigned char) * MAX_IMAGE_SIZE);
    int image_size;
    image = getImage();
    image_size = getImageSize();

    // Send image data to the server
    send(server_sock, &image_size, sizeof(int), 0);
    send(server_sock, image, image_size, 0);

    // // Prompt user to enter image path
    // printf("Enter image path: ");
    // if (fgets(send_message, sizeof(send_message), stdin) == NULL) {
    //     perror("Failed to read image path from stdin");
    //     exit(EXIT_FAILURE);
    // }

    // // Remove newline character if present
    // len = strlen(send_message);
    // if (len > 0 && send_message[len - 1] == '\n') {
    //     send_message[len - 1] = '\0';
    // }

    // // Open the image file
    // FILE *imageFile = fopen(send_message, "rb");
    // if (imageFile == NULL) {
    //     perror("Failed to open image file");
    //     exit(EXIT_FAILURE);
    // }

    // // Read image data from file
    // fseek(imageFile, 0, SEEK_END);
    // long fileSize = ftell(imageFile);
    // if (fileSize <= 0 || fileSize > MAX_IMAGE_SIZE) {
    //     fclose(imageFile);
    //     perror("Invalid image file size");
    //     exit(EXIT_FAILURE);
    // }
    // rewind(imageFile);

    // unsigned char *imageData = (unsigned char*)malloc(fileSize);
    // if (imageData == NULL) {
    //     fclose(imageFile);
    //     perror("Failed to allocate memory for image data");
    //     exit(EXIT_FAILURE);
    // }

    // size_t bytesRead = fread(imageData, 1, fileSize, imageFile);
    // fclose(imageFile);
    // if (bytesRead != fileSize) {
    //     free(imageData);
    //     perror("Failed to read image data from file");
    //     exit(EXIT_FAILURE);
    // }

    // // Send image data to the server
    // send(server_sock, &fileSize, sizeof(long), 0);
    // send(server_sock, imageData, fileSize, 0);

    // // Receive image data (grayscale byte array) from the server
    // int width, height;
    // recv(server_sock, &width, sizeof(int), 0);
    // recv(server_sock, &height, sizeof(int), 0);

    // // Print received width and height
    // printf("Received width: %d\n", width);
    // printf("Received height: %d\n", height);

    // unsigned char* grayscaleData = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    // if (grayscaleData == NULL) {
    //     perror("Failed to allocate memory for grayscale data");
    //     exit(EXIT_FAILURE);
    // }
    // recv(server_sock, grayscaleData, width * height * sizeof(unsigned char), 0);

    // // Display the received image on a window
    // printf("Displaying image with width: %d, height: %d\n", width, height);
    // showImageFromBytes(grayscaleData, width, height);

    // Clean up
    // free(imageData);
    // free(grayscaleData);
    close(server_sock);
    return 0;
}
