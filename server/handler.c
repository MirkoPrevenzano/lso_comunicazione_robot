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
    pthread_mutex_lock(&gameListLock);
    for(int i=0;i<MAX_GAME;i++)
        if(Partite[i]->id==id){
            pthread_mutex_unlock(&gameListLock);
            return Partite[i];
        }
    pthread_mutex_unlock(&gameListLock);
    return NULL;

}

void HandlerInviaMovesPartita(GIOCATORE*giocatore,GAME*partita){
    cJSON*root = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();
    crea_json(array,partita->id,giocatore->nome);

    cJSON_AddItemToObject(root, "game_response",array);
    char *json_str=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    printf("Invio griglia partita al socket %d: %s\n", giocatore->socket, json_str);
    fflush(stdout);
    send(giocatore->socket,json_str,strlen(json_str),0);
    free(json_str); // libera la memoria allocata da cJSON_PrintUnformatted

}

void handler_game_response(GIOCATORE*giocatore,GAME*partita){
    pthread_mutex_lock(&gameListLock);
    printf("Richiesta della griglia di gioco ricevuta\n");
    int indice;
    if(giocatore==partita->giocatoreParticipante[0])
        indice=1;
    else
        indice=0;
    HandlerInviaMovesPartita(partita->giocatoreParticipante[indice],partita);
    pthread_mutex_unlock(&gameListLock);
}
