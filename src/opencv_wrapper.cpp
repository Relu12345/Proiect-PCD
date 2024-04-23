#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include "opencv_wrapper.h"

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