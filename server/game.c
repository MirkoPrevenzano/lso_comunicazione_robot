#include "game.h"
#include "server.h"

// Dichiarazione extern per funzioni handler
extern void handler_game_response(GIOCATORE* giocatore, GAME* partita);

void aggiungi_game_queue(GAME *nuova_partita,GIOCATORE* giocatoreProprietario){

    // MODIFICA: Controllo parametri null prima di procedere
    if (!nuova_partita || !giocatoreProprietario) {
        fprintf(stderr, "Parametri non validi\n");
        return;
    }


    // MODIFICA: Controllo numero partite ora protetto dal mutex
    if(numero_partite>=MAX_GAME){
        // MODIFICA: Sostituito perror con fprintf per errore logico
        fprintf(stderr, "numero max partite superato\n");
        return;
    }


    int i;
    bool trovato = false;
    for( i=0; i < MAX_GAME; i++){
        if(!Partite[i] && !trovato){
            Partite[i]=nuova_partita;
            Partite[i]->giocatoreParticipante[0] = giocatoreProprietario;
            Partite[i]->id=numero_partite;
            Partite[i]->numero_richieste = 0; // MODIFICA: Inizializza numero_richieste
            
            // MODIFICA: Inizializza tutto l'array richieste a NULL
            for(int j = 0; j < MAX_GIOCATORI-1; j++) {
                Partite[i]->richieste[j] = NULL;
            }
            
            // MODIFICA: Inizializza la griglia TRIS a VUOTO 
            for(int row = 0; row < 3; row++) {
                for(int col = 0; col < 3; col++) {
                    Partite[i]->griglia[row][col] = VUOTO; // Inizializza ogni cella a VUOTO (0)
                }
            }
            
            //a cosa serve? il semaforo serve per sincronizzare l'accesso alla partita perchè può essere accessibile da più thread
            //con sem_init si inizializza il semaforo uno ad uno i parametri sono: il semaforo, il valore iniziale (0 o 1), e il valore di condivisione (0 per processo, 1 per thread)
            sem_init(&Partite[i]->semaforo, 0, 1); // MODIFICA: Inizializza semaforo
            Partite[i]->turno=0;
            Partite[i]->esito= NESSUN_ESITO;
            Partite[i]->stato_partita = IN_ATTESA;
            Partite[i]->voti_pareggio = 0; // MODIFICA: Inizializza voti_pareggio 
            trovato = true;
            
        }
    }
    numero_partite++;
    printf("Partita creata con id: %d indice %d\n",nuova_partita->id, i);
    fflush(stdout);
};


void rimuovi_game_queue(GAME*partita){

    if(partita!=NULL){
        // MODIFICA: Decremento numero_partite prima della ricerca per consistenza
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
}

void remove_game_by_player_id(int id) {
    pthread_mutex_lock(&gameListLock);
    for (int i = 0; i < MAX_GAME; ++i) {
        if (Partite[i] && Partite[i]->giocatoreParticipante[0] && Partite[i]->giocatoreParticipante[0]->id == id) {
            for(int j = 0; j < MAX_GIOCATORI-1; j++) {
                if(Partite[i]->richieste[j]) {
                    free(Partite[i]->richieste[j]);
                }
            }
            
            //sem_destroy(&Partite[i]->semaforo);
            free(Partite[i]);
            Partite[i] = NULL;
            numero_partite--; // MODIFICA: Aggiunto decremento mancante
        }
    }
    pthread_mutex_unlock(&gameListLock);
}


void new_game(int*leave_flag,char*buffer,GIOCATORE*giocatore){
    GAME *nuova_partita = (GAME *)malloc(sizeof(GAME));
    
    // MODIFICA: Controllo malloc fallito prima di chiamare aggiungi_game_queue
    if(!nuova_partita){
        sendSuccessNewGame(0,giocatore, -1);
        return;
    }
    
    aggiungi_game_queue(nuova_partita,giocatore);
    
    // MODIFICA: Rimosso controllo ridondante, nuova_partita è sempre valida qui
    sendSuccessNewGame(1,giocatore, nuova_partita->id);
    printf("Partita creata con id: %d\n",nuova_partita->id);
    fflush(stdout);
    printf("Giocatore %s ha creato una partita\n",giocatore->nome);
}

void send_success_message(int success, int socket, const char* message){
    cJSON *response = cJSON_CreateObject();

    cJSON_AddNumberToObject(response, "success", success);
    cJSON_AddStringToObject(response, "message", message);
    char *json_string = cJSON_PrintUnformatted(response);
    if(json_string == NULL) {
        printf("Errore nella creazione della risposta JSON\n");
        return;
    }

    //invia risposta al client
    int bytes_sent = send(socket, json_string, strlen(json_string), 0);
    if(bytes_sent < 0) {
        printf("Errore nell'invio della risposta JSON\n");
    } else {
        printf("Risposta JSON inviata: %s\n", json_string);
    }
    cJSON_Delete(response); // Libera la memoria allocata per la risposta
    
}

void send_declined_request_message(int id_partita, GIOCATORE* giocatore) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "path", "/decline_request");
    cJSON_AddNumberToObject(response, "game_id", id_partita);
    cJSON_AddStringToObject(response, "message", "Richiesta rifiutata");

    char *msg = cJSON_PrintUnformatted(response);
    
    // Controllo se il socket è valido prima dell'invio
    if(giocatore->socket > 0) {
        send(giocatore->socket, msg, strlen(msg), 0);
        printf("Richiesta rifiutata inviata al giocatore %s\n", giocatore->nome);
    } else {
        printf("Socket del giocatore non valido\n");
    }
    
    free(msg);
    cJSON_Delete(response);
}

void aggiungi_richiesta(int id_partita, GIOCATORE* giocatore) {
    pthread_mutex_lock(&gameListLock);
    
    // Controllo se id_partita è valido
    if(id_partita < 0 || id_partita >= MAX_GAME || Partite[id_partita] == NULL) {
        printf("ID partita non valido: %d\n", id_partita);
        send_success_message(0, giocatore->socket, "Partita non trovata");
        pthread_mutex_unlock(&gameListLock);
        return;
    }

    GAME* partita = Partite[id_partita];
    
    // Controllo se il numero di richieste supera il massimo
    if(partita->numero_richieste > MAX_GIOCATORI-1) {
        printf("Numero massimo di richieste raggiunto per la partita %d\n", id_partita);
        send_success_message(0, giocatore->socket, "Numero massimo di richieste raggiunto");
        pthread_mutex_unlock(&gameListLock);
        return;
    }

    // Controllo se il giocatore ha già una richiesta in questa partita
    for(int i = 0; i < MAX_GIOCATORI-1; i++) {
        if(partita->richieste[i] && partita->richieste[i]->giocatore == giocatore) {
            printf("Il giocatore %s ha già una richiesta in questa partita\n", giocatore->nome);
            send_success_message(0, giocatore->socket, "Richiesta già esistente");
            pthread_mutex_unlock(&gameListLock);
            return;
        }
    }
    
    // Controllo se la partita è in corso
    if(partita->stato_partita == IN_CORSO) {
        printf("La partita %d è già in corso, non è possibile aggiungere richieste\n", id_partita);
        send_success_message(0, giocatore->socket, "Partita già in corso");
        pthread_mutex_unlock(&gameListLock);
        return;
    }
   
    RICHIESTA *richiesta = crea_richiesta(giocatore);
    bool slot_trovato = false;
    
    for(int i = 0; i < MAX_GIOCATORI-1; i++) {
        if(partita->richieste[i] == NULL) {
            partita->richieste[i] = richiesta;
            partita->numero_richieste++;
            send_success_message(1, giocatore->socket, "Richiesta aggiunta con successo");
            // Invia richiesta al proprietario della partita
            invia_richiesta_proprietario(partita, giocatore);
            slot_trovato = true;
            break;
        }
    }
    
    // Gestisce il caso in cui non si trova uno slot libero
    if(!slot_trovato) {
        printf("Impossibile aggiungere la richiesta: nessuno slot disponibile\n");
        send_success_message(0, giocatore->socket, "Errore interno: nessuno slot disponibile");
        elimina_richiesta(richiesta); // Libera la memoria
    }
    
    pthread_mutex_unlock(&gameListLock);
}

void invia_richiesta_proprietario(GAME* partita, GIOCATORE* giocatore_richiedente) {
    if(partita == NULL || giocatore_richiedente == NULL) {
        printf("Parametri non validi per l'invio della richiesta\n");
        return;
    }
    
    // Invia la richiesta al proprietario della partita
    if(partita->giocatoreParticipante[0] != NULL) {
        cJSON *request_json = cJSON_CreateObject();
        cJSON_AddStringToObject(request_json, "path", "/new_request");
        cJSON_AddNumberToObject(request_json, "game_id", partita->id);
        cJSON_AddNumberToObject(request_json, "player_id", giocatore_richiedente->id);
        cJSON_AddStringToObject(request_json, "player_name", giocatore_richiedente->nome);

        char *msg = cJSON_PrintUnformatted(request_json);
        
        //Controllo se il socket è valido prima dell'invio
        if(partita->giocatoreParticipante[0]->socket > 0) {
            send(partita->giocatoreParticipante[0]->socket, msg, strlen(msg), 0);
            printf("Richiesta inviata al proprietario della partita %d\n", partita->id);
        } else {
            printf("Socket del proprietario non valido\n");
        }
        
        free(msg);
        cJSON_Delete(request_json);
    } else {
        printf("Proprietario della partita non trovato\n");
    }
}

void rimuovi_richiesta(int id_partita, GIOCATORE* giocatore) {
    pthread_mutex_lock(&gameListLock);
    
    // Controllo se id_partita è valido
    if(id_partita < 0 || id_partita >= MAX_GAME || Partite[id_partita] == NULL) {
        printf("ID partita non valido: %d\n", id_partita);
        send_success_message(0, giocatore->socket, "Partita non trovata");
        pthread_mutex_unlock(&gameListLock);
        return;
    }

    GAME* partita = Partite[id_partita];
    
    // Trova e rimuovi la richiesta del giocatore
    bool richiesta_trovata = false;
    for(int i = 0; i < MAX_GIOCATORI-1; i++) {
        if(partita->richieste[i] && partita->richieste[i]->giocatore == giocatore) {
            elimina_richiesta(partita->richieste[i]);
            partita->richieste[i] = NULL;
            partita->numero_richieste--;
            printf("Richiesta rimossa per il giocatore %s dalla partita %d\n", giocatore->nome, id_partita);
            send_success_message(1, giocatore->socket, "Richiesta rimossa con successo");
            richiesta_trovata = true;

            //Invia messaggio al proprietario solo se esiste e ha socket valido
            if(partita->giocatoreParticipante[0] != NULL && 
               partita->giocatoreParticipante[0]->socket > 0) {
                cJSON *response_json = cJSON_CreateObject();
                cJSON_AddStringToObject(response_json, "path", "/remove_request");
                cJSON_AddNumberToObject(response_json, "game_id", id_partita);
                cJSON_AddNumberToObject(response_json, "player_id", giocatore->id);
                char *msg = cJSON_PrintUnformatted(response_json);
                
                send(partita->giocatoreParticipante[0]->socket, msg, strlen(msg), 0);
                
                //Libera la memoria allocata
                free(msg);
                cJSON_Delete(response_json);
            }
            break;
        }
    }
    
    if (!richiesta_trovata) {
        printf("Nessuna richiesta trovata per il giocatore %s nella partita %d\n", giocatore->nome, id_partita);
        send_success_message(0, giocatore->socket, "Richiesta non trovata");
    }
    
    pthread_mutex_unlock(&gameListLock);
}
// fin qui tutto va bene
// MODIFICA: Aggiunto parametro GIOCATORE per poter inviare risposta
void gestioneRichiestaJSONuscita(cJSON*json,int*leave_flag,int*leave_game,GIOCATORE*giocatore){
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
            // MODIFICA: Aggiunto else if per evitare controlli multipli
            else if(strcmp(path->valuestring, "/left_game") == 0) {
                printf("Richiesta di uscita dalla partita ricevuta\n");
                *leave_game=1;
                // MODIFICA: Invia risposta di conferma al client
                if(giocatore) {
                    cJSON *response = cJSON_CreateObject();
                    cJSON_AddNumberToObject(response, "success", 1);
                    cJSON_AddStringToObject(response, "message", "Uscita dalla partita confermata");
                    char *msg = cJSON_PrintUnformatted(response);
                    send(giocatore->socket, msg, strlen(msg), 0);
                    cJSON_Delete(response);
                    free(msg);
                }
            }
        }
        cJSON_Delete(json);
    }
} 




void sendSuccessNewGame(int success, GIOCATORE*giocatore, int id_partita){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "success", success);
    cJSON_AddNumberToObject(root, "id_partita", id_partita);
    char *msg = cJSON_PrintUnformatted(root);
    send(giocatore->socket, msg, strlen(msg), 0);
    cJSON_Delete(root);
    free(msg);
}


bool aggiorna_partita(GAME*partita,GIOCATORE*giocatore,int col,int row){
    printf("Aggiornamento partita con id: %d, giocatore: %s, colonna: %d, riga: %d\n", partita->id, giocatore->nome, col, row);
    fflush(stdout);
    sem_wait(&(partita->semaforo));
    int turno = partita->turno;
    if(partita->giocatoreParticipante[turno]!=giocatore){
        printf("Il giocatore %s ha tentato di aggiornare la partita ma non è il suo turno\n", giocatore->nome);
        fflush(stdout);
        send_success_message(0,giocatore->socket,"non è il tuo turno");
        sem_post(&(partita->semaforo));
        return false;
    }else{
        bool success = aggiorna_griglia(partita,giocatore,col,row,turno);
        if(success){
            Esito esito = verifica_esito_partita(partita,turno+1);
            GIOCATORE*giocatore2= partita->giocatoreParticipante[switchGiocatore(turno+1)-1];
            if(esito!=NESSUN_ESITO){
                InviaEsito(partita,esito,giocatore);
                InviaEsito(partita,switchEsito(esito),giocatore2);
                sem_post(&(partita->semaforo));
                return false;
            }else{
                //handler_game_response(giocatore2,partita);
                switchTurn(partita);
            }
        }
        else
        {
           send_success_message(0,giocatore->socket,"casella già occupata");
           sem_post(&(partita->semaforo));
           return false; 
        }

    }
    sem_post(&(partita->semaforo));
    return true;
}

void  switchTurn(GAME*partita){
    if(partita->turno == 0) {
        partita->turno = 1; // Passa al giocatore 1
    } else {
        partita->turno = 0; // Passa al giocatore 0
    }
    printf("Turno cambiato a: %d\n", partita->turno);
    fflush(stdout);
}

bool aggiorna_griglia(GAME*partita,GIOCATORE*giocatore,int col,int row,int turno){
    // Rimuovo il mutex qui perché il semaforo della partita già protegge l'accesso
    if(partita->griglia[row][col]!=VUOTO){
        return false;
    }else{
        partita->griglia[row][col]=turno+1; //turno+1 perché il turno è 0 o 1, ma la griglia è 1 o 2
        printf("casella aggiornata con successo");
        return true;
    }

}

Esito verifica_esito_partita(GAME*partita,int giocatore){
    if(controlla_vittoria(partita,giocatore))
        return VITTORIA;
    else if(controlla_vittoria(partita,switchGiocatore(giocatore)))
        return SCONFITTA;
    else if(controlla_pareggio(partita))
        return PAREGGIO;

    return NESSUN_ESITO;
}

int switchGiocatore(int giocatore){
     if(giocatore == 1) {
        return 2; 
    } else {
       return 1; 
    }
}

Esito switchEsito(Esito esito){
    if(esito==VITTORIA)
        return SCONFITTA;
    else if(esito==SCONFITTA)
        return VITTORIA;
    else
        return PAREGGIO;
}

bool controlla_vittoria(GAME*partita,int giocatore){   
    TRIS(*matrice)[3]=partita->griglia;
    
for (int i = 0; i < 3; i++) {
    if ((matrice[i][0] == giocatore && matrice[i][1] == giocatore && matrice[i][2] == giocatore) ||
        (matrice[0][i] == giocatore && matrice[1][i] == giocatore && matrice[2][i] == giocatore)) {
        return true; // Vittoria
    }
}
// Diagonali
if ((matrice[0][0] == giocatore && matrice[1][1] == giocatore && matrice[2][2] == giocatore) ||
    (matrice[0][2] == giocatore && matrice[1][1] == giocatore && matrice[2][0] == giocatore)) {
    return true; // Vittoria
}

return false; // Nessuna vittoria
}

bool controlla_pareggio(GAME*partita){
    TRIS (*matrice)[3]=partita->griglia;
        for(int i=0;i<3;i++)
            for(int j=0;j<3;j++)
                if(matrice[i][j]==VUOTO)
                    return false;
 
    return true;

}

void InviaEsito(GAME* partita,Esito esito,GIOCATORE*giocatore){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "/game_response");//type esito_game
    cJSON_AddNumberToObject(root, "game_id", partita->id);
    //passo l'esito della partita
    cJSON_AddNumberToObject(root, "esito", esito);
    if(esito==VITTORIA)
        cJSON_AddStringToObject(root, "messaggio", "vuoi continuare?");
    else if(esito==PAREGGIO)
        cJSON_AddStringToObject(root, "messaggio", "rivincita?");

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
    cJSON_Delete(root); 
    printf("Invio griglia Esito partita al socket %d: %s\n", giocatore->socket, json_str);
    fflush(stdout);
    send(giocatore->socket,json_str,strlen(json_str),0);
    free(json_str);

    gestisci_esito_vittoria(giocatore,esito,partita);

}

void InviaVittoriaAltroGiocatore(GIOCATORE*giocatore){
    GAME* partita = SearchPartitaInCorsoByGiocatore(giocatore);
    if(giocatore==partita->giocatoreParticipante[0]){
        InviaEsito(partita,VITTORIA,partita->giocatoreParticipante[1]);
    }
    else{
        InviaEsito(partita,VITTORIA,partita->giocatoreParticipante[0]);
    }
        
}

GAME*SearchPartitaInCorsoByGiocatore(GIOCATORE*giocatore){
    pthread_mutex_lock(&gameListLock);
    for(int i=0;i< MAX_GAME ; i++){
        if((Partite[i]->giocatoreParticipante[0] == giocatore || Partite[i]->giocatoreParticipante[1] == giocatore ) && Partite[i]->stato_partita==IN_CORSO){
            pthread_mutex_unlock(&gameListLock);
            return Partite[i];
        }
    }
    pthread_mutex_unlock(&gameListLock);
    return NULL;
}

void gestisci_esito_vittoria(GIOCATORE*giocatore,Esito esito,GAME*partita){

    if(esito==VITTORIA){
        partita->giocatoreParticipante[0]->stato=IN_HOME;
        partita->giocatoreParticipante[1]->stato=IN_HOME;
        partita->giocatoreParticipante[0] = giocatore;
        partita->giocatoreParticipante[1]=NULL;
    }
    /*if(esito==SCONFITTA){}*/
    if(esito==PAREGGIO){
        partita->voti_pareggio=0;
        giocatore->stato=IN_HOME;
    }

}

void resetGame(GAME*partita){
            partita->numero_richieste = 0; // MODIFICA: Inizializza numero_richieste
            
            // MODIFICA: Inizializza tutto l'array richieste a NULL
            for(int j = 0; j < MAX_GIOCATORI-1; j++) {
                partita->richieste[j] = NULL;
            }
            
            // MODIFICA: Inizializza la griglia TRIS a VUOTO 
            for(int row = 0; row < 3; row++) {
                for(int col = 0; col < 3; col++) {
                    partita->griglia[row][col] = VUOTO; // Inizializza ogni cella a VUOTO (0)
                }
            }
            
            partita->turno=0;
            partita->esito= NESSUN_ESITO;
            partita->stato_partita = IN_ATTESA;
            partita->voti_pareggio = 0; // MODIFICA: Inizializza voti_pareggio 
}


void GestionePareggioGame(GAME* partita, GIOCATORE* giocatore, bool risposta) {
    if (!partita) return;
    
    pthread_mutex_lock(&gameListLock);
    
    // Registra il voto del giocatore
    if (risposta) {
        partita->voti_pareggio++;
        pthread_mutex_unlock(&gameListLock);

        printf("Giocatore %s ha accettato la rivincita. Voti: %d/2\n", giocatore->nome, partita->voti_pareggio);
    } else {
        printf("Giocatore %s ha rifiutato la rivincita\n", giocatore->nome);
        // Se qualcuno rifiuta, termina immediatamente
        
        partita->voti_pareggio = -1; // Segna come rifiutato
        
        // CORREZIONE SEGFAULT: Salva i puntatori dei giocatori prima di deallocare
        GIOCATORE* giocatore0 = partita->giocatoreParticipante[0];
        GIOCATORE* giocatore1 = partita->giocatoreParticipante[1];
        
        // Imposta lo stato dei giocatori prima di deallocare la partita
        if (giocatore0) giocatore0->stato = IN_HOME;
        if (giocatore1) giocatore1->stato = IN_HOME;
        
        // Rimuovi la partita prima di sbloccare il mutex
        rimuovi_game_queue(partita);
        pthread_mutex_unlock(&gameListLock);
        
        // Invia messaggio a entrambi i giocatori DOPO aver deallocato la partita
        inviaMessaggioRivincita(giocatore0, 0, NULL);
        inviaMessaggioRivincita(giocatore1, 0, NULL);
        
        return;
    }
    
    // Controlla se entrambi hanno votato
    if (partita->voti_pareggio == 2) {
        // Entrambi accettano - inizia nuova partita
        printf("Entrambi i giocatori hanno accettato la rivincita\n");
        
        partita->giocatoreParticipante[0]->stato = IN_GIOCO;
        partita->giocatoreParticipante[1]->stato = IN_GIOCO;
        partita->stato_partita = IN_CORSO;
        resetGame(partita);
        // Invia conferma a entrambi i giocatori
        inviaMessaggioRivincita(partita->giocatoreParticipante[0], 1, partita);
        inviaMessaggioRivincita(partita->giocatoreParticipante[1], 1, partita);
        pthread_mutex_unlock(&gameListLock); 
    }
    
}



void inviaMessaggioRivincita(GIOCATORE *giocatore,int risposta,GAME*partita){
    
    if (!giocatore || giocatore->socket <= 0) {
        printf("Giocatore non valido per invio messaggio rivincita\n");
        return;
    }
    
    if(risposta == 1){
        // Rivincita accettata
        // {path: "/game_pareggio", success: 1, game_id:1, message:..., nome_partecipante:...}
        if(partita){
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "path", "/game_pareggio");
            cJSON_AddNumberToObject(root, "success", 1);
            cJSON_AddNumberToObject(root, "game_id", partita->id);
            if(giocatore->id == partita->giocatoreParticipante[0]->id) {
                cJSON_AddStringToObject(root, "nome_partecipante", partita->giocatoreParticipante[1]->nome);
            } else {
                cJSON_AddStringToObject(root, "nome_partecipante", partita->giocatoreParticipante[0]->nome);
            }
            
            cJSON_AddStringToObject(root, "message", "Rivincita accettata, inizio nuova partita");
            char *msg = cJSON_PrintUnformatted(root);
            sleep(0.1);
            send(giocatore->socket, msg, strlen(msg), 0);
            cJSON_Delete(root);
            free(msg);
            sleep(0.1);
            resetGame(partita); // Reset della partita per la nuova sessione
            handler_game_response(giocatore,partita);
        }
    } else {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "path", "/game_pareggio");
        cJSON_AddNumberToObject(root, "success", 0);
        cJSON_AddNumberToObject(root, "game_id", -1);
        cJSON_AddStringToObject(root, "message", "Rivincita rifiutata, torna in home");
        char *msg = cJSON_PrintUnformatted(root);
        send(giocatore->socket, msg, strlen(msg), 0);
        cJSON_Delete(root);
        free(msg);
    } 
}

//turno = 0;

/*risposta 1  risposta 2
    si          si    => turno 2 (1+1)
    no          si    => turno 0 (1-1)
    si          no    => turno 0 (1-1)
    no          no    => turno-2 (-1-1)
                no    => turno -1
                si    => turno  1             */