#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include "connection.h"
#include "opencv_wrapper.h"

#define MAX_IMAGE_SIZE 100000000

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
    size_t len;

    // Prompt user to enter image path
    printf("Enter image path: ");
    if (fgets(send_message, sizeof(send_message), stdin) == NULL) {
        perror("Failed to read image path from stdin");
        exit(EXIT_FAILURE);
    }

    // Remove newline character if present
    len = strlen(send_message);
    if (len > 0 && send_message[len - 1] == '\n') {
        send_message[len - 1] = '\0';
    }

    // Open the image file
    FILE *imageFile = fopen(send_message, "rb");
    if (imageFile == NULL) {
        perror("Failed to open image file");
        exit(EXIT_FAILURE);
    }

    // Read image data from file
    fseek(imageFile, 0, SEEK_END);
    long fileSize = ftell(imageFile);
    if (fileSize <= 0 || fileSize > MAX_IMAGE_SIZE) {
        fclose(imageFile);
        perror("Invalid image file size");
        exit(EXIT_FAILURE);
    }
    rewind(imageFile);

    unsigned char *imageData = (unsigned char*)malloc(fileSize);
    if (imageData == NULL) {
        fclose(imageFile);
        perror("Failed to allocate memory for image data");
        exit(EXIT_FAILURE);
    }

    size_t bytesRead = fread(imageData, 1, fileSize, imageFile);
    fclose(imageFile);
    if (bytesRead != fileSize) {
        free(imageData);
        perror("Failed to read image data from file");
        exit(EXIT_FAILURE);
    }

    // Send image data to the server
    send(client_sock, &fileSize, sizeof(long), 0);
    send(client_sock, imageData, fileSize, 0);

    // Receive image data (grayscale byte array) from the server
    int width, height;
    recv(client_sock, &width, sizeof(int), 0);
    recv(client_sock, &height, sizeof(int), 0);
    unsigned char* grayscaleData = (unsigned char*)malloc(width * height * sizeof(unsigned char));
    if (grayscaleData == NULL) {
        perror("Failed to allocate memory for grayscale data");
        exit(EXIT_FAILURE);
    }
    recv(client_sock, grayscaleData, width * height * sizeof(unsigned char), 0);

    // Display the received image on a window
    showImageFromBytes(grayscaleData, width, height);

    // Clean up
    free(imageData);
    free(grayscaleData);
    close(client_sock);
    return 0;
}
