#include "game.h"
#include "server.h"
void aggiungi_game_queue(GAME *nuova_partita,GIOCATORE* giocatoreProprietario){

    //controllo che non ci siano più di MAX_GAME partite
    pthread_mutex_lock(&gameListLock);

    if(numero_partite>=MAX_GAME){
        perror("numero max partite superato");
        return;
    }

    //controllo che il giocatore non sia già in una partita
    for(int i=0; i < MAX_GAME; ++i){
        if(Partite[i]){
            if(Partite[i]->giocatoreParticipante[0] && Partite[i]->giocatoreParticipante[0]->id == giocatoreProprietario->id){
                perror("giocatore già in una partita");
                pthread_mutex_unlock(&gameListLock);
                return;
            }
        }
    }

    int i;
    for( i=0; i < MAX_GAME; i++){
        if(!Partite[i]){
            Partite[i]=nuova_partita;
            Partite[i]->giocatoreParticipante[0] = giocatoreProprietario;
            Partite[i]->id=numero_partite;
            Partite[i]->turno=0;
            Partite[i]->esito=-1;
            break;
            
        }
    }
    numero_partite++;
    printf("Partita creata con id: %d indice %d\n",nuova_partita->id, i);
    fflush(stdout);
    pthread_mutex_unlock(&gameListLock);
};


void rimuovi_game_queue(GAME*partita){

    pthread_mutex_lock(&gameListLock);
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
    pthread_mutex_unlock(&gameListLock);
}

void remove_game_by_player_id(int id) {
    pthread_mutex_lock(&gameListLock);
    for (int i = 0; i < MAX_GAME; ++i) {
        if (Partite[i] && Partite[i]->giocatoreParticipante[0] && Partite[i]->giocatoreParticipante[0]->id == id) {
            free(Partite[i]);
            Partite[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&gameListLock);
}


void new_game(int*leave_flag,char*buffer,GIOCATORE*giocatore){
    GAME *nuova_partita = (GAME *)malloc(sizeof(GAME));
    aggiungi_game_queue(nuova_partita,giocatore);
    const char *msg = "1";
    if(!nuova_partita){
        sendSuccessNewGame(0,giocatore, -1);
        return;
    }
    else{
        sendSuccessNewGame(1,giocatore, nuova_partita->id);
        printf("Partita creata con id: %d\n",nuova_partita->id);
        fflush(stdout);
        printf("Giocatore %s ha creato una partita\n",giocatore->nome);
        GameStartPlayer1(leave_flag,buffer,nuova_partita);
        
    }
   
    
}

 
void GameStartPlayer1(int*leave_flag,char*buffer,GAME*nuova_partita){
    int ricevuto=0;
    //se ricevo il numero 1 vuol dire che è stato premuto il pulsante esci
    //recv_with_timeout usa la funzione select per fare un timeout di 10 secondi
    while((!leave_flag) && (nuova_partita->giocatoreParticipante[1]==NULL)){
        ricevuto = recv_with_timeout(*(nuova_partita->giocatoreParticipante[0]->socket),buffer,sizeof(buffer),10);
        if(ricevuto==1){
            *leave_flag=1;
        }
    }
    if(*leave_flag==1){
            rimuovi_game_queue(nuova_partita);
            return ;
        }
    

    GamePlayer1(leave_flag,buffer,nuova_partita,nuova_partita->giocatoreParticipante[1]);
    
    //TO-DO il cambio di properitario
    rimuovi_game_queue(nuova_partita);
    
}

int recv_with_timeout(int sockfd, void* buf, size_t len, int timeout_sec) {
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;

    int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
    if (ret < 0) {
        perror("select");
        return -1; // errore
    } else if (ret == 0) {
        // Timeout scaduto
        printf("Timeout: nessun dato ricevuto entro %d secondi\n", timeout_sec);
        return 0;
    }

    // Socket pronto per la lettura
    return recv(sockfd, buf, len, 0);
}




void sendSuccessNewGame(int success, GIOCATORE*giocatore, int id_partita){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "success", 1);
    cJSON_AddNumberToObject(root, "id_partita", id_partita);
    char *msg = cJSON_PrintUnformatted(root);
    send(*(giocatore->socket), msg, strlen(msg), 0);
    cJSON_Delete(root);
    free(msg);
}

void GamePlayer1(int *leave_flag,char*buffer,GAME*nuova_partita,GIOCATORE*Giocatore1){
        while((!leave_flag) && !(nuova_partita->esito>0)){
            if(nuova_partita->turno!=0){
                   sem_wait(&(nuova_partita->semaforo));   
            }
            
            //TO-DO gioco 

            nuova_partita->turno=1;//cambio turno
            sem_post(&(nuova_partita->semaforo)); 
        }
}

void GamePlayer2(int *leave_flag,char*buffer,GAME*nuova_partita,GIOCATORE*Giocatore2){
        while((!leave_flag) && !(nuova_partita->esito>0)){
            if(nuova_partita->turno!=1){
                   sem_wait(&(nuova_partita->semaforo));   
            }
            
            //TO-DO gioco 

            nuova_partita->turno=0;//cambio turno
            sem_post(&(nuova_partita->semaforo)); 
        }
}



/*void partecipa_game(int*leave_flag,int id_lobby,char*buffer,GIOCATORE *giocatore){
    pthread_mutex_lock(&gameListLock);
    GAME*gioco=SearchGiocoByID(id_lobby);
    
    if(gioco!=NULL)
        gioco->giocatori[1]=giocatore;
    else{
        pthread_mutex_unlock(&gameListLock););
        return;//inviare messaggio d'errore TO-DO
    }
    pthread_mutex_unlock(&gameListLock););
    
    StartGame(1,buffer,gioco,giocatore);

    pthread_mutex_lock(&gameListLock);
    gioco->giocatori[1]=NULL;
    pthread_mutex_unlock(&gameListLock););
}

GAME* searchPartitaById(int id){
    for(int i=0;i<MAX_GAME;i++)
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
            for(int j=0;j<3;j++)
                if(matrice[i][j]==0)
                    return 0;
    }else
        return 0;

    return 1;

}


void inviaJsonMatrice(GAME*partita, GIOCATORE*giocatore){
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
    cJSON_AddNumberToObject(root,"id_partita",partita->id);
    if(*esito==1){
        cJSON_AddStringToObject(root,"vincitore",partita->giocatori[0]->nome);
        cJSON_AddStringToObject(root,"perdente",partita->giocatori[1]->nome);
    }
    else if(*esito==2){
        cJSON_AddStringToObject(root,"vincitore",partita->giocatori[1]->nome);
        cJSON_AddStringToObject(root,"perdente",partita->giocatori[0]->nome);
    }

    char*json_str=cJSON_PrintfUnformatted(root);
    cJON_Delete(root);
    send(*(partita->giocatori[0]->socket),json_str,strlen(json_str),0);
    send(*(partita->giocatori[1]->socket),json_str,strlen(json_str),0);
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
}*/

//esito -1 -> errore partita ; metere un timeout per evitare deadlock
//esito 0 = parità -> numero di default quindi nessun vincitore
//esito 1 o 2 -> giocatore vincente
