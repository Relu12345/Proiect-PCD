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

//pt admin, returneaza lista de useri
struct Post* get_all_posts(PGconn* conn, int userId) {
    int buffer_len = snprintf(NULL, 0, "SELECT p.*, u.name, COALESCE(COUNT(ulp.id), 0) AS like_count,\n"
                        "COALESCE(BOOL_OR(ulp.user_id = %d), false) AS liked\n"
                        "FROM post p\n"
                        "INNER JOIN users u ON p.user_id = u.id\n"
                        "LEFT JOIN user_liked_post ulp ON p.id = ulp.post_id\n"
                        "GROUP BY p.id, u.name;", userId);
    char query[buffer_len + 1];
    snprintf(query, buffer_len + 1, "SELECT p.*, u.name, COALESCE(COUNT(ulp.id), 0) AS like_count,\n"
                                    "COALESCE(BOOL_OR(ulp.user_id = %d), false) AS liked\n"
                                    "FROM post p\n"
                                    "INNER JOIN users u ON p.user_id = u.id\n"
                                    "LEFT JOIN user_liked_post ulp ON p.id = ulp.post_id\n"
                                    "GROUP BY p.id, u.name;", userId);
    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    int num_rows = PQntuples(res);

    struct Post* posts = malloc(sizeof(posts) * MAX_USERS);
    if (posts == NULL) {
        fprintf(stderr, "Failed to allocate memory for users\n");
        PQclear(res);
        return NULL;
    }

    int post_index = 0;
    for (int row = 0; row < num_rows; row++) {
        if (post_index >= MAX_USERS) {
            fprintf(stderr, "Warning: Reached maximum users (%d). Increase MAX_USERS if needed\n", MAX_USERS);
            break;
        }

        printf("%s", (PQgetvalue(res, row, 6)));
        posts[post_index].id = atoi(PQgetvalue(res, row, 0));
        posts[post_index].userId = atoi(PQgetvalue(res, row, 1));
        const unsigned char* image_data = PQgetvalue(res, row, 2);
        int image_len = PQgetlength(res, row, 2);
        posts[post_index].image = malloc(image_len);
        memcpy(posts[post_index].image, image_data, image_len);
        posts[post_index].description = strdup(PQgetvalue(res, row, 3));
        posts[post_index].userName = strdup(PQgetvalue(res, row, 4));
        posts[post_index].likeCount = atoi(PQgetvalue(res, row, 5));
        posts[post_index].liked = (strcmp(PQgetvalue(res, row, 6), "t") == 0) ? true : false;
        post_index++;
    }

    PQclear(res);
    return posts;
}

struct Post* get_all_user_posts(PGconn* conn, int userId) {
    int buffer_len = snprintf(NULL, 0, "SELECT p.*, u.name\n"
                                       "FROM post p\n"
                                       "INNER JOIN users u ON p.user_id = u.id where p.user_id = %d", userId);
    char query[buffer_len + 1];
    snprintf(query, buffer_len + 1, "SELECT p.*, u.name\n"
                                    "FROM post p\n"
                                    "INNER JOIN users u ON p.user_id = u.id where p.user_id = %d", userId);
    PGresult* res = PQexec(conn, query);


    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    int num_rows = PQntuples(res);

    struct Post* posts = malloc(sizeof(posts) * MAX_USERS);
    if (posts == NULL) {
        fprintf(stderr, "Failed to allocate memory for users\n");
        PQclear(res);
        return NULL;
    }

    int post_index = 0;
    for (int row = 0; row < num_rows; row++) {
        if (post_index >= MAX_USERS) {
            fprintf(stderr, "Warning: Reached maximum users (%d). Increase MAX_USERS if needed\n", MAX_USERS);
            break;
        }


        posts[post_index].id = atoi(PQgetvalue(res, row, 0));
        posts[post_index].userId = atoi(PQgetvalue(res, row, 1));
        const unsigned char* image_data = PQgetvalue(res, row, 2);
        int image_len = PQgetlength(res, row, 2);
        posts[post_index].image = malloc(image_len);
        memcpy(posts[post_index].image, image_data, image_len);
        posts[post_index].description = strdup(PQgetvalue(res, row, 3));
        posts[post_index].userName = strdup(PQgetvalue(res, row, 4));

        post_index++;
    }

    PQclear(res);
    return posts;
}


struct User* get_all_users(PGconn* conn) {
    const char* query = "SELECT id, name FROM users";

    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    int num_rows = PQntuples(res);

    struct User* users = malloc(sizeof(users) * MAX_USERS);
    if (users == NULL) {
        fprintf(stderr, "Failed to allocate memory for users\n");
        PQclear(res);
        return NULL;
    }

    int user_index = 0;
    for (int row = 0; row < num_rows; row++) {
        if (user_index >= MAX_USERS) {
            fprintf(stderr, "Warning: Reached maximum users (%d). Increase MAX_USERS if needed\n", MAX_USERS);
            break;
        }

        users[user_index].id = atoi(PQgetvalue(res, row, 0));
        users[user_index].name = strdup(PQgetvalue(res, row, 1));

        user_index++;
    }

    PQclear(res);
    return users;
}


bool register_user(PGconn* conn, const char* username, const char* password) {
    if (username == NULL || username[0] == '\0' || password == NULL || password[0] == '\0') {
        fprintf(stderr, "Error: Username and password cannot be empty.\n");
        return false;
    }

    const char* params[] = {username, password};
    const char* query = "INSERT INTO users (name, password) VALUES ($1, $2)";

    PGresult* res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        if (PQresultStatus(res) == PGRES_NONFATAL_ERROR) {
            fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        }
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}


bool insertPost(PGconn* conn, int user_id, void* image, size_t image_size, const char* description) {
    if (image == NULL || image_size == 0 || description == NULL || description[0] == '\0') {
        fprintf(stderr, "Error: Image, image size, and description cannot be empty.\n");
        return false;
    }
    size_t escaped_len;
    char* escaped_image = PQescapeByteaConn(conn, (const unsigned char*)image, image_size, &escaped_len);
    int buffer_len = snprintf(NULL, 0, "INSERT INTO post (user_id, image, description) VALUES (%d, '%s', '%s')", user_id, escaped_image, "description dhuhausdosahu duadh osa hudsaoh");
    char query[buffer_len + 1];

    snprintf(query, buffer_len + 1, "INSERT INTO post (user_id, image, description) VALUES (%d, '%s', '%s')", user_id, escaped_image, "description dhuhausdosahu duadh osa hudsaoh");

    PGresult* res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool deletePost(PGconn* conn, int post_id) {
    int buffer_len = snprintf(NULL, 0, "DELETE FROM post WHERE id = %d", post_id);
    char query[buffer_len + 1];
    snprintf(query, buffer_len + 1, "DELETE FROM post WHERE id = %d", post_id);

    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Error deleting post: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}


//Important! primeste ca ultim parametru return code;
//1 fail
//0 succes
struct User login_user(PGconn* conn, const char* username, const char* password, int* returnCode) {
    struct User user;
    const char* params[] =  {username, password};
    const char* query = "SELECT id, name FROM users WHERE name = $1 AND password = $2";
    PGresult* res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        if (PQresultStatus(res) == PGRES_NONFATAL_ERROR) {
            fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        }
        PQclear(res);
        *returnCode = 1;
        return user;
    }

    int num_rows = PQntuples(res);

    if (num_rows != 1) {
        PQclear(res);
        return user;
    }
    user.id = atoi(PQgetvalue(res, 0, 0));
    user.name = strdup(PQgetvalue(res, 0, 1));

    *returnCode = 0;
    PQclear(res);
    return user;
}



int handle_error(const char* message) {
    fprintf(stderr, "Error: %s\n", message);
    return 1;
}

void* allocate_memory(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        handle_error("Memory allocation failed");
        return NULL;
    }
    return ptr;
}


int main(int argc, char* argv[]) {
  // Your server code goes here
  printf("Server started!\n");
  // ...

  return 0;
}