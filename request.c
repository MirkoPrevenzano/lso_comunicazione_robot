#include "request.h"
#include "server.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>

void remove_request_by_player(GIOCATORE *giocatore) {
   
    if(giocatore == NULL) {
        printf("Nessun giocatore specificato per la rimozione della richiesta.\n");
        return; // Non c'è nulla da rimuovere
    }
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
    printf("Rimozione della richiesta per il giocatore %s\n", giocatore->nome);

 
}

void decline_request_by_GAME(int id_partita) {
    GAME*partita= Partite[id_partita];
    if(partita == NULL) {
    
        return; // Non c'è nulla da rimuovere
    }
    // Rimuove le richiesta associate alla partita
    for (int j = 0; j < MAX_GIOCATORI-1; ++j) {
        if (partita->richieste[j] && partita->richieste[j]->stato == RICHIESTA_IN_ATTESA) { 
            rifiuta_richiesta(partita->richieste[j], id_partita, NULL);
        }
    }
    printf("Rifiuto di tutte le richieste in attesa per la partita %d\n", id_partita);
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
    }
    else if(giocatore2->stato != IN_HOME){
        send_success_message(0, giocatore1->socket, "errore, il giocatore è impegnato in un'altra partita");
        return;
    }
    else{
        
        printf("Accettazione della richiesta per la partita %d tra %s e %s\n", id_partita, giocatore1->nome, giocatore2->nome);
        remove_request_by_player(giocatore1);
        remove_request_by_player(giocatore2);
        richiesta->stato = RICHIESTA_ACCETTATA; // Aggiorna lo stato della richiesta
        giocatore1->stato = IN_GIOCO; // Imposta lo stato del giocatore come in gioco
        giocatore2->stato = IN_GIOCO; // Imposta lo stato del giocatore come in gioco
        partita->giocatoreParticipante[0] = giocatore1; // Assegna il primo giocatore
        partita->giocatoreParticipante[1] = giocatore2; // Assegna il secondo giocatore
        //eliminare tutte le richieste

        //rifiuto tutte le richieste in attesa per questa partita
        decline_request_by_GAME(id_partita);

       
        //inviare messaggio di successo e info partita ai giocatori

        //{success:1, game_id: id_partita, player_id: giocatore2->id, "simbolo": X, "nickname_partecipante": giocatore2->nome, "game_data": {TRIS:partita->griglia, turno: 0}}
        cJSON *success_message1 = cJSON_CreateObject();
        cJSON_AddNumberToObject(success_message1, "success", 1);
        cJSON_AddNumberToObject(success_message1, "game_id", id_partita);
        cJSON_AddNumberToObject(success_message1, "player_id", giocatore2->id);
        cJSON_AddStringToObject(success_message1, "simbolo", "X");
        cJSON_AddStringToObject(success_message1, "nickname_partecipante", giocatore2->nome);
        cJSON *game_data1 = cJSON_CreateObject();
        char griglia_str[10]; // 9 positions + null terminator
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                griglia_str[i*3 + j] = (char)(char)('0');
            }
        }
        griglia_str[9] = '\0';
        
        cJSON_AddItemToObject(game_data1, "TRIS", cJSON_CreateString(griglia_str));
        cJSON_AddNumberToObject(game_data1, "turno", partita->turno);
        cJSON_AddItemToObject(success_message1, "game_data", game_data1);
        char *message1 = cJSON_Print(success_message1);
        send(giocatore1->socket, message1, strlen(message1), 0);
        cJSON_Delete(success_message1);
        free(message1);
        

        //{path: "/join_game", game_id: id_partita, player_id: giocatore1->id, "simbolo": O, "nickname_partecipante": giocatore1->nome, "game_data": {TRIS:partita->griglia, turno:0}}
        cJSON *success_message2 = cJSON_CreateObject();
        cJSON_AddStringToObject(success_message2, "path", "/game_start");
        cJSON_AddNumberToObject(success_message2, "game_id", id_partita);
        cJSON_AddNumberToObject(success_message2, "player_id", giocatore1->id);
        cJSON_AddStringToObject(success_message2, "simbolo", "O");
        cJSON_AddStringToObject(success_message2, "nickname_partecipante", giocatore1->nome);
        cJSON *game_data2 = cJSON_CreateObject();
        char griglia_str1[10]; // 9 positions + null terminator
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                griglia_str1[i*3 + j] = (char)('0');
            }
        }
        griglia_str1[9] = '\0';
        
        cJSON_AddItemToObject(game_data2, "TRIS", cJSON_CreateString(griglia_str1));        
        cJSON_AddNumberToObject(game_data2, "turno", partita->turno);
        cJSON_AddItemToObject(success_message2, "game_data", game_data2);
        char *message2 = cJSON_Print(success_message2);
        send(giocatore2->socket, message2, strlen(message2), 0);
        cJSON_Delete(success_message2);
        free(message2); 
        printf("Partita %d iniziata tra %s e %s\n", id_partita, giocatore1->nome, giocatore2->nome);
        fflush(stdout);
        // Imposta lo stato della partita come IN_CORSO
        partita->stato_partita = IN_CORSO;


    }
    
    
    
}

/*
entrando in questa funzione, entrambi i giocatori si trovano nel client in partita, quindi si può iniziare a giocare
La partita inizia con lo stato IN_CORSO e continua fino a quando non viene raggiunto un esito finale (vittoria o pareggio).
Durante ogni turno, il gioco può essere gestito in modo interattivo, ad esempio ricevendo mosse dai giocatori e aggiornando lo stato della griglia.
Controllare che chi effettua la mossa sia il giocatore giusto, in base al turno corrente.
Ad ogni iterazione bisogna controllare se uno dei due giocatori non è più connesso bisogna dare la vittoria all'altro giocatore.
*/

void rifiuta_richiesta(RICHIESTA* richiesta,int id_partita,GIOCATORE*giocatore1){
    if(richiesta == NULL){
        printf("Nessuna richiesta o gioco trovati");
        if(giocatore1 != NULL)
            send_success_message(0, giocatore1->socket, "errore, richiesta o partita non trovati");
        return;
    }else{
        richiesta->stato = RICHIESTA_RIFIUTATA; // Aggiorna lo stato della richiesta
        send_declined_request_message(id_partita, richiesta->giocatore);
        printf("Richiesta rimossa per il giocatore %s dalla partita %d\n", richiesta->giocatore->nome, id_partita);
        if(giocatore1 != NULL) {
            send_success_message(1, giocatore1->socket, "Richiesta rifiutata con successo");
        }
    }

}

RICHIESTA * searchRichiesta(int id_partita, GIOCATORE* giocatore){
    
    
    // Controllo se id_partita è valido
    if(id_partita < 0 || id_partita >= MAX_GAME || Partite[id_partita] == NULL) {
        printf("ID partita non valido: %d\n", id_partita);
        return NULL;
    }

    GAME* partita = Partite[id_partita];
    
    // Trova richiesta giocatore
    bool richiesta_trovata = false;
    for(int i = 0; i < MAX_GIOCATORI-1; i++) {
        if(partita->richieste[i] && partita->richieste[i]->giocatore == giocatore) {
            richiesta_trovata = true;
            
            return partita->richieste[i];

        }}
    
    if (!richiesta_trovata) {
        printf("Nessuna richiesta trovata per il giocatore %s nella partita %d\n", giocatore->nome, id_partita);
    }
    

  

    return NULL;


}