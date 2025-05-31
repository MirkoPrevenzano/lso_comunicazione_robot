#ifndef GAME_H
#define GAME_H


#include <stdbool.h>
#include "player.h"
#include <sys/select.h>
#include <errno.h>
#include <semaphore.h>

typedef struct game{
    int id;
    GIOCATORE* giocatoreParticipante[2]; //giocatori[0] è il proprietario
    int TRIS[3][3];
    int esito;
    int turno;
    sem_t semaforo;
} GAME;

void new_game(int*leave_flag,char*buffer,GIOCATORE*giocatore);

//void partecipa_game(int*leave_flag,int id_lobby,char*buffer,GIOCATORE*giocatore);

void GameStartPlayer1(int*leave_flag,char*buffer,GAME*nuova_partita);
void GamePlayer1(int *leave_flag,char*buffer,GAME*nuova_partita,GIOCATORE*Giocatore1);
void GamePlayer2(int *leave_flag,char*buffer,GAME*nuova_partita,GIOCATORE*Giocatore2);

int recv_with_timeout(int sockfd, void* buf, size_t len, int timeout_sec);
void aggiungi_game_queue(GAME *nuova_partita,GIOCATORE* giocatoreProprietario);
void remove_game_by_player_id(int id);
void sendSuccessNewGame(int success, GIOCATORE*giocatore, int id_partita);

//GAME *searchPartitaById(int giocatore_id); //farei file player.h

//bool controlla_vittoria(GAME*partita,int giocatore,int*esito);
//bool controlla_pareggio(GAME*partita,int esito);

//void inviaJsonMatrice(GAME*partita,GIOCATORE*giocatore);
//void RiceviJsonMossa(int*leave_game,char*buffer,GAME*partita,GIOCATORE*giocatore);
//void ModificaArrayTris(int i,int j,int giocatore,GAME*partita);
//void inviaEsitoPartita(int*esito,GAME*partita);

#endif