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

void handlerInviaMossePartita(GIOCATORE *giocatore,GIOCO *partita);
 

GIOCATORE* switchGiocatorePartita(GIOCATORE *giocatore,GIOCO *partita);

void serverUscitaGioco(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);

void serverMossaGioco(char *buffer, GIOCATORE* nuovoGiocatore,cJSON *body);

void serverPareggio(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);

void serverVittoria(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);

void serverRifiutaRichiesta(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);

void serverAccettaRichiesta(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);

void serverRimuoviRichiesta(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);

void serverAggiungiRichiesta(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);

void serverNuovaPartita(char *buffer, GIOCATORE *nuovoGiocatore);

void serverAspettaPartita(char *buffer, GIOCATORE *nuovoGiocatore, int socket_nuovo);

void serverUscitaPareggio(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);

void serverRichiesteInviate(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);

void serverRichiesteRicevute(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body);


#endif