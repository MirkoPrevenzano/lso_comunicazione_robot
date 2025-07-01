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
        if(Partite[i]->id==id){
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

