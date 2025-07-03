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

void creaJSON(cJSON *root,int id,char*nome);

bool partitaInCorso(GIOCO *partita);

void handlerInviaPartite(int socket_nuovo);

GIOCO* cercaPartitaById(int id);

void HandlerInviaMossePartita(GIOCATORE*giocatore,GIOCO*partita);

void handlerRispostaGioco(GIOCATORE*giocatore,GIOCO*partita);

GIOCATORE* switchGiocatorePartita(GIOCATORE*giocatore,GIOCO*partita);

void ServerUscitaGioco(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerMossaGioco(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerPareggio(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerVittoria(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerRifiutaRichiesta(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerAccettaRichiesta(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerRimuoviRichiesta(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body);
void ServerAggiungiRichiesta(char* buffer, GIOCATORE*nuovo_giocatore,int*leave_flag,cJSON *body);
void ServerNuovaPartita(char* buffer, GIOCATORE*nuovo_giocatore,int *leave_flag);
void ServerAspettaPartita(char* buffer, GIOCATORE*nuovo_giocatore, int socket_nuovo);
void ServerUscitaPareggio(char* buffer, GIOCATORE* nuovo_giocatore,cJSON *body);

#endif