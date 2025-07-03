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

RICHIESTA* creaRichiesta(GIOCATORE* giocatore);
void gestisciRichiesta(RICHIESTA* richiesta);
void accettaRichiesta(RICHIESTA* richiesta,int id_partita,GIOCATORE*giocatore1,GIOCATORE*giocatore2);
void rifiutaRichiesta(RICHIESTA* richiesta,int id_partita,GIOCATORE*giocatore1);
void eliminaRichiesta(RICHIESTA* richiesta);
void rimuoviRichiestabyGiocatore(GIOCATORE* giocatore);
RICHIESTA * cercaRichiesta(int id_partita, GIOCATORE* giocatore);
void RifiutaRichiestabyGioco(int id_partita);



#endif // REQUEST_H