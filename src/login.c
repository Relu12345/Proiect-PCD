
#include <stdio.h> // Folosit pentru: scanf, printf, perror, fprintf, stderr, fgets, fseek, fopen, fclose
#include <stdlib.h> // Folosit pentru: exit, EXIT_FAILURE
#include <unistd.h> // Folosit pentru: fork, execl, STDIN_FILENO
#include <termios.h> // Folosit pentru: termios, tcgetattr, tcsetattr, ECHO, TCSANOW
#include <string.h> // Folosit pentru: strcmp, strcspn 
#include <errno.h> // Folosit pentru: errno, ENOENT
#include <sys/wait.h> // Folosit pentru: waitpid, WIFEXITED, WEXITSTATUS

#define MAX_COMMAND_LENGTH 100 // Numarul maxim de caractere pentru lungimea unei comenzi
#define MAX_USERNAME_LENGTH 50 // Numarul maxim de caractere pentru lungimea numelui de utilizator
#define MAX_PASSWORD_LENGTH 50 // Numarul maxim de caractere pentru lungimea parolei utilizatorului

// Functie prin care oprim afisarea la consola
void disableEcho() {
    // Definim o structura pentru termios
    struct termios term;
    // Facem rost de atributul stdin si il punem in structura term
    tcgetattr(STDIN_FILENO, &term);
    // Setam flag-ul la echo (afisarea la terminal) ca oprit
    term.c_lflag &= ~(ECHO);
    // Setam atributul STDIN la "nu afisa" pentru echo
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// Functie prin care pornim afisarea la consola
void enableEcho() {
    // Definim o structura pentru termios
    struct termios term;
    // Facem rost de atributul stdin si il punem in structura term
    tcgetattr(STDIN_FILENO, &term);
    // Setam flag-ul la echo (afisarea la terminal) ca pornit
    term.c_lflag |= ECHO;
    // Setam atributul STDIN la "afiseaza" pentru echo
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void processClientInfo(const char* message, char* username, char* password) {
    // Tokenize the message based on the delimiter
    char* token = strtok((char*)message, ",");
    if (token != NULL) {
        // Extract the username
        strcpy(username, token);
        // Move to the next token to extract the password
        token = strtok(NULL, ",");
        if (token != NULL) {
            // Extract the password
            strcpy(password, token);
        }
    }
}

int login(char *username, char *password) {
    FILE *file = fopen("/tmp/credentials.txt", "r");

    if (file == NULL) {
        perror("Eroare");
        fprintf(stderr, "Probabil nu exista nici un user. Incearca sa creezi un user!\n");
        return 1;
    }

    char stored_username[MAX_USERNAME_LENGTH];
    char stored_password[MAX_PASSWORD_LENGTH];

    while (fscanf(file, "%s %s", stored_username, stored_password) != EOF) {
        if (strcmp(username, stored_username) == 0 && strcmp(password, stored_password) == 0) {
            printf("Utilizatorul a fost autentificat cu succes!\n");
            fclose(file);
            return 0;
        }
    }

    fclose(file);

    if (strcmp(username, stored_username) != 0 && strcmp(password, stored_password) != 0) {
        return 2;
    } else if (strcmp(username, stored_username) != 0) {
        return 2;
    } else if (strcmp(password, stored_password) != 0) {
        return 2;
    }

    return 1;
}

int create_user(char *username, char* password) {
    FILE *file = fopen("/tmp/credentials.txt", "r+");

    if (file == NULL) {
        if (errno == ENOENT) {
            file = fopen("/tmp/credentials.txt", "w+");
            if (file == NULL) {
                fprintf(stderr, "Eroare: Nu s-a putut crea fisierul de credentiale");
                return 1;
            }
        }
        else {
            fprintf(stderr, "Eroare: Nu s-a putut deschide fisierul de credentiale");
            return 1;
        }
    }

    char stored_username[MAX_USERNAME_LENGTH];
    char stored_password[MAX_PASSWORD_LENGTH];

    int user_exists = 0;

    while (fscanf(file, "%s %s", stored_username, stored_password) != EOF) {
        if (strcmp(username, stored_username) == 0) {
            user_exists = 1;
            break;
        }
    }

    if (user_exists) {
        fprintf(stderr, "Eroare: Acest utilizator exista deja!\n");
        return 1;
    } else {
        int valFseek = fseek(file, 0, SEEK_END);
        if (valFseek == 0) {
            fprintf(file, "%s %s\n", username, password);
        }
        else {
            fprintf(file, "\n%s %s\n", username, password);
        }
        printf("Utilizatorul a fost creat cu succes!\n");
    }

    fclose(file);
    
    return 0;
}