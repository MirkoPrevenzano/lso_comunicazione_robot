#include "./handler.h"

void crea_json(cJSON *root,int id,char*nome){
    cJSON_AddStringToObject(root,"path","/partecipa");
    cJSON_AddNumberToObject(root,"id_partita",id);
    cJSON_AddStringToObject(root,"Proprietario",nome);
}

bool partita_in_corso(GAME*partita){
    if(partita)
        return (Partite[i]->Giocatori[1]!=NULL);
    else
        return 0;
}

void handlerInviaGames(int * socket_nuovo,char*buffer){
    cJSON*root = cJSON_createObject();
    cJSON *partite_array = cJSON_CreateArray();
    cJSON *partita = NULL;
    for(int i=0,i<MAX_GAME,i++){
        if(Partite[i]!=NULL){
            if(!partita_in_corso(Partite[i])){
            partita = cJSON_CreateObject();
            crea_Json(partita,Partite[i]->id,Partite[i]->GiocatoreProprietario->nome)
            cJSON_AddItemToArray(partite_array, partita);
    }}}

        cJSON_AddItemToObject(root, "partite", partite_array);
        char*json_str=cJSON_PrintfUnformatted(root);
        cJON_Delete(root);
        send(*(socket_nuovo),json_str,strlen(json_str),0);
}

//inviare con un send un messaggio di errore nei vari casi* TO-DO

void handlerGames(int*leave_flag,int*socket_nuovo,char*buffer,GIOCATORE *giocatore){
    int size = read(*socket_nuovo, buffer, sizeof(buffer)-1);
    if(size > 0){
        buffer[size] = '\0';
        printf("Buffer ricevuto: %s\n", buffer);
        fflush(stdout);
        cJSON *json = cJSON_Parse(buffer);

    if (json == NULL) {
        printf("Errore nel parser JSON\n");
        *leave_flag = 1;
    } else {
        cJSON *path = cJSON_GetObjectItem(json, "path");
        cJSON *body = cJSON_GetObjectItem(json, "body");

        if (path && path->valuestring) {
            if (strcmp(path->valuestring, "/crea_partita") == 0) {
                crea_game(leave_flag,buffer,giocatore);
            }else if(strcmp(path->valuestring, "/partecipa_partita")){
                partecipa_partita_json(leave_flag,socket_nuovo,buffer,body,giocatore);
            }else {
                printf("no path register: %s\n", path->valuestring);
                *leave_flag = 1;
            }
        } else {
            printf("path non valido\n");
            *leave_flag = 1;
        }
        cJSON_Delete(body);
        cJSON_Delete(path); 


    }
}

}


void partecipa_partita_json(int *leave_flag,int*socket_nuovo,char*buffer,cJSON*body,GIOCATORE *giocatore){
    if (body && cJSON_IsString(body)) {
        cJSON *body_json = cJSON_Parse(body->valuestring);
        if (body_json) {
            cJSON *id_item = cJSON_GetObjectItem(body_json, "id");
            if (id_item && (cJSON_IsNumber(id_item))) {
                printf("id : %s\n", id_item->valuestring);
                partecipa_game(leave_flag,id_item->valuestring,buffer,giocatore);
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
}


void handlerRiceviJsonMossa(int*leave_game,char*buffer,GIOCATORE *giocatore,GAME*partita){
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
}    