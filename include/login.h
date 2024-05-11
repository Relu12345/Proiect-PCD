#include <libpq-fe.h>
#include <stdbool.h>

#ifndef LOGIN_H
#define LOGIN_H

void disableEcho();
void enableEcho();
int login(PGconn *conn, char *username, char *password);
bool ps_register(PGconn *conn, char *username, char *password);
int create_user(char *username, char *password);
void processClientInfo(const char* message, char* username, char* password);

#endif /* LOGIN_H */
