#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <cjson/cJSON.h> //installata libreria esterna
#include "player.h"
#include "game.h"
#include "request.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_GIOCATORI 8
#define MAX_GAME 20

//ogni gioco deve sempre avere un proprietario in qualsiasi momento





// Dichiarazioni
extern int numero_connessioni; // Numero di connessioni attive
extern int numero_partite; // Numero di partite attive


extern GAME* Partite[MAX_GAME]; // Array di puntatori a partite
extern GIOCATORE* Giocatori[MAX_GIOCATORI]; // Array di puntatori a giocatori

extern pthread_mutex_t lock; // Mutex per proteggere l'accesso alle risorse condivise
extern pthread_mutex_t playerListLock; // Mutex per proteggere l'accesso alla lista dei giocatori
extern pthread_mutex_t gameListLock; // Mutex per proteggere l'accesso alla lista delle partite

//opterei di fare un mutex per ogni grado di informazioni: esempio mutex_partita che blocca accessi simultanei al momento di aggiungere o rimuovere una partita
//forse un mutex per ogni partita
//un mutex per l'accesso alla lista dei giocatori 
//uso di possibili liste linkate


bool queue_add(GIOCATORE*giocatore_add);

void queue_remove(int id);

void * checkRouterThread(void *);

void *handle_client(void *arg);

void checkRouter(char* buffer, GIOCATORE*nuovo_giocatore, int socket_nuovo, int *leave_flag);

void *handle_close(void *);

GIOCATORE * SearchPlayerByid(int id_player);

#endif
