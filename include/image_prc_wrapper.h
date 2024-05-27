#ifndef IMAGE_PRC_WRAPPER_HPP
#define IMAGE_PRC_WRAPPER_HPP

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Function prototypes for the filters
unsigned char * applyNegative(const unsigned char* data, size_t size);
unsigned char * applySepia(const unsigned char* data, size_t size);
unsigned char * applyBlackAndWhite(const unsigned char* data, size_t size);
unsigned char * applyBlur(const unsigned char* data, size_t size);
unsigned char * applyCartoonEffect(const unsigned char* data, size_t size);
unsigned char * applyPencilSketch(const unsigned char* data, size_t size);
unsigned char * applyThermalVision(const unsigned char* data, size_t size);
unsigned char * applyEdgeDetection(const unsigned char* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* FILTERS_HPP */
