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
            Partite[i]->esito=0;
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
    //const char *msg = "1";
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

void gestioneRichiestaJSONuscita(cJSON*json,int*leave_flag,int*leave_game){
    if(json!=NULL){
        cJSON *path = cJSON_GetObjectItem(json, "path");
        if (path == NULL) {
            printf("Richiesta JSON non valida: manca il campo 'path'\n");
        }
        else {
            printf("Richiesta JSON ricevuta: %s\n", path->valuestring);
            fflush(stdout);
            if (strcmp(path->valuestring, "/close") == 0) {
                printf("Richiesta uscita attesa ricevuta\n");
                *leave_flag=1;
            }
            if(strcmp(path->valuestring, "/left_game") == 0) {
                printf("Richiesta di uscita dalla partita ricevuta\n");
                *leave_game=1;
            }
        }
        cJSON_Delete(json);
    }
} 

void GameStartPlayer1(int*leave_flag,char*buffer,GAME*nuova_partita){
    cJSON* json=NULL;
    int leave_game=0;
    
    //attendo il secondo giocatore
    while(!(*leave_flag)&& !(leave_game) && (nuova_partita->giocatoreParticipante[1]==NULL)){
        json = read_with_timeout(*(nuova_partita->giocatoreParticipante[0]->socket),buffer,sizeof(buffer),10,&leave_game);
        gestioneRichiestaJSONuscita(json,leave_flag,&leave_game);
    }
    printf("Esco dalla lettura con leave_flag: %d, leave_game: %d\n", *leave_flag, leave_game);
    fflush(stdout);


    if(*leave_flag==1 || leave_game==1){
        rimuovi_game_queue(nuova_partita);
        return ;
    }
    printf("Giocatore 1 ha iniziato la partita con id: %d\n", nuova_partita->id);
    fflush(stdout);
    
    sendJoinGame(nuova_partita->giocatoreParticipante[1],nuova_partita);
    GamePlayer1(leave_flag,&leave_game,buffer,nuova_partita,nuova_partita->giocatoreParticipante[0]);
    
    printf("Giocatore 1 ha terminato la partita con id: %d\n", nuova_partita->id);
    fflush(stdout);
    //TO-DO il cambio di properitario
    rimuovi_game_queue(nuova_partita);
    
}


cJSON* read_with_timeout(int sockfd, char* buffer, size_t len, int timeout_sec,int*leave_game) {
    // Funzione per leggere dati da un socket con timeout
    //fd_set: insieme di descrittori di file
    //timeout: struttura che specifica il timeout per la lettura
    fd_set readfds;
    struct timeval timeout;

    // FD_ZERO inizializza l'insieme di descrittori di file a zero
    FD_ZERO(&readfds);

    // Aggiungi il socket al set di descrittori di file
    FD_SET(sockfd, &readfds);

    // Imposta il timeout
    timeout.tv_sec = timeout_sec;
    // Imposta i microsecondi a zero per un timeout preciso
    timeout.tv_usec = 0;

    // Attendi che il socket sia pronto per la lettura o che il timeout scada
    int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);

    //se return è negativo, si è verificato un errore
    //se return è zero, il timeout è scaduto
    //se return è maggiore di zero, il socket è pronto per la lettura
    if (ret < 0) {
        perror("ERRORE");
        *leave_game = 1;
    } else if (ret == 0) {
        printf("Timeout: nessun dato ricevuto entro %d secondi\n", timeout_sec);
    }
    else{
        int size = read(sockfd, buffer, sizeof(buffer));
        if(size > 0){
            buffer[size] = '\0';
            printf("Buffer ricevuto: %s\n", buffer);
            fflush(stdout);
            cJSON *json = cJSON_Parse(buffer);

            if (json == NULL) {
                printf("Errore nel parser JSON\n");
                *leave_game = 1;
            } else 
                return json;        
        }
    }

    // Socket pronto per la lettura
    
    return NULL;
}

void sendJoinGame(GIOCATORE*giocatore2, GAME* partita){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "start_game");
    cJSON_AddNumberToObject(root, "game_id", partita->id);
    cJSON_AddNumberToObject(root, "simbolo", 1); // Corretto: simbolo come numero
    cJSON_AddStringToObject(root, "nickname_partecipante", giocatore2->nome);
    //game_data:{TRIS: [[0,0,0],[0,0,0],[0,0,0]], esito:0, turono:0}
    cJSON *game_data = cJSON_CreateObject();
    cJSON *tris = cJSON_CreateArray();
    for (int i = 0; i < 3; i++) {
        cJSON *row = cJSON_CreateArray();
        for (int j = 0; j < 3; j++) {
            cJSON_AddItemToArray(row, cJSON_CreateNumber(0)); // Inizializza la matrice con zeri
        }
        cJSON_AddItemToArray(tris, row);
    }
    cJSON_AddItemToObject(game_data, "TRIS", tris);
    cJSON_AddNumberToObject(game_data, "esito", 0);
    cJSON_AddNumberToObject(game_data, "turno", 0);
    cJSON_AddItemToObject(root, "game_data", game_data);

    char *msg = cJSON_PrintUnformatted(root);
    send(*(partita->giocatoreParticipante[0]->socket), msg, strlen(msg), 0);
    cJSON_Delete(root);
    free(msg);
    printf("Inviato messaggio di unione partita al giocatore 1");
    fflush(stdout);
}
void GameStartPlayer2(int*leave_flag,GAME*nuova_partita,GIOCATORE*giocatore2){
    int leave_game=0;
    pthread_mutex_lock(&gameListLock);
    //fai di nuovo controllo???
    if(nuova_partita->giocatoreParticipante[1]==NULL){
        nuova_partita->giocatoreParticipante[1]=giocatore2;
      
        pthread_mutex_unlock(&gameListLock);
        sendSuccessNewGame(1,giocatore2,nuova_partita->id);
        //invio messaggio di unione partita al giocatore 1:
        GamePlayer2(leave_flag,&leave_game,nuova_partita,giocatore2);


    }else{
        *leave_flag=1;
        sendSuccessNewGame(0,giocatore2,-1);
        pthread_mutex_unlock(&gameListLock);

    }


}

void sendSuccessNewGame(int success, GIOCATORE*giocatore, int id_partita){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "success", success);
    cJSON_AddNumberToObject(root, "id_partita", id_partita);
    char *msg = cJSON_PrintUnformatted(root);
    send(*(giocatore->socket), msg, strlen(msg), 0);
    cJSON_Delete(root);
    free(msg);
}

void GamePlayer1(int *leave_flag,int*leave_game,char*buffer,GAME*nuova_partita,GIOCATORE*Giocatore1){
        printf("sono in partita: %s\n", Giocatore1->nome);
        fflush(stdout);
  
        while((!*leave_flag)&&(!*leave_game)&& (nuova_partita->esito==0)){
           
            if(nuova_partita->turno!=0){
                   sem_wait(&(nuova_partita->semaforo));   
            }
            //inviare matrice del tris al giocatore 1
            

            //controllo se la partita è terminata oppure giocatore 2 è uscito
            //inviare nuova mossa o uscire 

            //TO-DO gioco 

            nuova_partita->turno=1;//cambio turno
            sem_post(&(nuova_partita->semaforo)); 
        }
}

void GamePlayer2(int *leave_flag,int*leave_game,GAME*nuova_partita,GIOCATORE*Giocatore2){
        //stampa("sono nel game player 2\n");
        //fflush(stdout);
        printf("sono in partita: %s\n", Giocatore2->nome);
        fflush(stdout);
        while((!*leave_flag)&&(!*leave_game) && (nuova_partita->esito==0)){
            if(nuova_partita->turno!=1){
                   sem_wait(&(nuova_partita->semaforo));   
            }
            
            //inviare matrice del tris al giocatore 2


            //controllo se la partita è terminata oppure giocatore 1 è uscito
            //inviare nuova mossa o uscire

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

bool controlla_pareggio(GAME*partita,int* esito){
    
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
