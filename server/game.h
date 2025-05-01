#ifndef GAME_H
#define GAME_H

#include "./server.h"

void crea_game(int*leave_flag,char*buffer,GIOCATORE*giocatore);

void partecipa_game(int*leave_flag,int id_lobby,char*buffer,GIOCATORE*giocatore);

GAME*aggiungi_game_queue(GAME *nuova_partita,GIOCATORE* giocatoreProprietario);

GIOCATORE*SearchGiocatoreByID(int giocatore_id);

bool controlla_vittoria(GAME*partita,int giocatore,int*esito);
bool controlla_pareggio(GAME*partita,int esito);

void StartGame(int turno,char*buffer,GAME*partita,GIOCATORE*giocatore);

void switchTurno(GAME*partita);

void inviaJsonMatrice(GAME*partita,GIOCATORE*giocatore);
void RiceviJsonMossa(int*leave_game,char*buffer,GAME*partita,GIOCATORE*giocatore);
void ModificaArrayTris(int i,int j,int giocatore,GAME*partita);
void inviaEsitoPartita(int*esito,GAME*partita);

#endif