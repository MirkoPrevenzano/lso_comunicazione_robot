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

void crea_json(cJSON *root,int id,char*nome);

bool partita_in_corso(GAME *partita);

void handlerInviaGames(int socket_nuovo);

void joinLobby(int*leave_flag,cJSON*body,GIOCATORE*nuovo_giocatore);

GAME* searchPartitaById(int id);

void HandlerInviaMovesPartita(GIOCATORE*giocatore,GAME*partita);

void handler_game_response(GIOCATORE*giocatore,GAME*partita);

GIOCATORE* switchGiocatorePartita(GIOCATORE*giocatore,GAME*partita);

void ServerExitGame(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerGameMove(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerDrawGame(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerVictoryGame(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerDeclineRequest(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerAcceptRequest(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerRemoveRequest(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerAddRequest(char* buffer, GIOCATORE*nuovo_giocatore,int*leave_flag,cJSON *body);
void ServerNewGames(char* buffer, GIOCATORE*nuovo_giocatore,int *leave_flag);
void ServerWaitingGames(char* buffer, GIOCATORE*nuovo_giocatore, int socket_nuovo);


#endif