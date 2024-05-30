#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include "interface_wrapper.h"
#include "../../../../../../usr/include/x86_64-linux-gnu/sys/socket.h"
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/un.h>
#include <termios.h>
#include <fcntl.h>
#include <cstdio>
#include <atomic>
#include <vector>
#include <iomanip>
#include <thread>
#include <chrono>
#include "database.h"
#include "connection.h"
#include <cstddef>

struct PostData {
    std::string imagePath;
    std::string imageType;
    std::string description;
    bool isTypingDescription;
    PostData() : isTypingDescription(false) {}
} postData;

struct LoginData {
    std::string username;
    std::string password;
    bool isTypingUsername;
    bool isTypingPassword;

    LoginData() : isTypingUsername(false), isTypingPassword(false) {}
};

void refreshPosts();
void sendLike(int postId, int userId);

LoginData loginData;

std::atomic<bool> loginWindowVisible(true);
std::atomic<bool> mainWindowVisible(true);
std::atomic<bool> postWindowVisible(true);
std::atomic<bool> image_uploaded(false);
std::atomic<bool> filterPressed(false);
std::atomic<bool> failedFilter(false);
std::atomic<bool> resetRequested(false);
std::atomic<bool> sendRequested(false);
std::atomic<bool> postSuccessMessageVisible(false);
std::atomic<bool> firstTime(true);
std::chrono::steady_clock::time_point postSuccessMessageTime;


// Global variable to hold the client socket
int currentPostIndex = 0;
int serverSock;
int postCount;
Post* posts = nullptr;
User user;
size_t *sizes = nullptr;

const char SIGNAL_POST = 'P';
const char SIGNAL_GET_POST = 'G';
const char SIGNAL_LIKE = 'L';
const char SIGNAL_FILTER = 'F';

typedef unsigned char uchar;
std::vector<uchar> buffer;
size_t image_size;

std::vector<uchar> convertToVector(const uchar* data, size_t size) {
    // Initialize vector with data from the array
    return std::vector<uchar>(data, data + size);
}

uchar* convertToPointer(std::vector<uchar>& vec) {
    // Return a pointer to the beginning of the vector's data
    return vec.data();
}

unsigned char hexCharToByte(char hex) {
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    } else if (hex >= 'a' && hex <= 'f') {
        return hex - 'a' + 10;
    } else if (hex >= 'A' && hex <= 'F') {
        return hex - 'A' + 10;
    }
    return 0; // Error
}

std::vector<unsigned char> transformData(const std::vector<unsigned char>& inputData) {
    std::vector<unsigned char> outputData;
    for (size_t i = 2; i < inputData.size(); i += 2) {
        unsigned char value = (hexCharToByte(inputData[i]) << 4) | hexCharToByte(inputData[i + 1]);
        outputData.push_back(value);
    }
    return outputData;
}

void sendImage(int client_sock, const std::vector<uchar>& image, size_t imageSize) {
    std::cout << "in send image" << std::endl;
    
    // Send the image size
    send(client_sock, &imageSize, sizeof(size_t), 0);

    std::cout << "sent image size: " << imageSize << std::endl;

    // Convert vector to pointer
    uchar* imageDataPointer = convertToPointer(const_cast<std::vector<uchar>&>(image));

    // Send the image data in chunks
    size_t bytesSent = 0;
    while (bytesSent < imageSize) {
        size_t bytesToSend = std::min((size_t) CHUNK_SIZE, imageSize - bytesSent);
        ssize_t sent = send(client_sock, imageDataPointer + bytesSent, bytesToSend, 0);
        if (sent < 0) {
            perror("Failed to send image data");
            exit(EXIT_FAILURE);
        }
        bytesSent += sent;
    }

    std::cout << "image sent succesfully!" << std::endl;
}

std::vector<uchar> receiveImage(int client_sock) {
    size_t imageSize;

    // Receive the image size
    recv(client_sock, &imageSize, sizeof(size_t), 0);

    std::cout << "received image size: " << imageSize << std::endl;

    // Allocate memory for the image
    std::vector<uchar> image(imageSize);

    // Receive the image data in chunks
    size_t bytesReceived = 0;
    while (bytesReceived < imageSize) {
        size_t bytesToReceive = std::min((size_t) CHUNK_SIZE, imageSize - bytesReceived);
        ssize_t received = recv(client_sock, image.data() + bytesReceived, bytesToReceive, 0);
        if (received < 0) {
            perror("Failed to receive image data");
            exit(EXIT_FAILURE);
        }
        bytesReceived += received;
    }

    std::cout << "image received succesfully!" << std::endl;
    
    return image;
}

// Function to send message to client
void sendLoginInfoToServer(int choice, const char* user, const char* pass) {
    // Create a buffer to hold the combined message
    char combinedMessage[205];
    char char_choice[2];

    memset(combinedMessage, 0, sizeof(combinedMessage));
    memset(char_choice, 0, sizeof(char_choice));

    sprintf(char_choice, "%d", choice);

    // Send choice to server
    if (send(serverSock, char_choice, strlen(char_choice), 0) == -1) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    // Concatenate the username and password with a delimiter
    snprintf(combinedMessage, sizeof(combinedMessage), "%s,%s", user, pass);

    // Send the combined message to the server
    if (send(serverSock, combinedMessage, strlen(combinedMessage), 0) == -1) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }
}

void drawTextField(cv::Mat& img, const std::string& text, const cv::Point& topLeft, int width, int height) {
    cv::rectangle(img, topLeft, cv::Point(topLeft.x + width, topLeft.y + height), cv::Scalar(0, 0, 0), 2);
    cv::putText(img, text, cv::Point(topLeft.x + 5, topLeft.y + height - 5), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
}

// Function to enable non-blocking keyboard input
void enableNonBlockingInput() {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

// Function to disable non-blocking keyboard input
void disableNonBlockingInput() {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag |= ICANON | ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK);
}

void GoToMainScreenFromPost() {
    postWindowVisible = false; // Close the post screen
    mainWindowVisible = true; // Open the main screen
    cv::destroyWindow("Post Screen");
    mainScreen(); // Call mainScreen function
    return;
}

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        LoginData* data = (LoginData*)userdata;
        
        // Check if the mouse click is within the bounds of the username input field
        if (x >= 200 && x <= 350 && y >= 100 && y <= 130) {
            data->isTypingUsername = true;
            data->isTypingPassword = false;
            std::cout << "Enter username: " << std::flush;
        }
        // Check if the mouse click is within the bounds of the password input field
        else if (x >= 200 && x <= 350 && y >= 160 && y <= 190) {
            data->isTypingPassword = true;
            data->isTypingUsername = false;
            std::cout << "Enter password: " << std::flush;
        }
        // Check if the mouse click is within the bounds of the login button
        else if (x >= 240 && x <= 340 && y >= 240 && y <= 280) {
            std::cout << "Login button pressed." << std::endl;
            std::cout << "Username: " << data->username << std::endl;
            std::cout << "Password: " << data->password << std::endl;
            // Send username and password to client
            sendLoginInfoToServer(1, data->username.c_str(), data->password.c_str());
            loginWindowVisible = false;
            cv::destroyWindow("Login Screen");
        }
        else if (x >= 50 && x <= 210 && y >= 240 && y <= 280) {
            std::cout << "Register button pressed." << std::endl;
            std::cout << "Username: " << data->username << std::endl;
            std::cout << "Password: " << data->password << std::endl;
            // Send username and password to client
            sendLoginInfoToServer(0, data->username.c_str(), data->password.c_str());
            loginWindowVisible = false;
            cv::destroyWindow("Login Screen");
        }
    }
}

void mainOnMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        // Check if the click is within the "New Post" button
        if (x >= 500 && x <= 700 && y >= 750 && y <= 800) {
            mainWindowVisible = false;
            postWindowVisible = true;
            cv::destroyWindow("Main Screen");
            createPostScreen();
        }
        else if (x >= 10 && x <= 160 && y >= 10 && y <= 60) {
            refreshPosts();
        }
        // Check if the click is within the left button
        else if (x >= 100 && x <= 200 && y >= 400 && y <= 450) {
            currentPostIndex = (currentPostIndex - 1 + postCount) % postCount;
        }
        // Check if the click is within the right button
        else if (x >= 1000 && x <= 1100 && y >= 400 && y <= 450) {
            currentPostIndex = (currentPostIndex + 1) % postCount;
        }
        // Check if the click is within the upper left button
        else if (x >= 50 && x <= 100 && y >= 200 && y <= 250) {
            currentPostIndex = 0;
        }
        // Check if the click is within the upper right button
        else if (x >= 1100 && x <= 1150 && y >= 200 && y <= 250) {
            currentPostIndex = postCount - 1;
        }
        // Check if the click is within the like button
        else if (x >= 515 && x <= 579 && y >= 635 && y <= 699) {
            // Toggle the liked state of the current post
            posts[currentPostIndex].liked = !posts[currentPostIndex].liked;
            // Update the like count accordingly
            if (posts[currentPostIndex].liked) {
                posts[currentPostIndex].likeCount++;
            } else {
                posts[currentPostIndex].likeCount--;
            }
            sendLike(posts[currentPostIndex].id, user.id);
        }
    }
}

cv::Mat originalImage;
cv::Mat currentImage;
bool autoFilter = false;

void processFilter(int i) {
    int attemptCount = 0;
    const int maxAttempts = 10;
    failedFilter = false;

    while (attemptCount < maxAttempts) {
        if (!originalImage.empty()) {
            if (send(serverSock, &SIGNAL_FILTER, sizeof(SIGNAL_FILTER), 0) == -1) {
                perror("send failed");
                exit(EXIT_FAILURE);
            }

            std::cout << "filter signal sent successfully!" << std::endl;

            if (send(serverSock, &i, sizeof(i), 0) == -1) {
                perror("send failed");
                exit(EXIT_FAILURE);
            }

            std::cout << "filter sent successfully!" << std::endl;

            if (!originalImage.empty()) {
                cv::imencode(postData.imageType, originalImage, buffer);
                size_t imageSizeSend = buffer.size();
                sendImage(serverSock, buffer, imageSizeSend);
            } else {
                std::cout << "EMPTY IMAGE!!!!" << std::endl;
                continue;
            }

            std::cout << "after send in client" << std::endl;

            std::vector<uchar> newImageData = receiveImage(serverSock);

            std::cout << "before decode in client" << std::endl;
            try {
                currentImage = cv::imdecode(newImageData, cv::IMREAD_COLOR);
                if (currentImage.empty()) {
                    throw cv::Exception(cv::Error::StsError, "Decoded image is empty", __FUNCTION__, __FILE__, __LINE__);
                }

                if (currentImage.channels() != 3) {
                    cv::cvtColor(currentImage, currentImage, cv::COLOR_GRAY2BGR);
                }
                std::cout << "after decode in client" << std::endl;
                filterPressed = true;
                break;
            } catch (const cv::Exception& e) {
                std::cerr << "cv::cvtColor failed: " << e.what() << std::endl;
                // Add a 50 ms delay before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } catch (const std::exception& e) {
                std::cerr << "Standard exception: " << e.what() << std::endl;
                // Add a 50 ms delay before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } catch (...) {
                std::cerr << "Unknown exception occurred" << std::endl;
                // Add a 50 ms delay before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            std::cout << "after the try-catch" << std::endl;
        } else {
            std::cerr << "Original image is empty, exiting loop." << std::endl;
            break;
        }
        attemptCount++;
    }

    if (attemptCount >= maxAttempts) {
        std::cerr << "Maximum attempts reached. Filter failed." << std::endl;
        failedFilter = true;
        filterPressed = true;
    }
}

void postOnMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        PostData* data = (PostData*)userdata;
        if (x >= 850 && x <= 960 && y >= 700 && y <= 750) {
            if (!currentImage.empty())
                filterPressed = true;
                resetRequested = true;
        }
        // Send button
        else if (x >= 545 && x <= 655 && y >= 720 && y <= 770) {
            std::cout << "Send button pressed!" << std::endl;
            if (!currentImage.empty())
                sendRequested = true;
        }
        // Filter buttons
        else {
            int buttonHeight = 50;
            int buttonSpacing = 20;
            int buttonX = 20;
            int ogButtonX = buttonX;
            for (int i = 0; i < 8; ++i) {
                int buttonY = (i < 4) ? 500 : 600;
                if (i == 4) buttonX = ogButtonX;
                if (x >= buttonX && x <= buttonX + 100 && y >= buttonY && y <= buttonY + buttonHeight) {
                    autoFilter = true;
                    while (autoFilter) {
                        processFilter(i);
                        autoFilter = !filterPressed;
                    }
                    if (failedFilter) {
                        filterPressed = false;
                        failedFilter = false;
                        currentImage = originalImage.clone();
                        resetRequested = true;
                    }
                    break;
                }
                buttonX += 100 + buttonSpacing;
            }
        }
        if (x >= 250 && x <= 350 && y >= 300 && y <= 340) {
            FILE *in;
            if (!(in = popen("zenity  --title=\"Select an image\" --file-selection","r"))) {
                perror("popen error");
            }

            char buff[512];
            std::string selectFile = "";
            while (fgets(buff, sizeof(buff), in) != NULL) {
                selectFile += buff;
            }
            pclose(in);

            selectFile.erase(std::remove(selectFile.begin(), selectFile.end(), '\n'), selectFile.end());

            std::string fileExtension = "";
            size_t dotPosition = selectFile.find_last_of(".");
            if (dotPosition != std::string::npos) {
                fileExtension = selectFile.substr(dotPosition);
            }

            data->imagePath = selectFile;
            data->imageType = fileExtension;
            image_uploaded = true;
        }
        
        // Check if the click is within the "Cancel" button
        if (x >= 10 && x <= 130 && y >= 10 && y <= 60) {
            GoToMainScreenFromPost();
            std::cout << "Cancel button pressed!" << std::endl;
        }
        
        // Check if the click is within the description input field
        if (x >= 20 && x <= 570 && y >= 400 && y <= 450) {
            data->isTypingDescription = true;
        } else {
            data->isTypingDescription = false;
        }
    }
}

// Keyboard callback function for handling keyboard input
void onKeyboard(char key) {
    static bool isTypingUsername = false;
    static bool isTypingPassword = false;

    if (key == 'u') {
        isTypingUsername = true;
        isTypingPassword = false;
        std::cout << "Enter username: ";
    } else if (key == 'p') {
        isTypingPassword = true;
        isTypingUsername = false;
        std::cout << "Enter password: ";
    } else if (key == '\n') {
        isTypingUsername = false;
        isTypingPassword = false;
        std::cout << std::endl;
    } else if (isTypingUsername) {
        loginData.username += key;
        std::cout << key;
    } else if (isTypingPassword) {
        loginData.password += key;
        std::cout << '*';
    }
}

void sendLike(int postId, int userId) {
    if (send(serverSock, &SIGNAL_LIKE, sizeof(SIGNAL_LIKE), 0) == -1) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    if (send(serverSock, &userId, sizeof(userId), 0) == -1) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    if (send(serverSock, &postId, sizeof(postId), 0) == -1) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }
}

void refreshPosts() {
    if (send(serverSock, &SIGNAL_GET_POST, sizeof(SIGNAL_GET_POST), 0) == -1) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }
    // Receive the number of posts
    int num_received_posts;
    recv(serverSock, &num_received_posts, sizeof(int), 0);

    if (num_received_posts == 0) {
        printf("No posts received\n");
        // Set posts to NULL and imageSizes to NULL
        struct Post* posts = NULL;
        size_t* imageSizes = NULL;
        setPosts(posts, 0, imageSizes);
    }
    else {
        // Allocate memory to hold the received posts
        struct Post* posts = new Post[num_received_posts];
        if (posts == NULL) {
            perror("Memory error");
            close(serverSock);
            exit(EXIT_FAILURE);
        }

        size_t* imageSizes = new size_t[num_received_posts];
        recv(serverSock, imageSizes, sizeof(size_t) * num_received_posts, 0);

        // Receive and print each post
        for (int i = 0; i < num_received_posts; i++) {
            recv(serverSock, &(posts[i].id), sizeof(int), 0);
            recv(serverSock, &(posts[i].userId), sizeof(int), 0);
            size_t imageSize;
            recv(serverSock, &imageSize, sizeof(size_t), 0);
            posts[i].image = new unsigned char[imageSize];
            if (posts[i].image == NULL) {
                perror("Memory allocation failed for image data");
                exit(EXIT_FAILURE);
            }
            size_t bytesReceived = 0;
            while (bytesReceived < imageSize) {
                size_t remainingBytes = imageSize - bytesReceived;
                size_t bytesToReceive = remainingBytes > CHUNK_SIZE ? CHUNK_SIZE : remainingBytes;
                int received = recv(serverSock, posts[i].image + bytesReceived, bytesToReceive, 0);
                if (received <= 0) {
                    if (received < 0) {
                        perror("Failed to receive image data");
                    } else {
                        printf("Connection closed by client\n");
                    }
                    close(serverSock);
                    exit(EXIT_FAILURE);
                }
                bytesReceived += received;
            }
            //recv(server_sock, posts[i].image, imageSize, 0);
            int description_length;
            recv(serverSock, &description_length, sizeof(int), 0);
            posts[i].description = new char[description_length];
            recv(serverSock, posts[i].description, description_length, 0);
            int username_length;
            recv(serverSock, &username_length, sizeof(int), 0);
            posts[i].userName = new char[username_length];
            recv(serverSock, posts[i].userName, username_length, 0);
            recv(serverSock, &(posts[i].likeCount), sizeof(int), 0);
            recv(serverSock, &(posts[i].liked), sizeof(int), 0);
        }

        setPosts(posts, num_received_posts, imageSizes);
    }
}

extern "C" void createLoginScreen() {
    // Create a window
    cv::Mat loginScreen(300, 400, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::namedWindow("Login Screen");

    // Define labels
    std::string usernameLabel = "Username:";
    std::string passwordLabel = "Password:";
    std::string loginLabel = "Login";
    std::string registerLabel = "Register";

    // Enable non-blocking keyboard input
    enableNonBlockingInput();

    // Set mouse callback for login button
    cv::setMouseCallback("Login Screen", onMouse, &loginData);

    // Show the login screen
    cv::imshow("Login Screen", loginScreen);

    while (loginWindowVisible) {
        // Clear the screen
        loginScreen = cv::Scalar(255, 255, 255);

        // Draw labels
        cv::putText(loginScreen, "Login", cv::Point(150, 50), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 0), 2);
        cv::putText(loginScreen, usernameLabel, cv::Point(50, 120), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
        cv::putText(loginScreen, passwordLabel, cv::Point(50, 180), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);

        // Draw input fields with cursor and text
        cv::rectangle(loginScreen, cv::Rect(200, 100, 150, 30), cv::Scalar(0, 0, 0), 2);
        if (loginData.isTypingUsername) {
            int textWidth = cv::getTextSize(loginData.username, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, nullptr).width;
            cv::line(loginScreen, cv::Point(200 + textWidth + 5, 103), cv::Point(200 + textWidth + 5, 127), cv::Scalar(0, 0, 0), 2);
        }
        cv::putText(loginScreen, loginData.username, cv::Point(205, 125), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 2);

        cv::rectangle(loginScreen, cv::Rect(200, 160, 150, 30), cv::Scalar(0, 0, 0), 2);
        if (loginData.isTypingPassword) {
            int textWidth = cv::getTextSize(loginData.password, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, nullptr).width;
            cv::line(loginScreen, cv::Point(200 + textWidth + 5, 163), cv::Point(200 + textWidth + 5, 187), cv::Scalar(0, 0, 0), 2);
        }
        cv::putText(loginScreen, std::string(loginData.password.size(), '*'), cv::Point(205, 185), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 2);

        // Draw login button
        cv::rectangle(loginScreen, cv::Rect(240, 240, 100, 40), cv::Scalar(0, 0, 255), -1);
        cv::putText(loginScreen, loginLabel, cv::Point(255, 270), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

        // Draw register button
        cv::rectangle(loginScreen, cv::Rect(40, 240, 160, 40), cv::Scalar(0, 0, 255), -1);
        cv::putText(loginScreen, registerLabel, cv::Point(70, 270), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

        // Show the updated login screen
        cv::imshow("Login Screen", loginScreen);

        // Process keyboard input
        char key = cv::waitKey(10);
        if (key != -1) {
            if (loginData.isTypingUsername && key != '\n') {
                if (key == '\b' || key == 127) {  // Backspace key or Delete key
                    if (!loginData.username.empty()) {
                        loginData.username.pop_back();
                    }
                } else {
                    loginData.username += key;
                }
            } else if (loginData.isTypingPassword && key != '\n') {
                if (key == '\b' || key == 127) {  // Backspace key or Delete key
                    if (!loginData.password.empty()) {
                        loginData.password.pop_back();
                    }
                } else {
                    loginData.password += key;
                }
            }
        }
    }

    // Disable non-blocking keyboard input
    disableNonBlockingInput();
}

void printVectorToFile(const std::vector<uchar>& data, const std::string& filename, int index) {
    std::ofstream outputFile(filename + std::to_string(index), std::ios::binary);
    if (!outputFile) {
        std::cerr << "Error opening file for writing: " << filename << std::endl;
        return;
    }

    // Write vector data to the file
    outputFile.write(reinterpret_cast<const char*>(data.data()), data.size());

    // Close the file
    outputFile.close();

    std::cout << "Vector data has been written to " << filename << std::endl;
}

extern "C" void mainScreen() {
    if (firstTime) {
        firstTime = false;
    }
    else {
        refreshPosts();
    }
    cv::Mat mainScreen(800, 1200, CV_8UC3, cv::Scalar(255,255,255));
    cv::namedWindow("Main Screen");
    enableNonBlockingInput();
    cv::setMouseCallback("Main Screen", mainOnMouse);
    cv::imshow("Main Screen", mainScreen);

    while (mainWindowVisible) {
        mainScreen = cv::Scalar(255, 255, 255);

        cv::Mat img(800, 800, CV_8UC3, cv::Scalar(255, 255, 255));

        if (posts != nullptr && currentPostIndex >= 0 && currentPostIndex < postCount) {
            std::vector<uchar> imageVector = convertToVector(posts[currentPostIndex].image, sizes[currentPostIndex]);
            std::vector<uchar> binaryData = transformData(imageVector);

            cv::Mat new_img = cv::imdecode(binaryData, cv::IMREAD_COLOR);

            // Check if the image is decoded successfully
            if (new_img.empty()) {
                std::cout << "Failed to decode image." << std::endl;
                return;
            }

            img = new_img.clone();

            // Print image dimensions
            // std::cout << "Image dimensions: " << img.cols << "x" << img.rows << std::endl;

            double aspect_ratio = (double)img.cols / (double)img.rows;

            int max_width = 640;
            int max_height = 480;

            int new_width = max_width;
            int new_height = static_cast<int>(max_width / aspect_ratio);
            if (new_height > max_height) {
                new_height = max_height;
                new_width = static_cast<int>(max_height * aspect_ratio);
            }

            cv::Mat resizedImg;
            cv::resize(img, resizedImg, cv::Size(new_width, new_height));

            int x = (mainScreen.cols - resizedImg.cols) / 2;
            int y = (mainScreen.rows - resizedImg.rows) / 2 - 75;

            cv::Rect imageRect(x, y, resizedImg.cols, resizedImg.rows);

            if (imageRect.x < 0 || imageRect.y < 0 || imageRect.x + imageRect.width > mainScreen.cols || imageRect.y + imageRect.height > mainScreen.rows) {
                std::cout << "Invalid region for placing image." << std::endl;
                return;
            }

            resizedImg.copyTo(mainScreen(imageRect));

            // Display the username
            std::string username = posts[currentPostIndex].userName;
            cv::putText(mainScreen, username, cv::Point(mainScreen.cols / 2 - 20, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 0), 2, cv::LINE_AA, false);

            // Display the description
            std::string description = posts[currentPostIndex].description;
            cv::putText(mainScreen, description, cv::Point(mainScreen.cols / 2 - 320, 615), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 0), 2, cv::LINE_AA, false);

            // Display the like button
            cv::Mat likeButton(64, 64, CV_8UC3, cv::Scalar(0, 0, 0));
            cv::Rect likeButtonRect(515, 635, likeButton.cols, likeButton.rows);
            likeButton.copyTo(mainScreen(likeButtonRect));

            // Display the like count
            int likeCount = posts[currentPostIndex].likeCount;
            std::string likeCountText = std::to_string(likeCount);
            cv::putText(mainScreen, likeCountText, cv::Point(605, 675), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 0), 2, cv::LINE_AA, false);
        }

        // Buttons
        cv::Rect refreshButtonRect(10, 10, 150, 50);
        cv::rectangle(mainScreen, refreshButtonRect, cv::Scalar(203, 192, 255), -1);
        cv::putText(mainScreen, "Refresh", cv::Point(25, 45), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

        cv::Rect leftButtonRect(100, 400, 100, 50);
        cv::rectangle(mainScreen, leftButtonRect, cv::Scalar(0, 0, 255), -1);
        cv::putText(mainScreen, "<", cv::Point(135, 435), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

        cv::Rect rightButtonRect(1000, 400, 100, 50);
        cv::rectangle(mainScreen, rightButtonRect, cv::Scalar(255, 0, 0), -1);
        cv::putText(mainScreen, ">", cv::Point(1035, 435), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

        cv::Rect upperLeftArrowRect(50, 200, 50, 50);
        cv::rectangle(mainScreen, upperLeftArrowRect, cv::Scalar(0, 0, 0), -1);
        cv::putText(mainScreen, "<<", cv::Point(50, 235), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

        cv::Rect upperRightArrowRect(1100, 200, 50, 50);
        cv::rectangle(mainScreen, upperRightArrowRect, cv::Scalar(0, 0, 0), -1);
        cv::putText(mainScreen, ">>", cv::Point(1100, 235), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

        cv::Rect makePostButtonRect(500, 750, 200, 50);
        cv::rectangle(mainScreen, makePostButtonRect, cv::Scalar(0, 255, 0), -1);
        cv::putText(mainScreen, "New Post", cv::Point(525, 785), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 0), 2);

        // Show the window
        cv::imshow("Main Screen", mainScreen);

        // Wait for a key press
        cv::waitKey(1);
    }
    disableNonBlockingInput();
    cv::destroyWindow("Main Screen");
}

extern "C" void createPostScreen () {
    originalImage = cv::Mat();
    currentImage = cv::Mat();
    image_uploaded = false;
    filterPressed = false;
    resetRequested = false;
    sendRequested = false;

    cv::Mat postScreen(800, 1200, CV_8UC3, cv::Scalar(255,255,255));
    cv::namedWindow("Post Screen");
    enableNonBlockingInput();
    cv::imshow("Post Screen", postScreen);
    cv::setMouseCallback("Post Screen", postOnMouse, &postData);

    // Draw the "Cancel" button
    int cancelButtonX = 10;
    int cancelButtonY = 10;
    int cancelButtonWidth = 120;
    int cancelButtonHeight = 50;
    cv::rectangle(postScreen, cv::Rect(cancelButtonX, cancelButtonY, cancelButtonWidth, cancelButtonHeight), cv::Scalar(0, 0, 255), -1);
    cv::putText(postScreen, "Cancel", cv::Point(cancelButtonX + 10, cancelButtonY + 35), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

    // Draw the "Reset" button
    int resetButtonX = 850;
    int resetButtonY = 700;
    int resetButtonWidth = 110;
    int resetButtonHeight = 50;
    cv::rectangle(postScreen, cv::Rect(resetButtonX, resetButtonY, resetButtonWidth, resetButtonHeight), cv::Scalar(0, 255, 0), -1);
    cv::putText(postScreen, "Reset", cv::Point(resetButtonX + 10, resetButtonY + 35), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

    // Draw the "Send" button
    int sendButtonX = 545;
    int sendButtonY = 720;
    int sendButtonWidth = 110;
    int sendButtonHeight = 50;
    cv::rectangle(postScreen, cv::Rect(sendButtonX, sendButtonY, sendButtonWidth, sendButtonHeight), cv::Scalar(0, 255, 255), -1);
    cv::putText(postScreen, "Send", cv::Point(sendButtonX + 15, sendButtonY + 35), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

    // Draw the filter buttons
    int buttonHeight = 50;
    int buttonSpacing = 20;
    int buttonX = 20;
    int ogButtonX = buttonX;

    for (int i = 0; i < 8; ++i) {
        int buttonY = (i < 4) ? 500 : 600;
        if (i == 4) buttonX = ogButtonX;
        cv::rectangle(postScreen, cv::Rect(buttonX, buttonY, 100, buttonHeight), cv::Scalar(255, 0, 0), -1);
        cv::putText(postScreen, "Filter " + std::to_string(i + 1), cv::Point(buttonX + 10, buttonY + 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
        buttonX += 100 + buttonSpacing;
    }

    cv::putText(postScreen, "Post Description:", cv::Point(20, 375), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 0), 2);
    cv::rectangle(postScreen, cv::Rect(20, 400, 550, 50), cv::Scalar(0, 0, 0), 2);

    int leftSectionWidth = 600;
    cv::Rect leftSectionRect(0, 0, leftSectionWidth, postScreen.rows);

    int rightSectionWidth = postScreen.cols - leftSectionWidth;
    cv::Rect rightSectionRect(leftSectionWidth, 0, rightSectionWidth, postScreen.rows);

    if (!image_uploaded) {
        std::string uploadText = "Upload an image:";
        cv::Size textSize = cv::getTextSize(uploadText, cv::FONT_HERSHEY_SIMPLEX, 1.5, 2, nullptr);
        cv::putText(postScreen, uploadText, cv::Point((leftSectionRect.width - textSize.width) / 2, 200), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 0), 2);

        // Draw the button
        int buttonWidth = 100;
        int buttonHeight = 40;
        int buttonX = (leftSectionRect.width - buttonWidth) / 2;
        int buttonY = 300;
        cv::rectangle(postScreen(leftSectionRect), cv::Rect(buttonX, buttonY, buttonWidth, buttonHeight), cv::Scalar(255, 0, 0), -1);
        cv::putText(postScreen, "Upload", cv::Point(buttonX + 10, buttonY + 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
    }

    while(postWindowVisible) {
        cv::Mat reconstructed_image;

        cv::putText(postScreen, "Post Description:", cv::Point(20, 375), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 0), 2);
        cv::rectangle(postScreen, cv::Rect(20, 400, 550, 50), cv::Scalar(255, 255, 255), -1);

        if (postData.isTypingDescription) {
            int textWidth = cv::getTextSize(postData.description, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, nullptr).width;
            cv::line(postScreen, cv::Point(25 + textWidth + 5, 410), cv::Point(25 + textWidth + 5, 440), cv::Scalar(0, 0, 0), 2);
        }
        cv::putText(postScreen, postData.description, cv::Point(25, 425), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);

        if (resetRequested) {
            std::cout << "Reset button pressed!" << std::endl;
            resetRequested = false;
            currentImage = originalImage.clone();
        }

        if (sendRequested) {
            sendRequested = false;
            std::cout << "Actual send initiated" << std::endl;

            if (send(serverSock, &SIGNAL_POST, sizeof(SIGNAL_POST), 0) == -1) {
                perror("send failed");
                exit(EXIT_FAILURE);
            }

            std::cout << "sent post signal" << std::endl;

            char description[105];
            strcpy(description, postData.description.c_str());

            std::cout << "sent description: " << description << std::endl;

            if (send(serverSock, description, strlen(description), 0) == -1) {
                perror("send failed");
                exit(EXIT_FAILURE);
            }

            cv::imencode(postData.imageType, currentImage, buffer);
            uint64_t image_size = buffer.size() + 1;
            uint64_t network_order_size = htobe64(image_size);

            send(serverSock, &network_order_size, sizeof(uint64_t), 0);
            uchar* imageDataPointer = convertToPointer(buffer);
            size_t bytesSent = 0;
            while (bytesSent < image_size) {
                size_t bytesToSend = std::min((size_t) CHUNK_SIZE, image_size - bytesSent);
                int sent = send(serverSock, imageDataPointer + bytesSent, bytesToSend, 0);
                if (sent < 0) {
                    perror("Failed to send image data");
                    exit(EXIT_FAILURE);
                }
                bytesSent += sent;
            }
            
            std::cout << "sent picture" << std::endl;

            postSuccessMessageVisible = true;
            postSuccessMessageTime = std::chrono::steady_clock::now();
        }

        if (postSuccessMessageVisible) {
            cv::putText(postScreen, "Post created successfully!", cv::Point(sendButtonX - 150, sendButtonY - 20), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
            auto currentTime = std::chrono::steady_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - postSuccessMessageTime).count();
            if (elapsedTime >= 2000) { // 2000 milliseconds = 2 seconds
                postSuccessMessageVisible = false;
                GoToMainScreenFromPost();
            }
        }
        else {
            cv::rectangle(postScreen, cv::Rect(sendButtonX - 150, sendButtonY - 40, 300, 30), cv::Scalar(255, 255, 255), -1);
        }
        
        if (image_uploaded) {
            cv::Mat image = cv::imread(postData.imagePath);

            cv::imencode(postData.imageType, image, buffer);
            image_size = buffer.size() + 1;

            std::cout << "image size: " << image_size << std::endl;
            reconstructed_image = cv::imdecode(buffer, cv::IMREAD_COLOR);

            if(reconstructed_image.empty()) {
                std::cout << "Failed to decode image." << std::endl;
                return;
            }

            if(!image.empty()) {
                double aspect_ratio = (double)reconstructed_image.cols / (double)reconstructed_image.rows;

                int max_width = rightSectionRect.width;
                int max_height = rightSectionRect.height;

                int new_width = max_width;
                int new_height = static_cast<int>(max_width / aspect_ratio);
                if (new_height > max_height) {
                    new_height = max_height;
                    new_width = static_cast<int>(max_height * aspect_ratio);
                }
                
                cv::resize(reconstructed_image, reconstructed_image, cv::Size(new_width, new_height));
            }

            image_uploaded = false;
        }

        if (!reconstructed_image.empty()) {
            cv::Rect imageRect((rightSectionRect.width - reconstructed_image.cols) / 2, (rightSectionRect.height - reconstructed_image.rows) / 2, reconstructed_image.cols, reconstructed_image.rows);
        
            if (reconstructed_image.channels() != 3) {
                cv::cvtColor(reconstructed_image, reconstructed_image, cv::COLOR_GRAY2BGR);
            }

            currentImage = reconstructed_image.clone();
            originalImage = currentImage.clone();

            currentImage.copyTo(postScreen(rightSectionRect)(imageRect));
        }

        if (filterPressed) {
            filterPressed = false;

            cv::Rect imageRect((rightSectionRect.width - currentImage.cols) / 2, (rightSectionRect.height - currentImage.rows) / 2, currentImage.cols, currentImage.rows);
            currentImage.copyTo(postScreen(rightSectionRect)(imageRect));
        }

        // Show the updated login screen
        cv::imshow("Post Screen", postScreen);

        char key = cv::waitKey(10);
        if (key != -1 && postData.isTypingDescription) {
            if (key == '\b' || key == 127) {  // Backspace key or Delete key
                if (!postData.description.empty()) {
                    postData.description.pop_back();
                }
            } else if (postData.description.size() < 100) { // Limit to 100 characters
                postData.description += key;
            }
        }
    }

    disableNonBlockingInput();
}

extern "C" void setSocket(int socket) {
    serverSock = socket;
}

extern "C" void setUser(int id, const char* name) {
    user.id = id;
    strcpy(user.name, name);
}

extern "C" void setPosts(Post* dbPosts, int count, size_t *imageSizes) {
    posts = dbPosts;
    postCount = count;
    sizes = imageSizes;
}