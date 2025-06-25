#ifndef GAME_H
#define GAME_H


#include <stdbool.h>
#include "player.h"
#include "request.h"

#include <sys/select.h>
#include <errno.h>
#include <semaphore.h>
#include <cjson/cJSON.h> //installata libreria esterna
#define MAX_GIOCATORI 8

typedef enum {
    VITTORIA,
    PAREGGIO,
    SCONFITTA,
    NESSUN_ESITO
} Esito;

typedef enum {
    IN_CORSO,
    IN_ATTESA
} STATO;

typedef enum{
    VUOTO,
    X, 
    O 
}TRIS;

typedef struct game{
    int id;
    GIOCATORE* giocatoreParticipante[2]; //giocatori[0] è il proprietario
    TRIS griglia[3][3];
    Esito esito;
    RICHIESTA * richieste [MAX_GIOCATORI-1];
    STATO stato_partita;
    int numero_richieste;
    int turno;
    sem_t semaforo;
} GAME;



void new_game(int*leave_flag,char*buffer,GIOCATORE*giocatore);
void send_success_message(int success, int socket, const char* message);
void send_declined_request_message(int , GIOCATORE* );

void aggiungi_richiesta(int, GIOCATORE*);
void rimuovi_richiesta(int , GIOCATORE*);
void invia_richiesta_proprietario(GAME* , GIOCATORE* );

void GameStartPlayer1(int*leave_flag,char*buffer,GAME*nuova_partita);
void GameStartPlayer2(int*leave_flag,GAME*nuova_partita,GIOCATORE*Giocatore2);
//void GamePlayer1(int *leave_flag,int*leave_game,char*buffer,GAME*nuova_partita,GIOCATORE*Giocatore1);
void * GamePlayer1(void *);
//void GamePlayer2(int *leave_flag,int*leave_game,GAME*nuova_partita,GIOCATORE*Giocatore2);
void * GamePlayer2(void *);


cJSON* read_with_timeout(int sockfd, char* buffer, size_t len, int timeout_sec,int*leave_game);
void gestioneRichiestaJSONuscita(cJSON* json,int*leave_flag,int*leave_game, GIOCATORE *);
void aggiungi_game_queue(GAME *nuova_partita,GIOCATORE* giocatoreProprietario);
void remove_game_by_player_id(int id);
void sendSuccessNewGame(int success, GIOCATORE*giocatore, int id_partita);
void sendJoinGame(GIOCATORE*giocatore,GAME*partita);
//GAME *searchPartitaById(int giocatore_id); //farei file player.h

//bool controlla_vittoria(GAME*partita,int giocatore,int*esito);
//bool controlla_pareggio(GAME*partita,int esito);

//void inviaJsonMatrice(GAME*partita,GIOCATORE*giocatore);
//void RiceviJsonMossa(int*leave_game,char*buffer,GAME*partita,GIOCATORE*giocatore);
//void ModificaArrayTris(int i,int j,int giocatore,GAME*partita);
//void inviaEsitoPartita(int*esito,GAME*partita);

#endif