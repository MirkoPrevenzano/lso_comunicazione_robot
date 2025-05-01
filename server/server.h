#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <cjson/cJSON.h> //installata libreria esterna
#include "./game.h"
#include "./handler.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_GIOCATORI 8
#define MAX_GAME 8

//ogni gioco deve sempre avere un proprietario in qualsiasi momento

typedef struct {
    int * socket;
    int id;
    char nome[30]; //nome di massimo 30 char
}GIOCATORE;

typedef struct {
    int id;
    GIOCATORE* Giocatori[2];
    GIOCATORE* GiocatoreProprietario;
    int TRIS[3][3];
    int esito;
    int turno;
    sem_t semaforo;
}GAME;

int static numero_connessioni=0;
int static numero_partite=0;
char *msg = "numero max di giocatori raggiunto.";
char *msg1 = "errore";
char *msg2 = "disconnesione";
GIOCATORE* Giocatori[MAX_GIOCATORI];
GAME* Partite[MAX_GAME];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock3 = PTHREAD_MUTEX_INITIALIZER;

void aggiorna_numero_connessioni(int * socket_nuovo);

void queue_add(GIOCATORE*giocatore_add);

void queue_remove(int id);

void *handle_client(void *arg);

#endif
