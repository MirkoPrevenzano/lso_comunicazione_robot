#ifndef GAME_H
#define GAME_H


#include <stdbool.h>
#include "player.h"
#include <semaphore.h>

typedef struct game{
    int id;
    GIOCATORE* giocatoreParticipante[2]; //giocatori[0] è il proprietario
    int TRIS[3][3];
    int esito;
    int turno;
    sem_t semaforo; //cos'è?
} GAME;

void new_game(int*leave_flag,char*buffer,GIOCATORE*giocatore);

//void partecipa_game(int*leave_flag,int id_lobby,char*buffer,GIOCATORE*giocatore);

void aggiungi_game_queue(GAME *nuova_partita,GIOCATORE* giocatoreProprietario);
void remove_game_by_player_id(int id);
void sendSuccessNewGame(int success, GIOCATORE*giocatore, int id_partita);

//GAME *searchPartitaById(int giocatore_id); //farei file player.h

//bool controlla_vittoria(GAME*partita,int giocatore,int*esito);
//bool controlla_pareggio(GAME*partita,int esito);

//void StartGame(int turno,char*buffer,GAME*partita,GIOCATORE*giocatore);

//void switchTurno(GAME*partita);

//void inviaJsonMatrice(GAME*partita,GIOCATORE*giocatore);
//void RiceviJsonMossa(int*leave_game,char*buffer,GAME*partita,GIOCATORE*giocatore);
//void ModificaArrayTris(int i,int j,int giocatore,GAME*partita);
//void inviaEsitoPartita(int*esito,GAME*partita);

#endif