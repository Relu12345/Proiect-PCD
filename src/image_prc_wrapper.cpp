#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <iostream>

// Function to apply Negative effect
cv::Mat applyNegative(const cv::Mat& image) {
    cv::Mat result;
    cv::bitwise_not(image, result);
    if (result.channels() == 1) {
        cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    }
    return result;
}

// Function to apply Sepia effect
cv::Mat applySepia(const cv::Mat& image) {
    cv::Mat result = image.clone();
    cv::transform(result, result, cv::Matx33f(0.272, 0.534, 0.131,
                                              0.349, 0.686, 0.168,
                                              0.393, 0.769, 0.189));
    if (result.channels() == 1) {
        cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    }
    return result;
}

// Function to apply Black and White effect
cv::Mat applyBlackAndWhite(const cv::Mat& image) {
    cv::Mat result;
    cv::cvtColor(image, result, cv::COLOR_BGR2GRAY);
    cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    return result;
}

// Function to apply Gaussian Blur
cv::Mat applyBlur(const cv::Mat& image) {
    cv::Mat result;
    cv::GaussianBlur(image, result, cv::Size(15, 15), 10);
    return result;
}

// Function to apply Cartoon effect
cv::Mat applyCartoonEffect(const cv::Mat& image) {
    cv::Mat gray, edges, result;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(gray, edges, 7);
    cv::adaptiveThreshold(edges, edges, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 9, 2);
    cv::cvtColor(edges, edges, cv::COLOR_GRAY2BGR);
    cv::bitwise_and(image, edges, result);
    return result;
}

// Function to apply Pencil Sketch effect
cv::Mat applyPencilSketch(const cv::Mat& image) {
    cv::Mat gray, blur, sketch;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, blur, cv::Size(21, 21), 0);
    cv::divide(gray, blur, sketch, 256.0);
    cv::cvtColor(sketch, sketch, cv::COLOR_GRAY2BGR);
    return sketch;
}

// Function to apply Thermal Vision effect
cv::Mat applyThermalVision(const cv::Mat& image) {
    cv::Mat result;
    cv::applyColorMap(image, result, cv::COLORMAP_JET);
    return result;
}

// Function to apply Edge Detection effect
cv::Mat applyEdgeDetection(const cv::Mat& image) {
    cv::Mat gray, edges;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Canny(gray, edges, 100, 200);
    cv::cvtColor(edges, edges, cv::COLOR_GRAY2BGR);
    return edges;
}
