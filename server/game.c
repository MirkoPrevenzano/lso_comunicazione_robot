#include "./game.h"

GAME*aggiungi_game_queue(GAME *nuova_partita,GIOCATORE* giocatoreProprietario){
    pthread_mutex_lock(&lock3);
    for(int i=0; i < MAX_GAME; i++){
        if(!Partite[i]){
            Partite[i]=nuova_partita;
            Partite[i]->GiocatoreProprietario = giocatoreProprietario;
            Partite[i]->Giocatori[0] = giocatoreProprietario;
            Partite[i]->id=numero_partite;
            Partite[i]->turno=0;
            Partite[i]->esito=-1;
            break;
        }
    }
    numero_partite++;
    pthread_mutex_unlock(&lock3);
    return nuova_partita;
};
void rimuovi_game_queue(GAME*partita){
    pthread_mutex_lock(&lock3);
    if(partita!=NULL){
    numero_partite--;
    for(int i=0; i < MAX_GAME; ++i){
		if(Partite[i]){
			if(Partite[i] == partita){
				Partite[i] = NULL;
				break;
			}
		}
	}
    free(partita);
}
    pthread_mutex_unlock(&lock3);
}

void crea_game(int*leave_flag,char*buffer,GIOCATORE*giocatore){
    GAME *nuova_partita = (GAME *)malloc(sizeof(GAME));
    aggiungi_game_queue(nuova_partita,giocatore);

    int leave_game=1;//pulsante per uscire
    while(nuova_partita->giocatori[1]==NULL && (leave_game)){
        //per ricevere il pulsante exit TO-DO
    }
    if(leave_game==0)
        rimuovi_game_queue(nuova_partita);
    else{
        StartGame(0,buffer,nuova_partita,giocatore);
        while(nuova_partita->giocatori[1]!=NULL){}
    }
    
    rimuovi_game_queue(nuova_partita);
    
}

void partecipa_game(int*leave_flag,int id_lobby,GIOCATORE*giocatore,char*buffer){
    pthread_mutex_lock(&lock3);
    GAME*gioco=SearchGiocoByID(id);
    
    if(gioco!=NULL)
        gioco->Giocatori[1]=giocatore;
    else{
        pthread_mutex_unlock(&lock3);
        return;//inviare messaggio d'errore TO-DO
    }
    pthread_mutex_unlock(&lock3);
    
    StartGame(1,buffer,gioco,giocatore);

    pthread_mutex_lock(&lock3);
    gioco->Giocatori[1]=NULL;
    pthread_mutex_unlock(&lock3);
}

GAME*SearchGiocoByID(int id){
    for(int i=0,i<MAX_GAME,i++)
        if(Partite[i]->id==id)
            return Partite[i];
    return NULL;

}


bool controlla_vittoria(GAME*partita,int giocatore,int*esito){   
    int(*matrice)[3]=partita->TRIS;
    
for (int i = 0; i < 3; i++) {
    if ((matrice[i][0] == giocatore && matrice[i][1] == giocatore && matrice[i][2] == giocatore) ||
        (matrice[0][i] == giocatore && matrice[1][i] == giocatore && matrice[2][i] == giocatore)) {
        *esito=giocatore;
        return 1; // Vittoria
    }
}
// Diagonali
if ((matrice[0][0] == giocatore && matrice[1][1] == giocatore && matrice[2][2] == giocatore) ||
    (matrice[0][2] == giocatore && matrice[1][1] == giocatore && matrice[2][0] == giocatore)) {
    *esito=giocatore;
    return 1; // Vittoria
}

return 0; // Nessuna vittoria
}

bool controlla_pareggio(GAME*partita,int esito){
    
    if(esito!=0){
    int(*matrice)[3]=partita->TRIS;
    for(int i=0;i<3;i++)
        for(int j=0;i<3;j++)
            if(matrice[i][j]==0)
                return 0
    }else
        return 0;

    return 1;

}
//giocatore 1 avra turno 0 -> il suo simbolo è il numero 1, esito=1 vittoria giocatore 1
//giocatore 2 avrà turno 1 -> il suo simbolo è il numero 2, esito=2 vittoria giocatore 2
void StartGame(int turno,char*buffer,GAME*partita,GIOCATORE*giocatore){
        int leave_game=1;
        int esito=0;
        while(partita_in_corso(partita)&&(leave_game)){
            sem_wait(&(partita->semaforo));
            inviaJsonMatrice(buffer,partita,giocatore);
            if(partita->turno==turno){/
                riceviJsonMossa(&leave_game,partita,giocatore);
                if(leave_game==0){
                    if(partita->esito==-1)
                        //errore  TO-DO
                    sem_post(&(partita->semaforo));
                    break;
                }
                if(controlla_vittoria(partita,turno+1,&esito)||controllo_pareggio(partita,esito)){
                    inviaEsitoPartita(esito,partita);
                    switchTurno(partita);
                    partita->esito=esito;
                    leave_game=0;
                    sem_post(&(partita->semaforo));
                }else
                    switchTurno(partita);
            }else
                sem_post(&(partita->semaforo));
            sem_post(&(partita->semaforo));
        }
        //TO-DO se partita non è più in corso in caso di errore
        //esempio:disconnessione dalla rete
        //potrebbe esserci un deadlock
}

void switchTurno(GAME*partita){
    switch(partita->turno){
        case 0:
        partita->turno=1;
        break;
        case 1:
        partita->turno=0;
        break;
    }
}

void inviaJsonMatrice(GAME*partita,GIOCATORE*giocatore){
    cJSON*root = cJSON_createObject();
    int(*matrice)[3]=partita->TRIS;
    cJSON *json_array = cJSON_CreateArray();
    for(int i=0;i<3;i++){
        cJSON *json_riga = cJSON_CreateArray();
        for(int j=0;j<3;j++){
            cJSON_AddItemToArray(json_riga, cJSON_CreateNumber(matrice[i][j]));
        }
        cJSON_AddItemToArray(json_array, json_riga);
    }

        cJSON_AddItemToObject(root, "partita_tris", json_array);
        
        char*json_str=cJSON_PrintfUnformatted(root);
        cJON_Delete(root);
        send(*(giocatore->socket),json_str,strlen(json_str),0);
}
void inviaEsitoPartita(int*esito,GAME*partita){
    cJSON*root = cJSON_createObject();
    cJSON_AddStringToObject(root,"path","/esito");
    cJSON_AddNumberToObject(root,"id_partita",id);
    if(*esito==1){
        cJSON_AddStringToObject(root,"vincitore",partita->Giocatori[0]->nome);
        cJSON_AddStringToObject(root,"perdente",partita->Giocatori[1]->nome);
    }
    else if(*esito==2){
        cJSON_AddStringToObject(root,"vincitore",partita->Giocatori[1]->nome);
        cJSON_AddStringToObject(root,"perdente",partita->Giocatori[0]->nome);
    }

    char*json_str=cJSON_PrintfUnformatted(root);
    cJON_Delete(root);
    send(*(partita->Giocatori[0]->socket),json_str,strlen(json_str),0);
    send(*(partita->Giocatori[1]->socket),json_str,strlen(json_str),0);
}
void RiceviJsonMossa(int*leave_game,char*buffer,GAME*partita,GIOCATORE*giocatore){
    if(partita->esito!=-1){
        *leave_game=0;
        return;
    }
    handlerRiceviJsonMossa(leave_game,buffer,giocatore,partita);

}
void ModificaArrayTris(int i,int j,int giocatore,GAME*partita){
    partita->TRIS[i][j]=giocatore;
}


