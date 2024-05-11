#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include "opencv_wrapper.h"
#include "../../../../../../usr/include/x86_64-linux-gnu/sys/socket.h"
#include <string>
#include <iostream>
#include <unistd.h>
#include <sys/un.h>
#include <termios.h>
#include <fcntl.h>
#include <cstdio>
#include <atomic>

extern "C" unsigned char* convertBytesToGrayscale(const unsigned char* imageData, long dataSize, int* width, int* height) {
    // Decode the image bytes
    cv::Mat image = cv::imdecode(cv::Mat(1, dataSize, CV_8UC1, (void*)imageData), cv::IMREAD_COLOR);
    if (image.empty()) {
        *width = 0;
        *height = 0;
        return nullptr;
    }

    // Convert the image to grayscale
    cv::Mat grayImage;
    cv::cvtColor(image, grayImage, cv::COLOR_BGR2GRAY);

    // Prepare the byte array
    *width = grayImage.cols;
    *height = grayImage.rows;
    unsigned char* byteArray = new unsigned char[*width * *height];
    
    // Copy grayscale values to the byte array
    for (int i = 0; i < grayImage.rows; ++i) {
        for (int j = 0; j < grayImage.cols; ++j) {
            byteArray[i * grayImage.cols + j] = grayImage.at<uchar>(i, j);
        }
    }

    return byteArray;
}

extern "C" void showImageFromBytes(const unsigned char* data, int width, int height) {
    cv::namedWindow("Grayscale Image", cv::WINDOW_NORMAL);
    cv::resizeWindow("Grayscale Image", width, height);
    cv::imshow("Grayscale Image", cv::Mat(height, width, CV_8UC1, (void*)data));

    cv::waitKey(0);
    cv::destroyAllWindows();
}

struct LoginData {
    std::string username;
    std::string password;
    bool isTypingUsername;
    bool isTypingPassword;

    LoginData() : isTypingUsername(false), isTypingPassword(false) {}
};

LoginData loginData;

std::atomic<bool> loginWindowVisible(true);

// Global variable to hold the client socket
int serverSock;

// Function to send message to client
void sendMessageToServer(const char* user, const char* pass) {
    // Create a buffer to hold the combined message
    char combinedMessage[256]; // Adjust the buffer size as needed

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
        else if (x >= 150 && x <= 250 && y >= 240 && y <= 280) {
            std::cout << "Login button pressed." << std::endl;
            std::cout << "Username: " << data->username << std::endl;
            std::cout << "Password: " << data->password << std::endl;
            // Send username and password to client
            sendMessageToServer(data->username.c_str(), data->password.c_str());
            loginWindowVisible = false;
            cv::destroyWindow("Login Screen");
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

extern "C" void createLoginScreen() {
    // Create a window
    cv::Mat loginScreen(300, 400, CV_8UC3, cv::Scalar(255, 255, 255));
    cv::namedWindow("Login Screen");

    // Define labels
    std::string usernameLabel = "Username:";
    std::string passwordLabel = "Password:";
    std::string loginLabel = "Login";

    // Draw labels
    cv::putText(loginScreen, "Login", cv::Point(180, 50), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 0), 2);
    cv::putText(loginScreen, usernameLabel, cv::Point(50, 120), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
    cv::putText(loginScreen, passwordLabel, cv::Point(50, 180), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);

    // Draw input fields with initial text
    cv::rectangle(loginScreen, cv::Rect(200, 100, 150, 30), cv::Scalar(0, 0, 0), 2);
    cv::putText(loginScreen, loginData.username, cv::Point(203, 125), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);

    cv::rectangle(loginScreen, cv::Rect(200, 160, 150, 30), cv::Scalar(0, 0, 0), 2);
    cv::putText(loginScreen, loginData.password, cv::Point(203, 185), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);

    // Draw login button
    cv::rectangle(loginScreen, cv::Rect(150, 240, 100, 40), cv::Scalar(0, 0, 255), -1);
    cv::putText(loginScreen, loginLabel, cv::Point(180, 270), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

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
        cv::putText(loginScreen, "Login", cv::Point(180, 50), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 0), 2);
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
        cv::putText(loginScreen, loginData.password, cv::Point(205, 185), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 2);


        // Draw login button
        cv::rectangle(loginScreen, cv::Rect(150, 240, 100, 40), cv::Scalar(0, 0, 255), -1);
        cv::putText(loginScreen, loginLabel, cv::Point(180, 270), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

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


extern "C" void setSocket(int socket) {
    serverSock = socket;
}