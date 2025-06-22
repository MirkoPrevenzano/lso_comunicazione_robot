#include "request.h"
#include "server.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>

void remove_request_by_player(GIOCATORE *giocatore) {
   
    if(giocatore == NULL) {
    
        return; // Non c'è nulla da rimuovere
    }
    pthread_mutex_lock(&gameListLock);
    // Rimuove la richiesta associata al giocatore
    for (int j = 0; j < MAX_GAME; ++j) {
        if (Partite[j]) {
            for (int k = 0; k < MAX_GIOCATORI-1; ++k) {
                if (Partite[j]->richieste[k] && Partite[j]->richieste[k]->giocatore == giocatore) {
                    free(Partite[j]->richieste[k]); 
                    Partite[j]->richieste[k] = NULL;
                    Partite[j]->numero_richieste--;
                }
            }
        }
    }
    pthread_mutex_unlock(&gameListLock);

 
}

RICHIESTA * crea_richiesta(GIOCATORE *giocatore) {
    RICHIESTA *richiesta = (RICHIESTA *)malloc(sizeof(RICHIESTA));
    if (richiesta == NULL) {
        perror("Errore allocazione memoria per richiesta");
        return NULL;
    }
    richiesta->giocatore = giocatore;
    richiesta->stato = RICHIESTA_IN_ATTESA; // Stato iniziale della richiesta
    return richiesta;
}

void elimina_richiesta(RICHIESTA *richiesta) {
    if (richiesta != NULL) {
        free(richiesta);
    }
}