#ifndef REQUEST_H
#define REQUEST_H


#include <stdbool.h>
#include "player.h"


enum StatoRichiesta {
    RICHIESTA_IN_ATTESA,
    RICHIESTA_ACCETTATA,
    RICHIESTA_RIFIUTATA
};
typedef enum StatoRichiesta StatoRichiesta;

typedef struct {
    GIOCATORE* giocatore;
    StatoRichiesta stato;
} RICHIESTA;

RICHIESTA* crea_richiesta(GIOCATORE* giocatore);
void gestisci_richiesta(RICHIESTA* richiesta);
void accetta_richiesta(RICHIESTA* richiesta,int id_partita,GIOCATORE*giocatore1,GIOCATORE*giocatore2);
void rifiuta_richiesta(RICHIESTA* richiesta,int id_partita,GIOCATORE*giocatore1,GIOCATORE* giocatore2);
void elimina_richiesta(RICHIESTA* richiesta);
void remove_request_by_player(GIOCATORE* giocatore);
RICHIESTA * searchRichiesta(int id_partita, GIOCATORE* giocatore);
//void remove_request_by_GAME(GAME *partita);
void remove_request_by_GAME(int id_partita);
void *handle_game(void *);



#endif // REQUEST_H