#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <iostream>

struct ImageData {
    unsigned char* dataPtr;
    size_t dataSize;
};

cv::Mat decodeImage(const unsigned char* data, size_t size) {
    std::vector<uchar> buffer(data, data + size);
    return cv::imdecode(buffer, cv::IMREAD_COLOR);
}

ImageData encodeImage(const cv::Mat& image) {
    std::vector<uchar> buffer;
    cv::imencode(".jpg", image, buffer);

    ImageData imageData;
    imageData.dataPtr = buffer.data();
    imageData.dataSize = buffer.size();

    return imageData;
}

// Function to apply Negative effect
extern "C" ImageData applyNegative(const unsigned char* data, size_t size) {
    cv::Mat image = decodeImage(data, size);
    cv::Mat result;
    cv::bitwise_not(image, result);
    if (result.channels() == 1) {
        cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    }
    ImageData encodedImage = encodeImage(result);
    result.release();
    return encodedImage;
}

// Function to apply Sepia effect
extern "C" ImageData applySepia(const unsigned char* data, size_t size) {
    cv::Mat image = decodeImage(data, size);
    cv::Mat result = image.clone();
    cv::transform(result, result, cv::Matx33f(0.272, 0.534, 0.131,
                                              0.349, 0.686, 0.168,
                                              0.393, 0.769, 0.189));
    if (result.channels() == 1) {
        cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    }
    ImageData encodedImage = encodeImage(result);
    result.release();
    return encodedImage;
}

// Function to apply Black and White effect
extern "C" ImageData applyBlackAndWhite(const unsigned char* data, size_t size) {
    cv::Mat image = decodeImage(data, size);
    cv::Mat result;
    cv::cvtColor(image, result, cv::COLOR_BGR2GRAY);
    cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    ImageData encodedImage = encodeImage(result);
    result.release();
    return encodedImage;
}

// Function to apply Gaussian Blur
extern "C" ImageData applyBlur(const unsigned char* data, size_t size) {
    cv::Mat image = decodeImage(data, size);
    cv::Mat result;
    cv::GaussianBlur(image, result, cv::Size(15, 15), 10);
    ImageData encodedImage = encodeImage(result);
    result.release();
    return encodedImage;
}

// Function to apply Cartoon effect
extern "C" ImageData applyCartoonEffect(const unsigned char* data, size_t size) {
    cv::Mat image = decodeImage(data, size);
    cv::Mat gray, edges, result;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(gray, edges, 7);
    cv::adaptiveThreshold(edges, edges, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 9, 2);
    cv::cvtColor(edges, edges, cv::COLOR_GRAY2BGR);
    cv::bitwise_and(image, edges, result);
    ImageData encodedImage = encodeImage(result);
    result.release();
    return encodedImage;
}

// Function to apply Pencil Sketch effect
extern "C" ImageData applyPencilSketch(const unsigned char* data, size_t size) {
    cv::Mat image = decodeImage(data, size);
    cv::Mat gray, blur, sketch;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, blur, cv::Size(21, 21), 0);
    cv::divide(gray, blur, sketch, 256.0);
    cv::cvtColor(sketch, sketch, cv::COLOR_GRAY2BGR);
    ImageData encodedImage = encodeImage(sketch);
    sketch.release();
    return encodedImage;
}

// Function to apply Thermal Vision effect
extern "C" ImageData applyThermalVision(const unsigned char* data, size_t size) {
    cv::Mat image = decodeImage(data, size);
    cv::Mat result;
    cv::applyColorMap(image, result, cv::COLORMAP_JET);
    ImageData encodedImage = encodeImage(result);
    result.release();
    return encodedImage;
}

// Function to apply Edge Detection effect
extern "C" ImageData applyEdgeDetection(const unsigned char* data, size_t size) {
    cv::Mat image = decodeImage(data, size);
    cv::Mat gray, edges;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Canny(gray, edges, 100, 200);
    cv::cvtColor(edges, edges, cv::COLOR_GRAY2BGR);
    ImageData encodedImage = encodeImage(edges);
    edges.release();
    return encodedImage;
}
