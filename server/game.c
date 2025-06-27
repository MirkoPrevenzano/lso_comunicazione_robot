#include "game.h"
#include "server.h"

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
            
            //a cosa serve? il semaforo serve per sincronizzare l'accesso alla partita perchè può essere accessibile da più thread
            //con sem_init si inizializza il semaforo uno ad uno i parametri sono: il semaforo, il valore iniziale (0 o 1), e il valore di condivisione (0 per processo, 1 per thread)
            sem_init(&Partite[i]->semaforo, 0, 1); // MODIFICA: Inizializza semaforo
            Partite[i]->turno=0;
            Partite[i]->esito= NESSUN_ESITO;
            Partite[i]->stato_partita = IN_ATTESA; 
            trovato = true;
            
        }
    }
    numero_partite++;
    printf("Partita creata con id: %d indice %d\n",nuova_partita->id, i);
    fflush(stdout);
};


void rimuovi_game_queue(GAME*partita){

    pthread_mutex_lock(&gameListLock);
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
    pthread_mutex_unlock(&gameListLock);
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
        // MODIFICA: Corretto bug sizeof(buffer) -> len-1 per evitare buffer overflow
        memset(buffer, 0, len);
        int size = read(sockfd, buffer, len-1);
        if(size > 0){
            buffer[size] = '\0';
            printf("Buffer ricevuto: %s\n", buffer);
            fflush(stdout);
            // MODIFICA: Cerca la fine del JSON e tronca la stringa se necessario
            
            cJSON *json = cJSON_Parse(buffer);
            if (json == NULL) {
                printf("Errore nel parser JSON\n");
                *leave_game = 1;
            } else 
                return json;        
        }
        // MODIFICA: Aggiunta gestione caso size <= 0
        else {
            printf("Errore lettura socket o connessione chiusa\n");
            *leave_game = 1;
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
    send(partita->giocatoreParticipante[0]->socket, msg, strlen(msg), 0);
    cJSON_Delete(root);
    free(msg);
    printf("Inviato messaggio di unione partita al giocatore 1");
    fflush(stdout);
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


void aggiorna_partita(GAME*partita,GIOCATORE*giocatore,int col,int row){
    printf("Aggiornamento partita con id: %d, giocatore: %s, colonna: %d, riga: %d\n", partita->id, giocatore->nome, col, row);
    fflush(stdout);
    sem_wait(&(partita->semaforo));
    int turno = partita->turno;
    if(partita->giocatoreParticipante[turno]!=giocatore){
        printf("Il giocatore %s ha tentato di aggiornare la partita ma non è il suo turno\n", giocatore->nome);
        fflush(stdout);
        send_success_message(0,giocatore->socket,"non è il tuo turno");
        sem_post(&(partita->semaforo));
        return;
    }else{
        bool success = aggiorna_griglia(partita,giocatore,col,row,turno);
        if(success){}
            Esito esito= verifica_esito_partita(partita);
            if(esito!=NESSUN_ESITO)
                InviaEsito(partita,esito);
            else
                switchTurn(partita);
    }
    sem_post(&(partita->semaforo));
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
    if(partita->griglia[col][row]!=0){
        send_success_message(0,giocatore->socket,"casella già occupata");
        return false;
    }else{
        partita->griglia[col][row]=turno+1; //turno+1 perché il turno è 0 o 1, ma la griglia è 1 o 2
        printf("casella aggiornata con successo");
        send_success_message(1,giocatore->socket,"casella aggiornata con successo");
        return true;
    }

}

Esito verifica_esito_partita(GAME*partita){
    return NESSUN_ESITO;
}
void InviaEsito(GAME* partita,Esito esito){

}