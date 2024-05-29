#ifndef IMAGE_PRC_WRAPPER_HPP
#define IMAGE_PRC_WRAPPER_HPP

#include <stddef.h>

typedef struct {
    unsigned char* dataPtr;
    size_t dataSize;
} ImageData;

#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes for the filters
ImageData applyNegative(const unsigned char* data, size_t size);
ImageData applySepia(const unsigned char* data, size_t size);
ImageData applyBlackAndWhite(const unsigned char* data, size_t size);
ImageData applyBlur(const unsigned char* data, size_t size);
ImageData applyCartoonEffect(const unsigned char* data, size_t size);
ImageData applyPencilSketch(const unsigned char* data, size_t size);
ImageData applyThermalVision(const unsigned char* data, size_t size);
ImageData applyEdgeDetection(const unsigned char* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* FILTERS_HPP */
