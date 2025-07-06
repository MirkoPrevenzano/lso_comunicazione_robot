#include "game.h"
#include "server.h"

// Dichiarazione extern per funzioni handler
extern void handlerInviaMossePartita(GIOCATORE* giocatore, GIOCO* partita);

void aggiungiGiocoQueue(GIOCO *nuova_partita,GIOCATORE* giocatoreProprietario){

    // MODIFICA: Controllo parametri null prima di procedere

    if(!giocatoreProprietario) {
        return;
    }
    if (!nuova_partita) {
        // Non usare giocatoreProprietario se è NULL
        return;
    }

    


    // MODIFICA: Controllo numero partite ora protetto dal mutex
    if(numeroPartite>=MAX_GAME){
        inviaMessaggioSuccesso(0, giocatoreProprietario->socket, "Numero massimo di partite raggiunto");
        fprintf(stderr, "numero max partite superato\n");
        return;
    }


    int i;
    bool trovato = false;
    for( i=0; i < MAX_GAME; i++){
        if(!Partite[i] && !trovato){
            Partite[i]=nuova_partita;
            Partite[i]->giocatoreParticipante[0] = giocatoreProprietario;
            Partite[i]->id=i;
            Partite[i]->numeroRichieste = 0; // MODIFICA: Inizializza numeroRichieste
            
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
            Partite[i]->statoPartita = IN_ATTESA;
            Partite[i]->votiPareggio = 0; // MODIFICA: Inizializza votiPareggio 
            trovato = true;
            
        }
    }
    numeroPartite++;
    printf("Partita creata con id: %d indice %d\n",nuova_partita->id, i);
    inviaMessaggioSuccessoNuovaPartita(1,giocatoreProprietario,nuova_partita->id);
    fflush(stdout);
};


void rimuoviGiocoQueue(GIOCO*partita){

    if(partita!=NULL){
        // MODIFICA: Decremento numero_partite prima della ricerca per consistenza
        numeroPartite--;
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

void rimuoviGiocoByIdGiocatore(int id) {
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
            numeroPartite--; // MODIFICA: Aggiunto decremento mancante
        }
    }
    pthread_mutex_unlock(&gameListLock);
}


void nuovaPartita(char*buffer,GIOCATORE*giocatore){
    GIOCO *nuova_partita = (GIOCO *)malloc(sizeof(GIOCO));
    
    // MODIFICA: Controllo malloc fallito prima di chiamare aggiungi_game_queue
    if(!nuova_partita){
        inviaMessaggioSuccessoNuovaPartita(0,giocatore, -1);
        return;
    }
    
    aggiungiGiocoQueue(nuova_partita,giocatore);
    
    
    printf("Partita creata con id: %d\n",nuova_partita->id);
    fflush(stdout);
    printf("Giocatore %s ha creato una partita\n",giocatore->nome);
}

void inviaMessaggioSuccesso(int success, int socket, const char* message){
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
    cJSON_Delete(response); // Libera la memoria allocata per la rispostaù
    free(json_string); // Libera la stringa JSON
    
    
}

void inviaMessaggioRichiestaRifiutata(int id_partita, GIOCATORE* giocatore) {
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

void aggiungiRichiesta(int id_partita, GIOCATORE* giocatore) {
    pthread_mutex_lock(&gameListLock);
    
    // Controllo se id_partita è valido
    if(id_partita < 0 || id_partita >= MAX_GAME || Partite[id_partita] == NULL) {
        printf("ID partita non valido: %d\n", id_partita);
        inviaMessaggioSuccesso(0, giocatore->socket, "Partita non trovata");
        pthread_mutex_unlock(&gameListLock);
        return;
    }

    GIOCO* partita = Partite[id_partita];
    
    // Controllo se il numero di richieste supera il massimo
    if(partita->numeroRichieste > MAX_GIOCATORI-1) {
        printf("Numero massimo di richieste raggiunto per la partita %d\n", id_partita);
        inviaMessaggioSuccesso(0, giocatore->socket, "Numero massimo di richieste raggiunto");
        pthread_mutex_unlock(&gameListLock);
        return;
    }

    // Controllo se il giocatore ha già una richiesta in questa partita
    for(int i = 0; i < MAX_GIOCATORI-1; i++) {
        if(partita->richieste[i] && partita->richieste[i]->giocatore == giocatore) {
            printf("Il giocatore %s ha già una richiesta in questa partita\n", giocatore->nome);
            inviaMessaggioSuccesso(0, giocatore->socket, "Richiesta già esistente");
            pthread_mutex_unlock(&gameListLock);
            return;
        }
    }
    
    // Controllo se la partita è in corso
    if(partita->statoPartita == IN_CORSO) {
        printf("La partita %d è già in corso, non è possibile aggiungere richieste\n", id_partita);
        inviaMessaggioSuccesso(0, giocatore->socket, "Partita già in corso");
        pthread_mutex_unlock(&gameListLock);
        return;
    }
   
    RICHIESTA *richiesta = creaRichiesta(giocatore);
    bool slot_trovato = false;
    
    for(int i = 0; i < MAX_GIOCATORI-1; i++) {
        if(partita->richieste[i] == NULL) {
            partita->richieste[i] = richiesta;
            partita->numeroRichieste++;
            inviaMessaggioSuccesso(1, giocatore->socket, "Richiesta aggiunta con successo");
            // Invia richiesta al proprietario della partita
            inviaRichiestaProprietario(partita, giocatore);
            slot_trovato = true;
            break;
        }
    }
    
    // Gestisce il caso in cui non si trova uno slot libero
    if(!slot_trovato) {
        printf("Impossibile aggiungere la richiesta: nessuno slot disponibile\n");
        inviaMessaggioSuccesso(0, giocatore->socket, "Errore interno: nessuno slot disponibile");
        eliminaRichiesta(richiesta); // Libera la memoria
    }
    
    pthread_mutex_unlock(&gameListLock);
}

void inviaRichiestaProprietario(GIOCO* partita, GIOCATORE* giocatore_richiedente) {
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

void rimuoviRichiesta(int id_partita, GIOCATORE* giocatore) {
    pthread_mutex_lock(&gameListLock);
    
    // Controllo se id_partita è valido
    if(id_partita < 0 || id_partita >= MAX_GAME || Partite[id_partita] == NULL) {
        printf("ID partita non valido: %d\n", id_partita);
        inviaMessaggioSuccesso(0, giocatore->socket, "Partita non trovata");
        pthread_mutex_unlock(&gameListLock);
        return;
    }

    GIOCO* partita = Partite[id_partita];
    
    // Trova e rimuovi la richiesta del giocatore
    bool richiesta_trovata = false;
    for(int i = 0; i < MAX_GIOCATORI-1; i++) {
        if(partita->richieste[i] && partita->richieste[i]->giocatore == giocatore) {
            eliminaRichiesta(partita->richieste[i]);
            partita->richieste[i] = NULL;
            partita->numeroRichieste--;
            printf("Richiesta rimossa per il giocatore %s dalla partita %d\n", giocatore->nome, id_partita);
            inviaMessaggioSuccesso(1, giocatore->socket, "Richiesta rimossa con successo");
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
        inviaMessaggioSuccesso(0, giocatore->socket, "Richiesta non trovata");
    }
    
    pthread_mutex_unlock(&gameListLock);
}






void inviaMessaggioSuccessoNuovaPartita(int success, GIOCATORE*giocatore, int id_partita){
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "success", success);
    cJSON_AddNumberToObject(root, "id_partita", id_partita);
    char *msg = cJSON_PrintUnformatted(root);
    send(giocatore->socket, msg, strlen(msg), 0);
    cJSON_Delete(root);
    free(msg);
}


bool aggiornaPartita(GIOCO*partita,GIOCATORE*giocatore,int col,int row){
    printf("Aggiornamento partita con id: %d, giocatore: %s, colonna: %d, riga: %d\n", partita->id, giocatore->nome, col, row);
    fflush(stdout);
    sem_wait(&(partita->semaforo));
    int turno = partita->turno;
    if(partita->giocatoreParticipante[turno]!=giocatore){
        printf("Il giocatore %s ha tentato di aggiornare la partita ma non è il suo turno\n", giocatore->nome);
        fflush(stdout);
        inviaMessaggioSuccesso(0,giocatore->socket,"non è il tuo turno");
        sem_post(&(partita->semaforo));
        return false;
    }else{
        bool success = aggiornaGriglia(partita,giocatore,col,row,turno);
        if(success){
            Esito esito = verificaEsitoPartita(partita,turno+1);
            GIOCATORE*giocatore2= partita->giocatoreParticipante[switchGiocatore(turno+1)-1];
            if(esito!=NESSUN_ESITO){
                inviaEsito(partita,esito,giocatore);
                inviaEsito(partita,switchEsito(esito),giocatore2);
                sem_post(&(partita->semaforo));
                return false;
            }else{
                //handler_game_response(giocatore2,partita);
                switchTurn(partita);
            }
        }
        else
        {
           inviaMessaggioSuccesso(0,giocatore->socket,"casella già occupata");
           sem_post(&(partita->semaforo));
           return false; 
        }

    }
    sem_post(&(partita->semaforo));
    return true;
}

void switchTurn(GIOCO*partita){
    if(partita->turno == 0) {
        partita->turno = 1; // Passa al giocatore 1
    } else {
        partita->turno = 0; // Passa al giocatore 0
    }
    printf("Turno cambiato a: %d\n", partita->turno);
    fflush(stdout);
}

bool aggiornaGriglia(GIOCO*partita,GIOCATORE*giocatore,int col,int row,int turno){
    // Rimuovo il mutex qui perché il semaforo della partita già protegge l'accesso
    if(partita->griglia[row][col]!=VUOTO){
        return false;
    }else{
        partita->griglia[row][col]=turno+1; //turno+1 perché il turno è 0 o 1, ma la griglia è 1 o 2
        printf("casella aggiornata con successo");
        return true;
    }

}

Esito verificaEsitoPartita(GIOCO*partita,int giocatore){
    if(controllaVittoria(partita,giocatore))
        return VITTORIA;
    else if(controllaVittoria(partita,switchGiocatore(giocatore)))
        return SCONFITTA;
    else if(controllaPareggio(partita))
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

bool controllaVittoria(GIOCO*partita,int giocatore){   
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

bool controllaPareggio(GIOCO*partita){
    TRIS (*matrice)[3]=partita->griglia;
        for(int i=0;i<3;i++)
            for(int j=0;j<3;j++)
                if(matrice[i][j]==VUOTO)
                    return false;
 
    return true;

}

void inviaEsito(GIOCO* partita,Esito esito,GIOCATORE*giocatore){
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

    gestisciEsitoVittoria(giocatore,esito,partita);

}

void inviaVittoriaAltroGiocatore(GIOCATORE*giocatore){
    GIOCO* partita = cercaPartitaInCorsoByGiocatore(giocatore);
    if(giocatore==partita->giocatoreParticipante[0]){
        inviaEsito(partita,VITTORIA,partita->giocatoreParticipante[1]);
    }
    else{
        inviaEsito(partita,VITTORIA,partita->giocatoreParticipante[0]);
    }
        
}

GIOCO*cercaPartitaInCorsoByGiocatore(GIOCATORE*giocatore){
    pthread_mutex_lock(&gameListLock);
    for(int i=0;i< MAX_GAME ; i++){
        if(Partite[i] && (Partite[i]->giocatoreParticipante[0] == giocatore || Partite[i]->giocatoreParticipante[1] == giocatore ) && Partite[i]->statoPartita==IN_CORSO){
            pthread_mutex_unlock(&gameListLock);
            printf("Partita trovata per il giocatore %s con id: %d\n", giocatore->nome, Partite[i]->id);
            fflush(stdout);
            return Partite[i];
        }
    }
    pthread_mutex_unlock(&gameListLock);
    return NULL;
}

void gestisciEsitoVittoria(GIOCATORE*giocatore,Esito esito,GIOCO*partita){

    if(esito==VITTORIA){
        partita->giocatoreParticipante[0]->stato=IN_HOME;
        partita->giocatoreParticipante[1]->stato=IN_HOME;
        partita->giocatoreParticipante[0] = giocatore;
        partita->giocatoreParticipante[1]=NULL;
    }
    /*if(esito==SCONFITTA){}*/
    if(esito==PAREGGIO){
        partita->votiPareggio=0;
        giocatore->stato=IN_HOME;
    }

}

void resetGioco(GIOCO*partita){
    partita->numeroRichieste = 0; // MODIFICA: Inizializza numeroRichieste
    
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
    partita->statoPartita = IN_ATTESA;
    partita->votiPareggio = 0; // MODIFICA: Inizializza votiPareggio 
}


void gestionePareggioGioco(GIOCO* partita, GIOCATORE* giocatore, bool risposta) {
    if (!partita) return;
    
    pthread_mutex_lock(&gameListLock);
    
    // Registra il voto del giocatore
    if (risposta) {
        partita->votiPareggio++;
        
        // Invia conferma immediata al giocatore che ha appena votato
        inviaMessaggioSuccesso(1, giocatore->socket, "Richiesta di rivincita ricevuta");
        
        pthread_mutex_unlock(&gameListLock);

        printf("Giocatore %s ha accettato la rivincita. Voti: %d/2\n", giocatore->nome, partita->votiPareggio);
    } else {
        printf("Giocatore %s ha rifiutato la rivincita\n", giocatore->nome);
        // Se qualcuno rifiuta, termina immediatamente
        
        partita->votiPareggio = -1; // Segna come rifiutato
        
        // CORREZIONE SEGFAULT: Salva i puntatori dei giocatori prima di deallocare
        GIOCATORE* giocatore0 = partita->giocatoreParticipante[0];
        GIOCATORE* giocatore1 = partita->giocatoreParticipante[1];
        
        // Imposta lo stato dei giocatori prima di deallocare la partita
        if (giocatore0) giocatore0->stato = IN_HOME;
        if (giocatore1) giocatore1->stato = IN_HOME;
        
        // Rimuovi la partita prima di sbloccare il mutex
        rimuoviGiocoQueue(partita);
        
        // Invia messaggio a entrambi i giocatori DOPO aver deallocato la partita
        inviaMessaggioRivincita(giocatore0, 0, NULL);
        inviaMessaggioRivincita(giocatore1, 0, NULL);
        pthread_mutex_unlock(&gameListLock);

   
        return;
    }
    
    // Controlla se entrambi hanno votato
    if (partita->votiPareggio == 2) {
        // Entrambi accettano - inizia nuova partita
        printf("Entrambi i giocatori hanno accettato la rivincita\n");
        
        partita->giocatoreParticipante[0]->stato = IN_GIOCO;
        partita->giocatoreParticipante[1]->stato = IN_GIOCO;
        resetGioco(partita);
        partita->statoPartita = IN_CORSO;
        // Invia conferma a entrambi i giocatori
        inviaMessaggioRivincita(partita->giocatoreParticipante[0], 1, partita);
        inviaMessaggioRivincita(partita->giocatoreParticipante[1], 1, partita);
        pthread_mutex_unlock(&gameListLock); 
    }
    
}



void inviaMessaggioRivincita(GIOCATORE *giocatore,int risposta,GIOCO*partita){
    
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
                cJSON_AddStringToObject(root, "simbolo", "X");
                cJSON_AddStringToObject(root, "nome_partecipante", partita->giocatoreParticipante[1]->nome);
            } else {
                cJSON_AddStringToObject(root, "simbolo", "O");
                cJSON_AddStringToObject(root, "nome_partecipante", partita->giocatoreParticipante[0]->nome);
            }
            
            cJSON_AddStringToObject(root, "message", "Rivincita accettata, inizio nuova partita");
            char *msg = cJSON_PrintUnformatted(root);
            sleep(1);
            send(giocatore->socket, msg, strlen(msg), 0);
            cJSON_Delete(root);
            free(msg);
            sleep(1);
            pthread_mutex_lock(&gameListLock);
            handlerInviaMossePartita(giocatore,partita);
            pthread_mutex_unlock(&gameListLock);
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

void inviaPareggioDisconnessione(GIOCATORE*giocatore){
    GIOCO*partita = cercaPartitaInCorsoByGiocatore(giocatore);
    if(partita){
        printf("⚠️ Client %s è uscito-> ha rifiutato la rivincità\n", giocatore->nome);
        gestionePareggioGioco(partita,giocatore,0);
    }
}