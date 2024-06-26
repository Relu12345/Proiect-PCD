#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <string.h>
#include <stdbool.h>

#define MAX_USERS 100

struct User {
    int id;
    char name[100];
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

bool like_or_remove_like(PGconn* conn, int user_id, int post_id) {
    char query[100];
    snprintf(query, sizeof(query), "SELECT ID FROM user_liked_post WHERE user_id = %d AND post_id = %d", user_id, post_id);

    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    
    if (PQntuples(res) > 0) {
        // Like exists, so delete it
        char delete_query[100];
        snprintf(delete_query, sizeof(delete_query), "DELETE FROM user_liked_post WHERE user_id = %d AND post_id = %d", user_id, post_id);

        PGresult* delete_res = PQexec(conn, delete_query);

        if (PQresultStatus(delete_res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "Error executing delete query: %s\n", PQerrorMessage(conn));
            PQclear(delete_res);
            PQclear(res);
            return -1;
        }

        PQclear(delete_res);
    } else {
        char insert_query[100];
        snprintf(insert_query, sizeof(insert_query), "INSERT INTO user_liked_post (post_id, user_id) VALUES (%d, %d)", post_id, user_id);

        PGresult* insert_res = PQexec(conn, insert_query);

        if (PQresultStatus(insert_res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "Error executing insert query: %s\n", PQerrorMessage(conn));
            PQclear(insert_res);
            PQclear(res);
            return -1;
        }

        PQclear(insert_res);
    }

    PQclear(res);
    return true;
}

int get_posts_counts(PGconn* conn) {
    int count = 0;

    char query[100]; 
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM post");

    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) > 0) {
        count = atoi(PQgetvalue(res, 0, 0));
    }

    PQclear(res);
    return count;
}

int get_user_posts_counts(PGconn* conn, int userId) {
    int count = 0;

    char query[100]; 
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM post WHERE user_id = %d", userId);

    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    if (PQntuples(res) > 0) {
        count = atoi(PQgetvalue(res, 0, 0));
    }

    PQclear(res);
    return count;
}

struct Post* get_posts(PGconn* conn) {
    int buffer_len = snprintf(NULL, 0, "SELECT p.*, u.name\n"
                        "FROM post p\n"
                        "INNER JOIN users u ON p.user_id = u.id\n"
                        "GROUP BY p.id, u.name;");
    char query[buffer_len + 1];
    snprintf(query, buffer_len + 1, "SELECT p.*, u.name\n"
                                    "FROM post p\n"
                                    "INNER JOIN users u ON p.user_id = u.id\n"
                                    "GROUP BY p.id, u.name;");
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

    // int post_index = 0;
    for (int row = 0; row < num_rows; row++) {
        //printf("%s", (PQgetvalue(res, row, 4)));
        posts[row].id = atoi(PQgetvalue(res, row, 0));
        posts[row].userId = atoi(PQgetvalue(res, row, 1));
        const unsigned char* image_data = PQgetvalue(res, row, 2);
        int image_len = PQgetlength(res, row, 2);
        posts[row].image = malloc(image_len);
        memcpy(posts[row].image, image_data, image_len);
        posts[row].description = strdup(PQgetvalue(res, row, 3));
        posts[row].userName = strdup(PQgetvalue(res, row, 4));
        // post_index++;
    }

    PQclear(res);
    return posts;
}


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


struct User* get_all_users(PGconn* conn, int* count) {
    const char* query = "SELECT id, name FROM users";

    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    int num_rows = PQntuples(res);

    // Allocate memory for the users array
    struct User* users = malloc(sizeof(struct User) * num_rows);
    if (users == NULL) {
        fprintf(stderr, "Failed to allocate memory for users\n");
        PQclear(res);
        return NULL;
    }

    for (int row = 0; row < num_rows; row++) {
        users[row].id = atoi(PQgetvalue(res, row, 0));
        strcpy(users[row].name, PQgetvalue(res, row, 1));
    }

    *count = num_rows;

    PQclear(res);
    return users;
}


bool block_user(PGconn* conn, int user_id) {
    // Check if the user is already blocked
    char query_check[100];
    snprintf(query_check, sizeof(query_check), "SELECT blocked FROM users WHERE id = %d", user_id);

    PGresult* res_check = PQexec(conn, query_check);

    if (PQresultStatus(res_check) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res_check);
        return false;
    }

    bool already_blocked = false;
    if (PQntuples(res_check) > 0) {
        already_blocked = (PQgetvalue(res_check, 0, 0)[0] == 't') ? true : false;
    }

    PQclear(res_check);

    // Toggle the blocked status
    char query_block[100];
    if (already_blocked) {
        snprintf(query_block, sizeof(query_block), "UPDATE users SET blocked = false WHERE id = %d", user_id);
    } else {
        snprintf(query_block, sizeof(query_block), "UPDATE users SET blocked = true WHERE id = %d", user_id);
    }

    PGresult* res_block = PQexec(conn, query_block);

    if (PQresultStatus(res_block) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res_block);
        return false;
    }

    PQclear(res_block);
    return true;
}


bool is_user_blocked(PGconn* conn, int user_id) {
    // Check if the user is blocked
    char query[100];
    snprintf(query, sizeof(query), "SELECT blocked FROM users WHERE id = %d", user_id);

    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error executing query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return false;
    }

    bool blocked = false;
    if (PQntuples(res) > 0) {
        blocked = (PQgetvalue(res, 0, 0)[0] == 't') ? true : false;
    }

    PQclear(res);
    return blocked;
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
    int buffer_len = snprintf(NULL, 0, "INSERT INTO post (user_id, image, description) VALUES (%d, '%s', '%s')", user_id, escaped_image, description);
 
    char* query = (char*)malloc(buffer_len + 1);
    if (query == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return false;
    }

    snprintf(query, buffer_len + 1, "INSERT INTO post (user_id, image, description) VALUES (%d, '%s', '%s')", user_id, escaped_image, description);

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
    const char* query = "SELECT id, name, blocked FROM users WHERE name = $1 AND password = $2";
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
    strcpy(user.name, PQgetvalue(res, 0, 1));
    bool blocked;
    const char *boolValue = PQgetvalue(res, 0, 2);
    if (boolValue[0] == 't') {
        blocked = true;
    } else if (boolValue[0] == 'f') {
        blocked = false;
    }

    if (blocked)
        *returnCode = 2;
    else
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