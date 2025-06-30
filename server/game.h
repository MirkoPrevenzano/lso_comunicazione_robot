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
    NESSUN_ESITO,
    VITTORIA,
    PAREGGIO,
    SCONFITTA
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
    int turno;//turno = 0 -> turno del giocatorePartecipante[0]
    sem_t semaforo;
} GAME;



void new_game(int*leave_flag,char*buffer,GIOCATORE*giocatore);
void send_success_message(int success, int socket, const char* message);
void send_declined_request_message(int , GIOCATORE* );
void rimuovi_game_queue(GAME*partita);

void aggiungi_richiesta(int, GIOCATORE*);
void rimuovi_richiesta(int , GIOCATORE*);
void invia_richiesta_proprietario(GAME* , GIOCATORE* );

cJSON* read_with_timeout(int sockfd, char* buffer, size_t len, int timeout_sec,int*leave_game);
void gestioneRichiestaJSONuscita(cJSON* json,int*leave_flag,int*leave_game, GIOCATORE *);
void aggiungi_game_queue(GAME *nuova_partita,GIOCATORE* giocatoreProprietario);
void remove_game_by_player_id(int id);
void sendSuccessNewGame(int success, GIOCATORE*giocatore, int id_partita);
void sendJoinGame(GIOCATORE*giocatore,GAME*partita);

bool aggiorna_partita(GAME*partita,GIOCATORE*giocatore,int col,int row);
bool aggiorna_griglia(GAME*partita,GIOCATORE*giocatore,int col,int row,int turno);
void switchTurn(GAME*partita);
Esito switchEsito(Esito esito);
int switchGiocatore(int giocatore);
Esito verifica_esito_partita(GAME*partita,int giocatore);
void InviaEsito(GAME* partita,Esito esito,GIOCATORE*giocatore);
bool controlla_pareggio(GAME*partita);
bool controlla_vittoria(GAME*partita,int giocatore);

void InviaVittoriaAltroGiocatore(GIOCATORE*giocatore);
GAME*SearchPartitaInCorsoByGiocatore(GIOCATORE*giocatore);
void gestisci_esito_vittoria(GIOCATORE*giocatore,Esito esito,GAME*partita);


#endif