#ifndef INTERFACE_WRAPPER_H
#define INTERFACE_WRAPPER_H

#include <libpq-fe.h>
#include "database.h"
#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif

void createLoginScreen();
void createPostScreen();
void setSocket(int socket);
void mainScreen ();
void setUser(int id, const char* name);
void setPosts(struct Post* dbPosts, int count, size_t *imageSizes);
unsigned char* getImage();
int getImageSize();

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_WRAPPER_H */
