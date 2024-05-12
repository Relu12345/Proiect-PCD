#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include "opencv_wrapper.h"
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
#include "database.h"

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

struct PostData {
    std::string imagePath;
} postData;

struct LoginData {
    std::string username;
    std::string password;
    bool isTypingUsername;
    bool isTypingPassword;

    LoginData() : isTypingUsername(false), isTypingPassword(false) {}
};

LoginData loginData;

std::atomic<bool> loginWindowVisible(true);

std::atomic<bool> mainWindowVisible(true);

std::atomic<bool> postWindowVisible(true);

std::atomic<bool> image_uploaded(false);


// Global variable to hold the client socket
int serverSock;
int postCount;
Post* posts = nullptr;
User user;
size_t *sizes = nullptr;

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

// void mainOnMouse(int event, int x, int y, int flags, void* userdata) {
//     if (event == cv::EVENT_LBUTTONDOWN) {

//     }
// }

void postOnMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        PostData* data = (PostData*)userdata;
        if (x >= 300 && x <= 400 && y >= 300 && y <= 340) {
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
            data->imagePath = selectFile;
            image_uploaded = true;
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

        // Draw register button
        cv::rectangle(loginScreen, cv::Rect(240, 240, 100, 40), cv::Scalar(0, 0, 255), -1);
        cv::putText(loginScreen, loginLabel, cv::Point(260, 270), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

        // Draw login button
        cv::rectangle(loginScreen, cv::Rect(50, 240, 160, 40), cv::Scalar(0, 0, 255), -1);
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

typedef unsigned char uchar;

std::vector<uchar> convertToVector(const uchar* data, size_t size) {
    // Initialize vector with data from the array
    return std::vector<uchar>(data, data + size);
}

extern "C" void mainScreen() {
    cv::Mat mainScreen(800, 1200, CV_8UC3, cv::Scalar(255,255,255));
    cv::namedWindow("Main Screen");
    cv::imshow("Main Screen", mainScreen);
    // cv::setMouseCallback("Main Screen", mainOnMouse, &posts);

    
    while (mainWindowVisible) {
        mainScreen = cv::Scalar(255, 255, 255);
        // nothing yet
        std::cout << "rando id: " << posts[0].id << std::endl;
        // Username
        
        cv::putText(mainScreen, std::to_string(posts[0].id), cv::Point(200, 200), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 0), 2);
        std::vector<uchar> imageVector = convertToVector(posts[0].image, sizes[0]);
        cv::Mat img = cv::Mat(400, 300, CV_8UC3, (void*)posts[0].image);
        cv::Rect imageRect((mainScreen.cols - img.cols) / 2, 50, img.cols, img.rows);
        img.copyTo(mainScreen(imageRect));

        // Buttons
        cv::Rect leftButtonRect(150, 400, 100, 50);
        cv::rectangle(mainScreen, leftButtonRect, cv::Scalar(0, 0, 255), -1);
        cv::Rect rightButtonRect(950, 400, 100, 50);
        cv::rectangle(mainScreen, rightButtonRect, cv::Scalar(255, 0, 0), -1);

        cv::Rect upperLeftArrowRect(50, 200, 50, 50);
        cv::rectangle(mainScreen, upperLeftArrowRect, cv::Scalar(0, 0, 0), -1);
        cv::Rect upperRightArrowRect(1150, 200, 50, 50);
        cv::rectangle(mainScreen, upperRightArrowRect, cv::Scalar(0, 0, 0), -1);

        cv::Rect makePostButtonRect(500, 750, 200, 50);
        cv::rectangle(mainScreen, makePostButtonRect, cv::Scalar(0, 255, 0), -1);
        cv::putText(mainScreen, "Make post", cv::Point(550, 785), cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 0, 0), 2);

        // Show the window
        cv::imshow("Main Screen", mainScreen);

        // Wait for a key press
        cv::waitKey(0);

    }
}

extern "C" void createPostScreen () {
    cv::Mat postScreen(800, 1200, CV_8UC3, cv::Scalar(255,255,255));
    cv::namedWindow("Post Screen");
    enableNonBlockingInput();
    cv::imshow("Post Screen", postScreen);
    cv::setMouseCallback("Post Screen", postOnMouse, &postData);

    int leftSectionWidth = 600;
    cv::Rect leftSectionRect(0, 0, leftSectionWidth, postScreen.rows);

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
        int rightSectionWidth = postScreen.cols - leftSectionWidth;
        cv::Rect rightSectionRect(leftSectionWidth, 0, rightSectionWidth, postScreen.rows);
        
        if (image_uploaded) {
            cv::Mat image = cv::imread(postData.imagePath);
            if(!image.empty()) {
                cv::resize(image, image, cv::Size(rightSectionRect.width, rightSectionRect.height));

                // Copy the image to the right section
                image.copyTo(postScreen(rightSectionRect));
            }
        }
        // Show the updated login screen
        cv::imshow("Post Screen", postScreen);
        
        char key = cv::waitKey(10);
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