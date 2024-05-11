#ifndef OPENCV_WRAPPER_H
#define OPENCV_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

unsigned char* convertBytesToGrayscale(const unsigned char* imageData, long dataSize, int* width, int* height);
void showImageFromBytes(const unsigned char* data, int width, int height);
void createLoginScreen();
void setSocket(int socket);

#ifdef __cplusplus
}
#endif

#endif /* OPENCV_WRAPPER_H */
