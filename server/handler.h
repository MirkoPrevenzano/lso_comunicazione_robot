#ifndef HANDLER_H
#define HANDLER_H 


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

#include <stdbool.h>
#include "player.h"
#include "game.h"
#include "server.h"
void crea_json(cJSON *root,int id,char*nome);

bool partita_in_corso(GAME *partita);

void handlerInviaGames(int *socket_nuovo);

void joinLobby(int*leave_flag,char*buffer,GIOCATORE*nuovo_giocatore);

void checkGame(char* buffer, GIOCATORE*nuovo_giocatore, int *socket_nuovo, int *leave_flag);

GAME* searchPartitaById(int id);

//void handlerGames(int *leave_flag,int*socket_nuovo,char*buffer,GIOCATORE *giocatore);

//void partecipa_partita_json(int *leave_flag,int*socket_nuovo,char*buffer,cJSON*body,GIOCATORE *giocatore);

//void handlerRiceviJsonMossa(int *leave_flag,char*buffer,GIOCATORE *giocatore,GAME*partita);


#endif