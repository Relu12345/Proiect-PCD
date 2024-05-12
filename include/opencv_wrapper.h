#ifndef OPENCV_WRAPPER_H
#define OPENCV_WRAPPER_H

#include <libpq-fe.h>
#include "database.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned char* convertBytesToGrayscale(const unsigned char* imageData, long dataSize, int* width, int* height);
void showImageFromBytes(const unsigned char* data, int width, int height);
void createLoginScreen();
void createPostScreen();
void setSocket(int socket);
void mainScreen ();
void setPosts(struct Post *dbPosts);

#ifdef __cplusplus
}
#endif

#endif /* OPENCV_WRAPPER_H */
