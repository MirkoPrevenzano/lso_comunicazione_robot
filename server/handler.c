#include "./handler.h"
#include "./server.h"

// Dichiarazioni esterne delle variabili globali già in server.h

void crea_json(cJSON *root,int id,char*nome){
    cJSON_AddNumberToObject(root,"id_partita",id);
    cJSON_AddStringToObject(root,"proprietario",nome);
}

bool partita_in_corso(GAME*partita){
    if(partita)
        return (partita->stato_partita == IN_CORSO);
    else
        return false;
}

void handlerInviaGames(int  socket_nuovo){
    cJSON*root = cJSON_CreateObject();
    cJSON *partite_array = cJSON_CreateArray();
    cJSON *partita = NULL; 
    for(int i=0;i<MAX_GAME;i++){
        if(Partite[i]!=NULL){
            if(!partita_in_corso(Partite[i]) && Partite[i]->giocatoreParticipante[0]->socket != socket_nuovo){
                partita = cJSON_CreateObject();
                crea_json(partita,(int)Partite[i]->id,Partite[i]->giocatoreParticipante[0]->nome);
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


GAME * searchPartitaById(int id){
    for(int i=0;i<MAX_GAME;i++)
        if(Partite[i] && Partite[i]->id==id){
            //pthread_mutex_unlock(&gameListLock);
            return Partite[i];
        }
    return NULL;

}

void HandlerInviaMovesPartita(GIOCATORE*giocatore,GAME*partita){
    
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

void handler_game_response(GIOCATORE*giocatore,GAME*partita){
    printf("Richiesta della griglia di gioco ricevuta\n");
    HandlerInviaMovesPartita(giocatore,partita);
}

GIOCATORE* switchGiocatorePartita(GIOCATORE*giocatore,GAME*partita){

    if(giocatore==partita->giocatoreParticipante[0])
      return partita->giocatoreParticipante[1];
    else
       return partita->giocatoreParticipante[0];
}



void ServerWaitingGames(char* buffer, GIOCATORE*nuovo_giocatore, int socket_nuovo){
    pthread_mutex_lock(&gameListLock);
    printf("Richiesta di giochi in attesa ricevuta\n");
    handlerInviaGames(socket_nuovo);
    pthread_mutex_unlock(&gameListLock);
}

void ServerNewGames(char* buffer, GIOCATORE*nuovo_giocatore,int *leave_flag){
    pthread_mutex_lock(&gameListLock);
    printf("Richiesta di creazione partita ricevuta\n");
    new_game(leave_flag,buffer,nuovo_giocatore);
    pthread_mutex_unlock(&gameListLock);
}

void ServerAddRequest(char* buffer, GIOCATORE*nuovo_giocatore,int*leave_flag,cJSON *body){
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
            aggiungi_richiesta(id_partita, nuovo_giocatore);
        } else {
            printf("Il campo 'id' non è presente o non è un numero.\n");
            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
        }
        cJSON_Delete(body_json);
    }
 }


void ServerRemoveRequest(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
        printf("Richiesta di rimozione da una partita ricevuta\n");
        printf("Body ricevuto: %s\n", body->valuestring);
        fflush(stdout);
        
        // Parsa il JSON body
        cJSON *body_json = cJSON_Parse(body->valuestring);
        if (!body_json) {
            printf("Errore nel parsing del JSON body\n");
            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
        } else {
            cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
            if (id_item && cJSON_IsNumber(id_item)) {
                int id_partita = id_item->valueint;
                printf("ID partita: %d\n", id_partita);
                // Rimuovi la richiesta dalla partita
                rimuovi_richiesta(id_partita, nuovo_giocatore); 
            } else {
                printf("Il campo 'game_id' non è presente o non è un numero.\n");
                send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
            }
            cJSON_Delete(body_json);
        }
 }


void ServerAcceptRequest(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
    printf("Richiesta di accetazione una partita ricevuta\n");
    printf("Body ricevuto: %s\n", body->valuestring);
    fflush(stdout);
    
    // Parsa il JSON body
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
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
                GIOCATORE * nuovo_giocatore_2 = SearchPlayerByid(id_player);
                pthread_mutex_lock(&gameListLock);
                accetta_richiesta(searchRichiesta(id_partita, SearchPlayerByid(id_player)),id_partita,nuovo_giocatore, nuovo_giocatore_2);
                pthread_mutex_unlock(&gameListLock);
            }else{
            aggiungi_richiesta(id_partita, nuovo_giocatore);
            printf("Il campo 'player_id' non è presente o non è un numero.\n");
            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
        }}else {
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
        }
        cJSON_Delete(body_json);
    }
}


void ServerDeclineRequest(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
    printf("rifiuto di una richiesta ricevuta\n");
    printf("Body ricevuto: %s\n", body->valuestring);
    fflush(stdout);
    
    // Parsa il JSON body
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
    } else {
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
        cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "player_id");
        if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            printf("ID partita: %d\n", id_partita);
            if (id_item_2 && cJSON_IsNumber(id_item_2)) {
                int id_player = id_item_2->valueint;
                printf("ID Giocatore della richiesta: %d\n", id_player);
                GIOCATORE * nuovo_giocatore_2 = SearchPlayerByid(id_player);
                pthread_mutex_lock(&gameListLock);
                rifiuta_richiesta(searchRichiesta(id_partita, nuovo_giocatore_2),id_partita,nuovo_giocatore); 
                pthread_mutex_unlock(&gameListLock);
            }else{
                printf("Il campo 'player_id' non è presente o non è un numero.\n");
                send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
            }
        } else {
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
        }
        cJSON_Delete(body_json);
    }

}

void ServerVictoryGame(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
    } else {
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
        cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "risposta");
        if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            if(id_item_2 && cJSON_IsBool(id_item_2)) {
                bool risposta = id_item_2->valueint;
                pthread_mutex_lock(&gameListLock);
                GAME* partita = searchPartitaById(id_partita);
                pthread_mutex_unlock(&gameListLock);
                if(risposta){
                    resetGame(partita);
                    printf("il giocatore vincitore (ID : %d) ha deciso di diventare proprietario del gioco (ID : %d) ",nuovo_giocatore->id,id_partita);
                    send_success_message(1,nuovo_giocatore->socket,"sei diventato proprietario");
                }else{
                    send_success_message(1,nuovo_giocatore->socket,"partita cancellata");
                    printf("il giocatore vincitore (ID : %d) ha deciso di non diventare proprietario del Il tuo abbandono darà sconfitta a tavolinogioco (ID : %d) ",nuovo_giocatore->id,id_partita);
                    pthread_mutex_lock(&gameListLock);
                    rimuovi_game_queue(partita);
                    pthread_mutex_unlock(&gameListLock);
                }
            }else{
                printf("Il campo 'risposta' non è presente o non è booleano.\n");
                send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
            }
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(body_json);

}


void ServerDrawGame(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
     cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
    } else {
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
        cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "risposta");
        if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            if(id_item_2 && cJSON_IsBool(id_item_2)) {
                bool risposta = id_item_2->valueint;
                pthread_mutex_lock(&gameListLock);
                GAME* partita = searchPartitaById(id_partita);
                pthread_mutex_unlock(&gameListLock);
                if(partita){
                    send_success_message(1, nuovo_giocatore->socket, "Richiesta di rivincita ricevuta");
                    sleep(1);
                    GestionePareggioGame(partita,nuovo_giocatore,risposta);
                } else {
                    send_success_message(0, nuovo_giocatore->socket, "Partita eliminata");
                }
            }else{
                printf("Il campo 'risposta' non è presente o non è booleano.\n");
                send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
            }
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(body_json);
}


void ServerGameMove(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){
     cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
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
                        GAME*partita=searchPartitaById(id_partita);
                        bool success = aggiorna_partita(partita,nuovo_giocatore,col,row);
                        if(success){
                            handler_game_response(nuovo_giocatore,partita);
                            handler_game_response(switchGiocatorePartita(nuovo_giocatore,partita),partita);

                        }
                        pthread_mutex_unlock(&gameListLock);

                    }else{
                        printf("Il campo 'row' non è presente o non è un numero.\n");
                        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                    }
                }else{
                    printf("Il campo 'col' non è presente o non è un numero.\n");
                    send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                }
        }else{
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
        }
    }
    cJSON_Delete(body_json);
}



void ServerExitGame(char* buffer, GIOCATORE*nuovo_giocatore,cJSON *body){

    //ricevere intenzione di uscire dalla partita
    cJSON *body_json = cJSON_Parse(body->valuestring);
    if (!body_json) {
        printf("Errore nel parsing del JSON body\n");
        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
    } else{
    cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id"); 
        if (id_item && cJSON_IsNumber(id_item)) {
                int id_partita = id_item->valueint;
                printf("ID partita: %d\n", id_partita);
                pthread_mutex_lock(&gameListLock);
                GAME*partita = searchPartitaById(id_partita);
                pthread_mutex_unlock(&gameListLock);
                InviaEsito(partita,SCONFITTA,nuovo_giocatore);
                if(nuovo_giocatore==partita->giocatoreParticipante[0])
                    InviaEsito(partita,VITTORIA,partita->giocatoreParticipante[1]);
                else
                    InviaEsito(partita,VITTORIA,partita->giocatoreParticipante[0]);
    }else{
        printf("Il campo 'game_id' non è presente o non è un numero.\n");
        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
    }}
    cJSON_Delete(body_json);
}