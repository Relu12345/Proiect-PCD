#ifndef IMAGE_PRC_WRAPPER_HPP
#define IMAGE_PRC_WRAPPER_HPP

#include <opencv4/opencv2/opencv.hpp>

// Function prototypes for the filters
cv::Mat applyNegative(const cv::Mat& image);
cv::Mat applySepia(const cv::Mat& image);
cv::Mat applyBlackAndWhite(const cv::Mat& image);
cv::Mat applyBlur(const cv::Mat& image);
cv::Mat applyCartoonEffect(const cv::Mat& image);
cv::Mat applyPencilSketch(const cv::Mat& image);
cv::Mat applyThermalVision(const cv::Mat& image);
cv::Mat applyEdgeDetection(const cv::Mat& image);

#endif /* FILTERS_HPP */
