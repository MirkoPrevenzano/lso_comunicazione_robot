#include "./handler.h"
#include "./server.h"

// Dichiarazioni esterne delle variabili globali già in server.h

void creaJSON(cJSON *root,int id,char*nome){
    cJSON_AddNumberToObject(root,"id_partita",id);
    cJSON_AddStringToObject(root,"proprietario",nome);
}

bool partitaInCorso(GIOCO*partita){
    if(partita)
        return (partita->stato_partita == IN_CORSO);
    else
        return false;
}

void handlerInviaPartite(int  socket_nuovo){
    cJSON*root = cJSON_CreateObject();
    cJSON *partite_array = cJSON_CreateArray();
    cJSON *partita = NULL; 
    for(int i=0;i<MAX_GAME;i++){
        if(Partite[i]!=NULL){
            if(!partitaInCorso(Partite[i]) && Partite[i]->giocatoreParticipante[0]->socket != socket_nuovo){
                partita = cJSON_CreateObject();
                creaJSON(partita,(int)Partite[i]->id,Partite[i]->giocatoreParticipante[0]->nome);
                cJSON_AddItemToArray(partite_array, partita);
            }
        }
    }

    cJSON_AddItemToObject(root, "partite", partite_array);
    char *json_str=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    printf("Invio lista partite in attesa al socket %d: %s\n", socket_nuovo, json_str);
    fflush(stdout);
    send(socket_nuovo,json_str,strlen(json_str),0);
    free(json_str); // libera la memoria allocata da cJSON_PrintUnformatted

}

//inviare con un send un messaggio di errore nei vari casi* TO-DO


GIOCO * cercaPartitaById(int id){
    for(int i=0;i<MAX_GAME;i++)
        if(Partite[i] && Partite[i]->id==id){
            return Partite[i];
        }
    return NULL;

}

void HandlerInviaMossePartita(GIOCATORE*giocatore,GIOCO*partita){
    
    //{type: "game_response", game_id: partita->id, game_data: {TRIS:partita->griglia, turno: partita->turno}}
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "/game_response");
    cJSON_AddNumberToObject(root, "game_id", partita->id);
    cJSON *game_data = cJSON_CreateObject();
    char griglia_str1[10]; // 9 positions + null terminator
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            griglia_str1[i*3 + j] = (char)(partita->griglia[i][j]+'0'); // Converti int a char
        }
    }
    griglia_str1[9] = '\0';
    
    cJSON_AddItemToObject(game_data, "TRIS", cJSON_CreateString(griglia_str1));        
    cJSON_AddNumberToObject(game_data, "turno", partita->turno);
    cJSON_AddItemToObject(root, "game_data", game_data);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root); // Libera la memoria allocata per il JSON
    printf("Invio griglia partita al socket %d: %s\n", giocatore->socket, json_str);
    fflush(stdout);
    send(giocatore->socket,json_str,strlen(json_str),0);
    free(json_str); // libera la memoria allocata da cJSON_PrintUnformatted

}

void handlerRispostaGioco(GIOCATORE*giocatore,GIOCO*partita){
    printf("Richiesta della griglia di gioco ricevuta\n");
    HandlerInviaMossePartita(giocatore,partita);
}

GIOCATORE* switchGiocatorePartita(GIOCATORE*giocatore,GIOCO*partita){

    if(giocatore==partita->giocatoreParticipante[0])
      return partita->giocatoreParticipante[1];
    else
       return partita->giocatoreParticipante[0];
}



void ServerAspettaPartita(char* buffer, GIOCATORE*nuovo_giocatore, int socket_nuovo){
    pthread_mutex_lock(&gameListLock);
    printf("Richiesta di giochi in attesa ricevuta\n");
    handlerInviaPartite(socket_nuovo);
    pthread_mutex_unlock(&gameListLock);
}

void ServerNuovaPartita(char* buffer, GIOCATORE*nuovo_giocatore,int *leave_flag){
    pthread_mutex_lock(&gameListLock);
    printf("Richiesta di creazione partita ricevuta\n");
    nuovaPartita(leave_flag,buffer,nuovo_giocatore);
    pthread_mutex_unlock(&gameListLock);
}

void ServerAggiungiRichiesta(char* buffer, GIOCATORE*nuovo_giocatore,int*leave_flag,cJSON *body){
    printf("Richiesta di aggiunta a una partita ricevuta\n");
    printf("Body ricevuto: %s\n", body->valuestring);
    fflush(stdout);
                
    // Parsa il JSON body
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if(!body_json) {
        printf("Errore nel parsing del JSON body\n");
        *leave_flag = 1;
    } else {
        cJSON *id_item = cJSON_GetObjectItem(body_json, "id");
        if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            printf("ID partita: %d\n", id_partita);
            // Aggiungi la richiesta alla partita
            aggiungiRichiesta(id_partita, nuovo_giocatore);
        } else {
            printf("Il campo 'id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
        }
        cJSON_Delete(body_json);
    }
 }


void ServerRimuoviRichiesta(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
        printf("Richiesta di rimozione da una partita ricevuta\n");
        printf("Body ricevuto: %s\n", body->valuestring);
        fflush(stdout);
        
        // Parsa il JSON body
        cJSON *body_json = cJSON_Parse(body->valuestring);
        if (!body_json) {
            printf("Errore nel parsing del JSON body\n");
            inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
        } else {
            cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
            if (id_item && cJSON_IsNumber(id_item)) {
                int id_partita = id_item->valueint;
                printf("ID partita: %d\n", id_partita);
                // Rimuovi la richiesta dalla partita
                rimuoviRichiesta(id_partita, nuovo_giocatore); 
            } else {
                printf("Il campo 'game_id' non è presente o non è un numero.\n");
                inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
            }
            cJSON_Delete(body_json);
        }
 }


void ServerAccettaRichiesta(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
    printf("Richiesta di accetazione una partita ricevuta\n");
    printf("Body ricevuto: %s\n", body->valuestring);
    fflush(stdout);
    
    // Parsa il JSON body
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
    } else {
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
        cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "player_id");
        //bisogan ora inviare al player id il messaggio ed eseguire joinlobby
            if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            printf("ID partita: %d\n", id_partita);
            if (id_item_2 && cJSON_IsNumber(id_item_2)) {
                int id_player = id_item_2->valueint;
                printf("ID Giocatore della richiesta: %d\n", id_player);
                GIOCATORE * nuovo_giocatore_2 = CercaGiocatoreById(id_player);
                pthread_mutex_lock(&gameListLock);
                accettaRichiesta(cercaRichiesta(id_partita, CercaGiocatoreById(id_player)),id_partita,nuovo_giocatore, nuovo_giocatore_2);
                pthread_mutex_unlock(&gameListLock);
            }else{
            aggiungiRichiesta(id_partita, nuovo_giocatore);
            printf("Il campo 'player_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
        }}else {
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
        }
        cJSON_Delete(body_json);
    }
}


void ServerRifiutaRichiesta(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
    printf("rifiuto di una richiesta ricevuta\n");
    printf("Body ricevuto: %s\n", body->valuestring);
    fflush(stdout);
    
    // Parsa il JSON body
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
    } else {
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
        cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "player_id");
        if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            printf("ID partita: %d\n", id_partita);
            if (id_item_2 && cJSON_IsNumber(id_item_2)) {
                int id_player = id_item_2->valueint;
                printf("ID Giocatore della richiesta: %d\n", id_player);
                GIOCATORE * nuovo_giocatore_2 = CercaGiocatoreById(id_player);
                pthread_mutex_lock(&gameListLock);
                rifiutaRichiesta(cercaRichiesta(id_partita, nuovo_giocatore_2),id_partita,nuovo_giocatore); 
                pthread_mutex_unlock(&gameListLock);
            }else{
                printf("Il campo 'player_id' non è presente o non è un numero.\n");
                inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
            }
        } else {
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
        }
        cJSON_Delete(body_json);
    }

}

void ServerVittoria(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
    } else {
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
        cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "risposta");
        if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            if(id_item_2 && cJSON_IsBool(id_item_2)) {
                bool risposta = id_item_2->valueint;
                pthread_mutex_lock(&gameListLock);
                GIOCO* partita = cercaPartitaById(id_partita);
                pthread_mutex_unlock(&gameListLock);
                if(risposta){
                    resetGioco(partita);
                    printf("il giocatore vincitore (ID : %d) ha deciso di diventare proprietario del gioco (ID : %d) ",nuovo_giocatore->id,id_partita);
                    inviaMessaggioSuccesso(1,nuovo_giocatore->socket,"sei diventato proprietario");
                }else{
                    inviaMessaggioSuccesso(1,nuovo_giocatore->socket,"partita cancellata");
                    printf("il giocatore vincitore (ID : %d) ha deciso di non diventare proprietario del Il tuo abbandono darà sconfitta a tavolinogioco (ID : %d) ",nuovo_giocatore->id,id_partita);
                    pthread_mutex_lock(&gameListLock);
                    rimuoviGiocoQueue(partita);
                    pthread_mutex_unlock(&gameListLock);
                }
            }else{
                printf("Il campo 'risposta' non è presente o non è booleano.\n");
                inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
            }
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(body_json);

}


void ServerPareggio(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
     cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
    } else {
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
        cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "risposta");
        if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            if(id_item_2 && cJSON_IsBool(id_item_2)) {
                bool risposta = id_item_2->valueint;
                pthread_mutex_lock(&gameListLock);
                GIOCO* partita = cercaPartitaById(id_partita);
                pthread_mutex_unlock(&gameListLock);
                if(partita){
                    inviaMessaggioSuccesso(1, nuovo_giocatore->socket, "Richiesta di rivincita ricevuta");
                    sleep(1);
                    GestionePareggioGame(partita,nuovo_giocatore,risposta);
                } else {
                    inviaMessaggioSuccesso(1, nuovo_giocatore->socket, "Partita eliminata");
                }
            }else{
                printf("Il campo 'risposta' non è presente o non è booleano.\n");
                inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
            }
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(body_json);
}


void ServerMossaGioco(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
     cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
    } 
    else {
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id"); 
        cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "col");
        cJSON *id_item_3 = cJSON_GetObjectItem(body_json, "row");  
        if (id_item && cJSON_IsNumber(id_item)) {
                int id_partita = id_item->valueint;
                printf("ID partita: %d\n", id_partita);
                if (id_item_2 && cJSON_IsNumber(id_item_2)) {
                    if (id_item_3 && cJSON_IsNumber(id_item_3)) {
                        int col = id_item_2->valueint;
                        int row = id_item_3->valueint;
                        pthread_mutex_lock(&gameListLock);
                        GIOCO*partita=cercaPartitaById(id_partita);
                        bool success = aggiornaPartita(partita,nuovo_giocatore,col,row);
                        if(success){
                            handlerRispostaGioco(nuovo_giocatore,partita);
                            handlerRispostaGioco(switchGiocatorePartita(nuovo_giocatore,partita),partita);

                        }
                        pthread_mutex_unlock(&gameListLock);

                    }else{
                        printf("Il campo 'row' non è presente o non è un numero.\n");
                        inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
                    }
                }else{
                    printf("Il campo 'col' non è presente o non è un numero.\n");
                    inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
                }
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(body_json);
}



void ServerUscitaGioco(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){

    //ricevere intenzione di uscire dalla partita
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
    } else{
    cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id"); 
        if (id_item && cJSON_IsNumber(id_item)) {
                int id_partita = id_item->valueint;
                printf("ID partita: %d\n", id_partita);
                pthread_mutex_lock(&gameListLock);
                GIOCO*partita = cercaPartitaById(id_partita);
                pthread_mutex_unlock(&gameListLock);
                InviaEsito(partita,SCONFITTA,nuovo_giocatore);
                if(nuovo_giocatore==partita->giocatoreParticipante[0])
                    InviaEsito(partita,VITTORIA,partita->giocatoreParticipante[1]);
                else
                    InviaEsito(partita,VITTORIA,partita->giocatoreParticipante[0]);
    }else{
        printf("Il campo 'game_id' non è presente o non è un numero.\n");
        inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
    }}
    cJSON_Delete(body_json);
}


void ServerUscitaPareggio(char* buffer, GIOCATORE* nuovo_giocatore, cJSON *body){
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
    } else{
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id"); 
        if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            printf("ID partita: %d\n", id_partita);
            pthread_mutex_lock(&gameListLock);
            GIOCO*partita = cercaPartitaById(id_partita);
            pthread_mutex_unlock(&gameListLock);
            GestionePareggioGame(partita,nuovo_giocatore,0);
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovo_giocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(body_json);
}