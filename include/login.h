#ifndef LOGIN_H
#define LOGIN_H

void disableEcho();
void enableEcho();
int login(char *username, char *password);
int create_user(char *username, char *password);
void processClientInfo(const char* message, char* username, char* password);

#endif /* LOGIN_H */