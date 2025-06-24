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

void remove_request_by_GAME(int id_partita) {
    GAME*partita= Partite[id_partita];
    if(partita == NULL) {
    
        return; // Non c'è nulla da rimuovere
    }
    // Rimuove le richiesta associate alla partita
    for (int j = 0; j < MAX_GIOCATORI; ++j) {
        if (partita->richieste[j]) {
                    free(partita->richieste[j]); 
                    partita->richieste[j] = NULL;
                    partita->numero_richieste--;
                }
        }
    

 
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

void accetta_richiesta(RICHIESTA* richiesta,int id_partita,GIOCATORE*giocatore1,GIOCATORE*giocatore2){


    GAME*partita= Partite[id_partita];
    if(richiesta==NULL){
        send_success_message(0, giocatore1->socket, "errore");
        send_success_message(0, giocatore2->socket, "errore");
        return;
    }else{
        send_success_message(1, giocatore2->socket, "la tua richiesta è stata accettata");
        send_success_message(1, giocatore1->socket, "stai entrando in game");
        //qui da segmentation fault
        
        pthread_mutex_lock(&gameListLock);
        remove_request_by_player(giocatore1);
        remove_request_by_player(giocatore2);
        //eliminare tutte le richieste
        remove_request_by_GAME(id_partita);

        pthread_mutex_unlock(&gameListLock);

        //passare secondo giocatore alla partita
    }
    pthread_t tid1, tid2;

    pthread_create(&tid1, NULL, GamePlayer1, partita);
    pthread_create(&tid2, NULL, GamePlayer2, partita);

    
}

void rifiuta_richiesta(RICHIESTA* richiesta,int id_partita,GIOCATORE*giocatore1 ,GIOCATORE* giocatore2){
    pthread_mutex_lock(&lock);
    pthread_mutex_lock(&gameListLock);
    
    GAME* partita = Partite[id_partita];
    
    if(richiesta == NULL){
        printf("Nessuna richiesta o gioco trovati");
        send_success_message(0, giocatore1->socket, "errore, richiesta o partita non trovati");
        return;
    }else{
        send_success_message(0, giocatore2->socket, "richiesta rifiutata");
        elimina_richiesta(richiesta);
        partita->numero_richieste--;
        printf("Richiesta rimossa per il giocatore %s dalla partita %d\n", giocatore2->nome, id_partita);
        send_success_message(1, giocatore1->socket, "Richiesta rimossa con successo");
    }

    pthread_mutex_unlock(&gameListLock);
    pthread_mutex_unlock(&lock);
}

RICHIESTA * searchRichiesta(int id_partita, GIOCATORE* giocatore){
    pthread_mutex_lock(&lock);
    pthread_mutex_lock(&gameListLock);
    
    // Controllo se id_partita è valido
    if(id_partita < 0 || id_partita >= MAX_GAME || Partite[id_partita] == NULL) {
        printf("ID partita non valido: %d\n", id_partita);
        pthread_mutex_unlock(&gameListLock);
        pthread_mutex_unlock(&lock);
        return NULL;
    }

    GAME* partita = Partite[id_partita];
    
    // Trova richiesta giocatore
    bool richiesta_trovata = false;
    for(int i = 0; i < MAX_GIOCATORI-1; i++) {
        if(partita->richieste[i] && partita->richieste[i]->giocatore == giocatore) {
            richiesta_trovata = true;
            pthread_mutex_unlock(&gameListLock);
            pthread_mutex_unlock(&lock);
            return partita->richieste[i];

        }}
    
    if (!richiesta_trovata) {
        printf("Nessuna richiesta trovata per il giocatore %s nella partita %d\n", giocatore->nome, id_partita);
    }
    

    pthread_mutex_unlock(&gameListLock);
    pthread_mutex_unlock(&lock);

    return NULL;


}