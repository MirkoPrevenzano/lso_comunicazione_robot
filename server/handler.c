#include "./handler.h"

void crea_json(cJSON *root,int id,char*nome){
    cJSON_AddNumberToObject(root,"id_partita",id);
    cJSON_AddStringToObject(root,"proprietario",nome);
}

bool partita_in_corso(GAME*partita){
    if(partita)
        return (partita->giocatoreParticipante[1]!=NULL);
    else
        return false;
}

void handlerInviaGames(int * socket_nuovo){
    cJSON*root = cJSON_CreateObject();
    cJSON *partite_array = cJSON_CreateArray();
    cJSON *partita = NULL; 
    for(int i=0;i<MAX_GAME;i++){
        if(Partite[i]!=NULL){
            if(!partita_in_corso(Partite[i])){
                partita = cJSON_CreateObject();
                crea_json(partita,(int)Partite[i]->id,Partite[i]->giocatoreParticipante[0]->nome);
                cJSON_AddItemToArray(partite_array, partita);
            }
        }
    }

        cJSON_AddItemToObject(root, "partite", partite_array);
        char *json_str=cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        send(*(socket_nuovo),json_str,strlen(json_str),0);
        free(json_str); // libera la memoria allocata da cJSON_PrintUnformatted

}

//inviare con un send un messaggio di errore nei vari casi* TO-DO

void joinLobby(int*leave_flag,char*buffer,GIOCATORE*nuovo_giocatore){
    int size = read(*nuovo_giocatore->socket, buffer, sizeof(buffer)-1);
    if(size > 0){
        buffer[size] = '\0';
        printf("Buffer ricevuto: %s\n", buffer);
        fflush(stdout);
        cJSON *json = cJSON_Parse(buffer);

    if (json == NULL) {
        printf("Errore nel parser JSON\n");
        *leave_flag = 1;
    } else {
            checkGame(buffer,nuovo_giocatore,nuovo_giocatore->socket,leave_flag);
        }
    }
     printf("Errore\n");
    *leave_flag = 1;
    return;
}

void checkGame(char* buffer, GIOCATORE*nuovo_giocatore, int *socket_nuovo, int *leave_flag){
        cJSON *json = cJSON_Parse(buffer);
        if (!json) return;
        cJSON *path = cJSON_GetObjectItem(json, "path");
        cJSON *body = cJSON_GetObjectItem(json, "body");

        if (strcmp(path->valuestring, "/join_game") == 0) {
            cJSON *body_json = cJSON_Parse(body->valueint);
            if(cJSON_IsNumber(body_json)){
            if(partita_in_corso(searchPartitaById(body->valueint))){
                    GameStartPlayer2(leave_flag,buffer,searchPartitaById(body->valueint),nuovo_giocatore);
                }
            }
        }

        return ;
}

GAME* searchPartitaById(int id){
    for(int i=0;i<MAX_GAME;i++)
        if(Partite[i]->id==id)
            return Partite[i];
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