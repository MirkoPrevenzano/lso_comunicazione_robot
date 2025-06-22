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


//questa funzione gestisce la richiesta di un giocatore che vuole unirsi a una lobby
void joinLobby(int*leave_flag,cJSON * body,GIOCATORE*nuovo_giocatore){
    printf("Stampa di body: %s\n", body->valuestring);
    if (body && cJSON_IsString(body)) {
        //estraggo game_id dal body
        cJSON *body_json = cJSON_Parse(body->valuestring);
        if (!body_json) {
            printf("Errore nel parsing del body JSON.\n");
            *leave_flag = 1; // Indica un errore
            return;
        }
        // Controllo se il campo "game_id" è presente e se è un numero
        cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
        if (id_item && cJSON_IsNumber(id_item)) {
            int id_partita = id_item->valueint;
            GAME *partita = searchPartitaById(id_partita);
            if (!partita_in_corso(partita)) {
                //GameStartPlayer2(leave_flag, partita, nuovo_giocatore);

                //TO-DOOOOOO

                
                remove_request_by_player(nuovo_giocatore);             
            } else {
                printf("Partita con ID %d non trovata.\n", id_partita);
                *leave_flag = 1; // Indica un errore
            }
        } else {
            printf("Il campo 'game_id' non è presente o non è un numero.\n");
            *leave_flag = 1; // Indica un errore
        }
        cJSON_Delete(body_json); // Libera la memoria allocata
        return;
    } else {
        printf("Il body della richiesta non è valido.\n");
        *leave_flag = 1; // Indica un errore
    }

}



GAME* searchPartitaById(int id){
    pthread_mutex_lock(&gameListLock);
    for(int i=0;i<MAX_GAME;i++)
        if(Partite[i]->id==id){
            pthread_mutex_unlock(&gameListLock);
            return Partite[i];
        }
    pthread_mutex_unlock(&gameListLock);
    return NULL;

}
/*void partecipa_partita_json(int *leave_flag,int*socket_nuovo,char*buffer,cJSON*body,GIOCATORE *giocatore){
    if (body && cJSON_IsString(body)) {
        cJSON *body_json = cJSON_Parse(body->valuestring);
        if (body_json) {
            cJSON *id_item = cJSON_GetObjectItem(body_json, "id");
            if (id_item && (cJSON_IsNumber(id_item))) {
                printf("id : %s\n", id_item->valuestring);
                //partecipa_game(leave_flag,id_item->valuestring,buffer,giocatore);
            } else {
                printf("Il campo 'id' non è presente o non è un numero\n");
                *leave_flag = 1;
            }
            cJSON_Delete(id_item); // Libera la memoria del JSON annidato
        } else {
            printf("Errore  JSON annidato in 'body'\n");
            *leave_flag = 1;
        }
    } else {
        printf("Il campo 'body' non è presente o non è un numero\n");
        *leave_flag = 1;
    }
}*/


/*void handlerRiceviJsonMossa(int*leave_game,char*buffer,GIOCATORE *giocatore,GAME*partita){
    int size = read(*(giocatore->socket), buffer, sizeof(buffer)-1);
    if(size > 0){
        buffer[size] = '\0';
        printf("Buffer ricevuto: %s\n", buffer);
        fflush(stdout);
        cJSON *json = cJSON_Parse(buffer);

    if (json == NULL) {
        printf("Errore nel parser JSON\n");
        *leave_game = 0;
    } else {
        cJSON *path = cJSON_GetObjectItem(json, "path");
        cJSON *body = cJSON_GetObjectItem(json, "body");

        if (path && path->valuestring) {
            if (strcmp(path->valuestring, "/MossaGiocatore1") == 0) {
                cJSON *i = cJSON_GetObjectItem(body, "j");
                cJSON *j = cJSON_GetObjectItem(body, "i");
                if (i && (cJSON_IsNumber(i))) {
                    if(j && (cJSON_IsNumber(j))){
                        ModificaArrayTris(i->valuestring,j->valuestring,1,partita);
                    }else
                        *leave_game = 0;
                } else {
                    *leave_game = 0;
                }
            }else if(strcmp(path->valuestring, "/MossaGiocatore2")){
                cJSON *i = cJSON_GetObjectItem(body, "j");
                cJSON *j = cJSON_GetObjectItem(body, "i");
                if (i && (cJSON_IsNumber(i))) {
                    if(j && (cJSON_IsNumber(j))){
                        ModificaArrayTris(i->valuestring,j->valuestring,2,partita);
                    }else
                        *leave_game = 0;
                } else {
                    *leave_game = 0;
                }
                
            }else {
                printf("no path register: %s\n", path->valuestring);
                *leave_game = 0;
            }
        } else {
            printf("path non valido\n");
            *leave_game = 0;
        }
        cJSON_Delete(body);
        cJSON_Delete(path); 


    }
    }
}  */