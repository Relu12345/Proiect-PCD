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
#include "database.h"
#include "connection.h"
#include <cstddef>
#include "image_prc_wrapper.hpp"

struct PostData {
    std::string imagePath;
    std::string imageType;
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
std::atomic<bool> filterPressed(false);
std::atomic<bool> resetRequested(false);
std::atomic<bool> sendRequested(false);


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

void mainOnMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        // Check if the click is within the "New Post" button
        if (x >= 500 && x <= 700 && y >= 750 && y <= 800) {
            std::cout << "Post button pressed!" << std::endl;
            mainWindowVisible = false; // Close the main screen
            postWindowVisible = true; // Open the post screen
            cv::destroyWindow("Main Screen");
            createPostScreen(); // Open the createPostScreen
        }
    }
}

cv::Mat originalImage;
cv::Mat currentImage;

void postOnMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        PostData* data = (PostData*)userdata;
        if (x >= 850 && x <= 960 && y >= 700 && y <= 750) {
            std::cout << "Reset button pressed!" << std::endl;
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
                    // Apply the corresponding filter
                    switch (i) {
                        case 0: 
                            if (!currentImage.empty())
                            {
                                filterPressed = true;
                                currentImage = applyNegative(currentImage); 
                            }
                            std::cout << "Negative button pressed!" << std::endl;
                            break;
                        case 1: 
                            if (!currentImage.empty())
                            {
                                filterPressed = true;
                                currentImage = applySepia(currentImage); 
                            }
                            std::cout << "Sepia button pressed!" << std::endl;
                            break;
                        case 2: 
                            if (!currentImage.empty())
                            {
                                filterPressed = true;
                                currentImage = applyBlackAndWhite(currentImage);
                            }
                            std::cout << "B&W button pressed!" << std::endl; 
                            break;
                        case 3: 
                            if (!currentImage.empty())
                            {
                                filterPressed = true;
                                currentImage = applyBlur(currentImage); 
                            }
                            std::cout << "Blur button pressed!" << std::endl;
                            break;
                        case 4: 
                            if (!currentImage.empty())
                            {
                                filterPressed = true;
                                currentImage = applyCartoonEffect(currentImage); 
                            } 
                            std::cout << "Cartoon button pressed!" << std::endl;
                            break;
                        case 5: 
                            if (!currentImage.empty())
                            {
                                filterPressed = true;
                                currentImage = applyPencilSketch(currentImage); 
                            }
                            std::cout << "Pencil button pressed!" << std::endl;
                            break;
                        case 6: 
                            if (!currentImage.empty())
                            {
                                filterPressed = true;
                                currentImage = applyThermalVision(currentImage); 
                            }
                            std::cout << "Thermal button pressed!" << std::endl;
                            break;
                        case 7: 
                            if (!currentImage.empty())
                            {
                                filterPressed = true;
                                currentImage = applyEdgeDetection(currentImage); 
                            }
                            std::cout << "Edge button pressed!" << std::endl;
                            break;
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
            std::cout << "Cancel button pressed!" << std::endl;
            postWindowVisible = false; // Close the post screen
            mainWindowVisible = true; // Open the main screen
            cv::destroyWindow("Post Screen");
            mainScreen(); // Call mainScreen function
            return;
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

void printVectorToFile(const std::vector<uchar>& data, const std::string& filename) {
    std::ofstream outputFile(filename, std::ios::binary);
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

typedef unsigned char uchar;

std::vector<uchar> convertToVector(const uchar* data, size_t size) {
    // Initialize vector with data from the array
    return std::vector<uchar>(data, data + size);
}

uchar* convertToPointer(std::vector<uchar>& vec) {
    // Return a pointer to the beginning of the vector's data
    return vec.data();
}

extern "C" void mainScreen() {
    cv::Mat mainScreen(800, 1200, CV_8UC3, cv::Scalar(255,255,255));
    cv::namedWindow("Main Screen");
    enableNonBlockingInput();
    cv::setMouseCallback("Main Screen", mainOnMouse);
    cv::imshow("Main Screen", mainScreen);

    
    while (mainWindowVisible) {
        mainScreen = cv::Scalar(255, 255, 255);

        cv::Mat img(800, 800, CV_8UC3, cv::Scalar(255, 255, 255));

        if (posts != nullptr) {
            std::vector<uchar> imageVector = convertToVector(posts[0].image, sizes[0]);
            std::vector<uchar> binaryData = transformData(imageVector);

            cv::Mat new_img = cv::imdecode(binaryData, cv::IMREAD_COLOR);

            // Check if the image is decoded successfully
            if (new_img.empty()) {
                std::cout << "Failed to decode image." << std::endl;
                return;
            }

            img = new_img.clone();
        }

        /* For comparing images
        std::string filename = "output.txt";
        std::string filename2 = "output2.txt";

        // Print vector data to file
        printVectorToFile(imageVector, filename);
        printVectorToFile(binaryData, filename2);
        */

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
        int y = (mainScreen.rows - resizedImg.rows) / 2;

        cv::Rect imageRect(x, y, resizedImg.cols, resizedImg.rows);

        if (imageRect.x < 0 || imageRect.y < 0 || imageRect.x + imageRect.width > mainScreen.cols || imageRect.y + imageRect.height > mainScreen.rows) {
            std::cout << "Invalid region for placing image." << std::endl;
            return;
        }

        resizedImg.copyTo(mainScreen(imageRect));

        // Buttons
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
        cv::waitKey(0);
    }
    disableNonBlockingInput();
    cv::destroyWindow("Main Screen");
}

std::vector<uchar> buffer;
size_t image_size;

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

        if (resetRequested) {
            resetRequested = false;
            currentImage = originalImage.clone();
        }

        if (sendRequested) {
            sendRequested = false;
            std::cout << "Actual send initiated" << std::endl;

            if (send(serverSock, "post", strlen("post"), 0) == -1) {
                perror("send failed");
                exit(EXIT_FAILURE);
            }

            cv::imencode(postData.imageType, currentImage, buffer);
            image_size = buffer.size() + 1;

            std::cout << "sent post signal" << std::endl;
            send(serverSock, &image_size, sizeof(size_t), 0);
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
        }
        
        if (image_uploaded) {
            cv::Mat image = cv::imread(postData.imagePath);

            cv::imencode(postData.imageType, image, buffer);
            image_size = buffer.size() + 1;

            /* This is used for debug only
            std::string filename = "output.txt";
            printVectorToFile(buffer, filename);
            */

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

        cv::waitKey(1);
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