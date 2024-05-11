
#include <stdio.h> // Folosit pentru: scanf, printf, perror, fprintf, stderr, fgets, fseek, fopen, fclose
#include <stdlib.h> // Folosit pentru: exit, EXIT_FAILURE
#include <unistd.h> // Folosit pentru: fork, execl, STDIN_FILENO
#include <termios.h> // Folosit pentru: termios, tcgetattr, tcsetattr, ECHO, TCSANOW
#include <string.h> // Folosit pentru: strcmp, strcspn 
#include <errno.h> // Folosit pentru: errno, ENOENT
#include <sys/wait.h> // Folosit pentru: waitpid, WIFEXITED, WEXITSTATUS
#include <libpq-fe.h>
#include "../include/database.h"

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

// Functia pentru logarea unui utilizator cu parola sa
int login(PGconn *conn, char *username, char *password) {
    int returnStatus;
    login_user(conn, username, password, &returnStatus);
    return returnStatus;
}

bool ps_register(PGconn *conn, char *username, char *password) {
    return register_user(conn, username, password);
}

// Functie pentru crearea unui utilizator nou
int create_user(char *username, char* password) {
    // Deschidem fisierul de credentiale pentru salvarea noilor date
    FILE *file = fopen("/tmp/credentials.txt", "r+");

    // Daca fisierul de credentiale nu exista
    if (file == NULL) {
        // Verificam daca codul de eroare este cel pentru fisier inexistent
        if (errno == ENOENT) {
            // Cream fisierul prin modul write +
            file = fopen("/tmp/credentials.txt", "w+");
            // Daca totusi nu putem crea fisierul
            if (file == NULL) {
                // Afisam o eroare si incheiem executia
                fprintf(stderr, "Eroare: Nu s-a putut crea fisierul de credentiale");
                return 1;
            }
        }
        // Altfel, fisierul exista dar din anumite motive nu il putem deschide (probabil accesul la fisier e interzis) si incheiem execuita cu eroare
        else {
            fprintf(stderr, "Eroare: Nu s-a putut deschide fisierul de credentiale");
            return 1;
        }
    }

    // Definim variabile pentru fiecare credential (nume si parola) deja stocat in fisierul de credentiale
    char stored_username[MAX_USERNAME_LENGTH];
    char stored_password[MAX_PASSWORD_LENGTH];

    // Definim o variabila pentru verificarea daca un utilizator exista sau nu in fisier
    int user_exists = 0;

    // Citim linie cu linie din fisierul credentials.txt pana la finalul acestuia
    while (fscanf(file, "%s %s", stored_username, stored_password) != EOF) {
        // Daca gasim utilizatorul, tinem minte acest lucru
            if (strcmp(username, stored_username) == 0) {
                user_exists = 1;
                break;
        }
    }

    // Daca utilizatorul exista, afisam o eroare
    if (user_exists) {
        fprintf(stderr, "Eroare: Acest utilizator exista deja!\n");
        return 1;
    } else { // Altfel, il punem in fisierul de credentiale, fiind un utilizator nou
        int valFseek = fseek(file, 0, SEEK_END);
        if (valFseek == 0) {
            fprintf(file, "%s %s\n", username, password);
        }
        else {
            fprintf(file, "\n%s %s\n", username, password);
        }
        printf("Utilizatorul a fost creat cu succes!\n");
    }

    // Si in final, inchidem fisierul
    fclose(file);
    
    return 0;
}