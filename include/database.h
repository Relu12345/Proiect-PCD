#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <string.h>
#include <stdbool.h>

#define MAX_USERS 100

struct User {
    int id;
    char* name;
};

struct Post {
    int id;
    int userId;
    unsigned char* image;
    char* description;
    char* userName;
    int likeCount;
    bool liked;
};

struct Post* get_all_posts(PGconn* conn, int userId);

struct Post* get_all_user_posts(PGconn* conn, int userId);

struct User* get_all_users(PGconn* conn);

bool register_user(PGconn* conn, const char* username, const char* password);

bool insertPost(PGconn* conn, int user_id, void* image, size_t image_size, const char* description);

bool deletePost(PGconn* conn, int post_id);

struct User login_user(PGconn* conn, const char* username, const char* password, int* returnCode);

int handle_error(const char* message);

void* allocate_memory(size_t size);

#endif /* DATABASE_H */