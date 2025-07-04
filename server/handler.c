#include "./handler.h"
#include "./server.h"

// Dichiarazioni esterne delle variabili globali già in server.h

void creaJSON(cJSON *root,int id,char *nome){
    cJSON_AddNumberToObject(root,"id_partita",id);
    cJSON_AddStringToObject(root,"proprietario",nome);
}

bool partitaInCorso(GIOCO*partita){
    if(partita)
        return (partita->statoPartita == IN_CORSO);
    else
        return false;
}

void handlerInviaPartite(int  socketNuovo){
    cJSON*root = cJSON_CreateObject();
    cJSON *partiteArray = cJSON_CreateArray();
    cJSON *partita = NULL; 
    for(int i=0;i<MAX_GAME;i++){
        if(Partite[i]!=NULL){
            if(!partitaInCorso(Partite[i]) && Partite[i]->giocatoreParticipante[0]->socket != socketNuovo){
                partita = cJSON_CreateObject();
                creaJSON(partita,(int)Partite[i]->id,Partite[i]->giocatoreParticipante[0]->nome);
                cJSON_AddItemToArray(partiteArray, partita);
            }
        }
    }

    cJSON_AddItemToObject(root, "partite", partiteArray);
    char *jsonStr=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    printf("Invio lista partite in attesa al socket %d: %s\n", socketNuovo, jsonStr);
    fflush(stdout);
    send(socketNuovo,jsonStr,strlen(jsonStr),0);
    free(jsonStr); // libera la memoria allocata da cJSON_PrintUnformatted

}



GIOCO * cercaPartitaById(int id){
    for(int i=0;i<MAX_GAME;i++)
        if(Partite[i] && Partite[i]->id==id){
            return Partite[i];
        }
    return NULL;
}

void handlerInviaMossePartita(GIOCATORE *giocatore,GIOCO *partita){
    
    //{type: "game_response", game_id: partita->id, gameData: {TRIS:partita->griglia, turno: partita->turno}}
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "/game_response");
    cJSON_AddNumberToObject(root, "game_id", partita->id);
    cJSON *gameData = cJSON_CreateObject();
    char grigliaStr1[10]; // 9 positions + null terminator
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            grigliaStr1[i*3 + j] = (char)(partita->griglia[i][j]+'0'); // Converti int a char
        }
    }
    grigliaStr1[9] = '\0';
    
    cJSON_AddItemToObject(gameData, "TRIS", cJSON_CreateString(grigliaStr1));        
    cJSON_AddNumberToObject(gameData, "turno", partita->turno);
    cJSON_AddItemToObject(root, "game_data", gameData);
    char *jsonStr = cJSON_PrintUnformatted(root);
    cJSON_Delete(root); // Libera la memoria allocata per il JSON
    printf("Invio griglia partita al socket %d: %s\n", giocatore->socket, jsonStr);
    fflush(stdout);
    send(giocatore->socket,jsonStr,strlen(jsonStr),0);
    free(jsonStr); // libera la memoria allocata da cJSON_PrintUnformatted

}


GIOCATORE* switchGiocatorePartita(GIOCATORE *giocatore,GIOCO *partita){
    if(giocatore==partita->giocatoreParticipante[0])
      return partita->giocatoreParticipante[1];
    else
       return partita->giocatoreParticipante[0];
}

void serverAspettaPartita(char *buffer, GIOCATORE *nuovoGiocatore, int socketNuovo){
    pthread_mutex_lock(&gameListLock);
    printf("Richiesta di giochi in attesa ricevuta\n");
    handlerInviaPartite(socketNuovo);
    pthread_mutex_unlock(&gameListLock);
}

void serverNuovaPartita(char *buffer, GIOCATORE *nuovoGiocatore){
    pthread_mutex_lock(&gameListLock);
    printf("Richiesta di creazione partita ricevuta\n");
    nuovaPartita(buffer,nuovoGiocatore);
    pthread_mutex_unlock(&gameListLock);
}

void serverAggiungiRichiesta(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body){
    printf("Richiesta di aggiunta a una partita ricevuta\n");
    printf("Body ricevuto: %s\n", body->valuestring);
    fflush(stdout);
                
    // Parsa il JSON body
    cJSON *bodyJson = cJSON_Parse(body->valuestring);
    if(!bodyJson) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi"); 
    } else {
        cJSON *idItem = cJSON_GetObjectItem(bodyJson, "id");
        if (idItem && cJSON_IsNumber(idItem)) {
            int idPartita = idItem->valueint;
            printf("ID partita: %d\n", idPartita);
            // Aggiungi la richiesta alla partita
            aggiungiRichiesta(idPartita, nuovoGiocatore);
        } else {
            printf("Il campo 'id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
        }
        cJSON_Delete(bodyJson);
    }
 }


void serverRimuoviRichiesta(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body){
        printf("Richiesta di rimozione da una partita ricevuta\n");
        printf("Body ricevuto: %s\n", body->valuestring);
        fflush(stdout);
        
        // Parsa il JSON body
        cJSON *bodyJson = cJSON_Parse(body->valuestring);
        if (!bodyJson) {
            printf("Errore nel parsing del JSON body\n");
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
        } else {
            cJSON *idItem = cJSON_GetObjectItem(bodyJson, "game_id");
            if (idItem && cJSON_IsNumber(idItem)) {
                int idPartita = idItem->valueint;
                printf("ID partita: %d\n", idPartita);
                // Rimuovi la richiesta dalla partita
                rimuoviRichiesta(idPartita, nuovoGiocatore); 
            } else {
                printf("Il campo 'game_id' non è presente o non è un numero.\n");
                inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
            }
            cJSON_Delete(bodyJson);
        }
 }


void serverAccettaRichiesta(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body){
    printf("Richiesta di accetazione una partita ricevuta\n");
    printf("Body ricevuto: %s\n", body->valuestring);
    fflush(stdout);
    
    // Parsa il JSON body
    cJSON *bodyJson = cJSON_Parse(body->valuestring);
    if (!bodyJson) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
    } else {
        cJSON *idItem = cJSON_GetObjectItem(bodyJson, "game_id");
        cJSON *idItem2 = cJSON_GetObjectItem(bodyJson, "player_id");
        //bisogan ora inviare al player id il messaggio ed eseguire joinlobby
            if (idItem && cJSON_IsNumber(idItem)) {
            int idPartita = idItem->valueint;
            printf("ID partita: %d\n", idPartita);
            if (idItem2 && cJSON_IsNumber(idItem2)) {
                int idPlayer = idItem2->valueint;
                printf("ID Giocatore della richiesta: %d\n", idPlayer);
                GIOCATORE * nuovoGiocatore2 = cercaGiocatoreById(idPlayer);
                
                pthread_mutex_lock(&gameListLock);
                accettaRichiesta(cercaRichiesta(idPartita, nuovoGiocatore2),idPartita,nuovoGiocatore, nuovoGiocatore2);
                pthread_mutex_unlock(&gameListLock);
            }else{
            aggiungiRichiesta(idPartita, nuovoGiocatore);
            printf("Il campo 'player_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
        }}else {
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
        }
        cJSON_Delete(bodyJson);
    }
}


void serverRifiutaRichiesta(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body){
    printf("rifiuto di una richiesta ricevuta\n");
    printf("Body ricevuto: %s\n", body->valuestring);
    fflush(stdout);
    
    // Parsa il JSON body
    cJSON *bodyJson = cJSON_Parse(body->valuestring);
    if (!bodyJson) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
    } else {
        cJSON *idItem = cJSON_GetObjectItem(bodyJson, "game_id");
        cJSON *idItem2 = cJSON_GetObjectItem(bodyJson, "player_id");
        if (idItem && cJSON_IsNumber(idItem)) {
            int idPartita = idItem->valueint;
            printf("ID partita: %d\n", idPartita);
            if (idItem2 && cJSON_IsNumber(idItem2)) {
                int idPlayer = idItem2->valueint;
                printf("ID Giocatore della richiesta: %d\n", idPlayer);
                GIOCATORE *nuovoGiocatore2 = cercaGiocatoreById(idPlayer);
                pthread_mutex_lock(&gameListLock);
                rifiutaRichiesta(cercaRichiesta(idPartita, nuovoGiocatore2),idPartita,nuovoGiocatore); 
                pthread_mutex_unlock(&gameListLock);
            }else{
                printf("Il campo 'player_id' non è presente o non è un numero.\n");
                inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
            }
        } else {
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
        }
        cJSON_Delete(bodyJson);
    }

}

void serverVittoria(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body){
    cJSON *bodyJson = cJSON_Parse(body->valuestring);
    if (!bodyJson) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
    } else {
        cJSON *idItem = cJSON_GetObjectItem(bodyJson, "game_id");
        cJSON *idItem2 = cJSON_GetObjectItem(bodyJson, "risposta");
        if (idItem && cJSON_IsNumber(idItem)) {
            int idPartita = idItem->valueint;
            if(idItem2 && cJSON_IsBool(idItem2)) {
                bool risposta = idItem2->valueint;
                pthread_mutex_lock(&gameListLock);
                GIOCO *partita = cercaPartitaById(idPartita);
                pthread_mutex_unlock(&gameListLock);
                if(partita == NULL) {
                    inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Partita non trovata");
                    cJSON_Delete(bodyJson);
                    return;
                }
                if(risposta){
                    resetGioco(partita);
                    printf("il giocatore vincitore (ID : %d) ha deciso di diventare proprietario del gioco (ID : %d) ",nuovoGiocatore->id,idPartita);
                    inviaMessaggioSuccesso(1,nuovoGiocatore->socket,"sei diventato proprietario");
                }else{
                    inviaMessaggioSuccesso(1,nuovoGiocatore->socket,"partita cancellata");
                    printf("il giocatore vincitore (ID : %d) ha deciso di non diventare proprietario del Il tuo abbandono darà sconfitta a tavolinogioco (ID : %d) ",nuovoGiocatore->id,idPartita);
                    pthread_mutex_lock(&gameListLock);
                    rimuoviGiocoQueue(partita);
                    pthread_mutex_unlock(&gameListLock);
                }
            }else{
                printf("Il campo 'risposta' non è presente o non è booleano.\n");
                inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
            }
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(bodyJson);

}


void serverPareggio(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body){
     cJSON *bodyJson = cJSON_Parse(body->valuestring);
    if (!bodyJson) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
    } else {
        cJSON *idItem = cJSON_GetObjectItem(bodyJson, "game_id");
        cJSON *idItem2 = cJSON_GetObjectItem(bodyJson, "risposta");
        if (idItem && cJSON_IsNumber(idItem)) {
            int idPartita = idItem->valueint;
            if(idItem2 && cJSON_IsBool(idItem2)) {
                bool risposta = idItem2->valueint;
                pthread_mutex_lock(&gameListLock);
                GIOCO* partita = cercaPartitaById(idPartita);
                pthread_mutex_unlock(&gameListLock);
                if(partita){
                    // Non inviare messaggio qui - lo gestisce gestionePareggioGioco
                    gestionePareggioGioco(partita,nuovoGiocatore,risposta);
                } else {
                    inviaMessaggioSuccesso(1, nuovoGiocatore->socket, "Partita eliminata");
                }
            }else{
                printf("Il campo 'risposta' non è presente o non è booleano.\n");
                inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
            }
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(bodyJson);
}


void serverMossaGioco(char* buffer, GIOCATORE*nuovoGiocatore,cJSON *body){
     cJSON *bodyJson = cJSON_Parse(body->valuestring);
    if (!bodyJson) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
    } 
    else {
        cJSON *idItem = cJSON_GetObjectItem(bodyJson, "game_id"); 
        cJSON *idItem2 = cJSON_GetObjectItem(bodyJson, "col");
        cJSON *idItem3 = cJSON_GetObjectItem(bodyJson, "row");  
        if (idItem && cJSON_IsNumber(idItem)) {
                int idPartita = idItem->valueint;
                printf("ID partita: %d\n", idPartita);
                if (idItem2 && cJSON_IsNumber(idItem2)) {
                    if (idItem3 && cJSON_IsNumber(idItem3)) {
                        int col = idItem2->valueint;
                        int row = idItem3->valueint;
                        pthread_mutex_lock(&gameListLock);
                        GIOCO *partita=cercaPartitaById(idPartita);
                        if(partita == NULL) {
                            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Partita non trovata");
                            cJSON_Delete(bodyJson);
                            pthread_mutex_unlock(&gameListLock);
                            return;
                        }
                        bool success = aggiornaPartita(partita,nuovoGiocatore,col,row);
                        if(success){
                            handlerInviaMossePartita(nuovoGiocatore,partita);
                            handlerInviaMossePartita(switchGiocatorePartita(nuovoGiocatore,partita),partita);
                        }
                        pthread_mutex_unlock(&gameListLock);

                    }else{
                        printf("Il campo 'row' non è presente o non è un numero.\n");
                        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
                    }
                }else{
                    printf("Il campo 'col' non è presente o non è un numero.\n");
                    inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
                }
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(bodyJson);
}



void serverUscitaGioco(char *buffer, GIOCATORE *nuovoGiocatore,cJSON *body){

    //ricevere intenzione di uscire dalla partita
    cJSON *bodyJson = cJSON_Parse(body->valuestring);
    if (!bodyJson) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
    } else{
    cJSON *idItem = cJSON_GetObjectItem(bodyJson, "game_id"); 
        if (idItem && cJSON_IsNumber(idItem)) {
                int idPartita = idItem->valueint;
                pthread_mutex_lock(&gameListLock);
                GIOCO*partita = cercaPartitaById(idPartita);
                pthread_mutex_unlock(&gameListLock);
                if(partita == NULL) {
                    inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Partita non trovata");
                    cJSON_Delete(bodyJson);
                    return;
                }
                inviaEsito(partita,SCONFITTA,nuovoGiocatore);
                if(nuovoGiocatore==partita->giocatoreParticipante[0])
                    inviaEsito(partita,VITTORIA,partita->giocatoreParticipante[1]);
                else
                    inviaEsito(partita,VITTORIA,partita->giocatoreParticipante[0]);
    }else{
        printf("Il campo 'game_id' non è presente o non è un numero.\n");
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
    }}
    cJSON_Delete(bodyJson);
}


void serverUscitaPareggio(char *buffer, GIOCATORE *nuovoGiocatore, cJSON *body){
    cJSON *bodyJson = cJSON_Parse(body->valuestring);
    if (!bodyJson) {
        printf("Errore nel parsing del JSON body\n");
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
    } else{
        cJSON *idItem = cJSON_GetObjectItem(bodyJson, "game_id"); 
        if (idItem && cJSON_IsNumber(idItem)) {
            int idPartita = idItem->valueint;
            printf("ID partita: %d\n", idPartita);
            pthread_mutex_lock(&gameListLock);
            GIOCO*partita = cercaPartitaById(idPartita);
            pthread_mutex_unlock(&gameListLock);
            if(partita == NULL) {
                inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Partita non trovata");
                cJSON_Delete(bodyJson);
                return;
            }
            gestionePareggioGioco(partita,nuovoGiocatore,0);
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(bodyJson);
}

void serverRichiesteInviate(char* buffer, GIOCATORE*nuovoGiocatore,cJSON *body){
    // Invia le richieste al client
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "path", "/send_requests");
    cJSON_AddItemToObject(response, "requests", cJSON_CreateArray());
    
    // Aggiungi le richieste del giocatore
    pthread_mutex_lock(&gameListLock);
    for (int i = 0; i < MAX_GAME; i++) {
        if (Partite[i]) {
            for (int j = 0; j < MAX_GIOCATORI - 1; j++) {
                if (Partite[i]->richieste[j] && Partite[i]->richieste[j]->giocatore == nuovoGiocatore) {
                    cJSON *request_json = cJSON_CreateObject();
                    cJSON_AddNumberToObject(request_json, "game_id", Partite[i]->id);
                    cJSON_AddNumberToObject(request_json, "player_id", Partite[i]->richieste[j]->giocatore->id);
                    cJSON_AddStringToObject(request_json, "stato", Partite[i]->richieste[j]->stato == RICHIESTA_IN_ATTESA  ? "in_attesa" : "declined");
                    cJSON_AddItemToArray(cJSON_GetObjectItem(response, "requests"), request_json);
                }
            }
        }
    }
    pthread_mutex_unlock(&gameListLock);
    
    char *responseStr = cJSON_PrintUnformatted(response);
    send(nuovoGiocatore->socket, responseStr, strlen(responseStr), 0);
    
    free(responseStr);
    cJSON_Delete(response);
}

void serverRichiesteRicevute(char* buffer, GIOCATORE*nuovoGiocatore,cJSON *body){
    // Invia le richieste ricevute al client
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "path", "/received_requests");
    cJSON_AddItemToObject(response, "requests", cJSON_CreateArray());
    
    pthread_mutex_lock(&gameListLock);
    for (int i = 0; i < MAX_GAME; i++) {
        if (Partite[i] ) {
            for (int j = 0; j < MAX_GIOCATORI - 1; j++) {
                if (Partite[i]->richieste[j] && Partite[i]->giocatoreParticipante[0] == nuovoGiocatore && Partite[i]->richieste[j]->stato != RICHIESTA_RIFIUTATA) {
                    cJSON *request_json = cJSON_CreateObject();
                    cJSON_AddNumberToObject(request_json, "game_id", Partite[i]->id);
                    cJSON_AddNumberToObject(request_json, "player_id", Partite[i]->richieste[j]->giocatore->id);
                    cJSON_AddStringToObject(request_json,  "mittente", Partite[i]->richieste[j]->giocatore->nome);
                    cJSON_AddItemToArray(cJSON_GetObjectItem(response, "requests"), request_json);
                }
            }
        }
    }
    pthread_mutex_unlock(&gameListLock);
    
    char *responseStr = cJSON_PrintUnformatted(response);
    send(nuovoGiocatore->socket, responseStr, strlen(responseStr), 0);
    
    free(responseStr);
    cJSON_Delete(response);
}