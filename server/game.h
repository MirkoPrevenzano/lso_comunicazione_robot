#ifndef GAME_H
#define GAME_H


#include <stdbool.h>
#include "player.h"
#include "request.h"

#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
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

typedef struct gioco{
    int id;
    GIOCATORE* giocatoreParticipante[2]; //giocatori[0] è il proprietario
    TRIS griglia[3][3];
    Esito esito;
    RICHIESTA *richieste [MAX_GIOCATORI-1];
    STATO statoPartita;
    int numeroRichieste;
    int turno; //turno = 0 -> turno del giocatorePartecipante[0]
    sem_t semaforo;
    int votiPareggio;
}GIOCO;



void nuovaPartita(char *buffer,GIOCATORE *giocatore);
void inviaMessaggioSuccesso(int success, int socket, const char *message);
void inviaMessaggioRichiestaRifiutata(int , GIOCATORE* );
void rimuoviGiocoQueue(GIOCO*partita);

void aggiungiRichiesta(int, GIOCATORE*);
void rimuoviRichiesta(int , GIOCATORE*);
void inviaRichiestaProprietario(GIOCO* , GIOCATORE* );

void aggiungiGiocoQueue(GIOCO *nuova_partita,GIOCATORE *giocatoreProprietario);
void rimuoviGiocoByIdGiocatore(int id);
void inviaMessaggioSuccessoNuovaPartita(int success, GIOCATORE *giocatore, int idPartita);

bool aggiornaPartita(GIOCO *partita,GIOCATORE *giocatore,int col,int row);
bool aggiornaGriglia(GIOCO *partita,GIOCATORE *giocatore,int col,int row,int turno);
void switchTurn(GIOCO *partita);
Esito switchEsito(Esito esito);
int switchGiocatore(int giocatore);
Esito verificaEsitoPartita(GIOCO *partita,int giocatore);
void inviaEsito(GIOCO *partita,Esito esito,GIOCATORE *giocatore);
bool controllaPareggio(GIOCO *partita);
bool controllaVittoria(GIOCO *partita,int giocatore);

void inviaVittoriaAltroGiocatore(GIOCATORE *giocatore);
void inviaPareggioDisconnessione(GIOCATORE *giocatore);
GIOCO* cercaPartitaInCorsoByGiocatore(GIOCATORE *giocatore);
void gestisciEsitoVittoria(GIOCATORE *giocatore,Esito esito,GIOCO *partita);

void resetGioco(GIOCO *partita);
void gestionePareggioGioco(GIOCO *partita,GIOCATORE *giocatore,bool risposta);
void inviaMessaggioRivincita(GIOCATORE *giocatore,int risposta,GIOCO *partita);


#endif