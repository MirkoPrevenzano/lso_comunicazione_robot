#ifndef HANDLER_H
#define HANDLER_H 

#include "./server.h"

char*crea_Json(int id,char*type);

bool partita_in_corso(GAME*partita);

void handlerInviaGames(int * socket_nuovo,char*buffer);

void handlerGames(&leave_flag,int*socket_nuovo,char*buffer,GIOCATORE *giocatore);

void Crea_partita_json(int*leave_flag,int*socket_nuovo,char*buffer,cJSON *body,GIOCATORE *giocatore);

void partecipa_partita_json(int *leave_flag,int*socket_nuovo,char*buffer,cJSON*body,GIOCATORE *giocatore);

void handlerRiceviJsonMossa(int*leave_flag,char*buffer,GIOCATORE *giocatore,GAME*partita);


#endif