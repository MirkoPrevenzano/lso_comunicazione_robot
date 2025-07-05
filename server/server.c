#include "./server.h"
#include "./handler.h"

int numeroConnessioni = 0;
int numeroPartite = 0;

GIOCO* Partite[MAX_GAME] = { NULL };
GIOCATORE* Giocatori[MAX_GIOCATORI] = { NULL };

pthread_mutex_t playerListLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gameListLock = PTHREAD_MUTEX_INITIALIZER;

char *msg = "numero max di giocatori raggiunto.";
char *msg1 = "errore";
char *msg2 = "disconnesione";


//ogni gioco deve sempre avere un proprietario in qualsiasi momento
//GIOCATORE* Giocatori[MAX_GIOCATORI]; verrà trattata come una pseudo-coda.

GIOCATORE * cercaGiocatoreById(int id_player){
    pthread_mutex_lock(&playerListLock);
    for(int i = 0 ; i<MAX_GIOCATORI;i++){
        if(Giocatori[i] && Giocatori[i]->id==id_player){
            pthread_mutex_unlock(&playerListLock);
            return Giocatori[i];
        }
    }
            
    pthread_mutex_unlock(&playerListLock);
    return NULL;
}

bool queueAggiunta(GIOCATORE* nuovoGiocatore){
    //aggiunto controllo per evitare che giocatore_add sia NULL
    if (!nuovoGiocatore) {
        printf("Errore: giocatore_add è NULL\n");
        return false;
    }
    
	pthread_mutex_lock(&playerListLock);
    bool trovato = false;
	for(int i=0; i < MAX_GIOCATORI; ++i){
		if(!Giocatori[i] && !trovato){
            trovato = true;
            numeroConnessioni++;
			Giocatori[i] = nuovoGiocatore;
            Giocatori[i]->id = i;
            Giocatori[i]->stato = IN_HOME; 
		}
	}

	pthread_mutex_unlock(&playerListLock);
    return trovato;
}

//l'utilizzo di due mutex può causare deadlock  se in un altro thread si cerca di accedere a Giocatori e Partite contemporaneamente
//uso playerListLock anche per numero_connessioni
void queueRimozione(int id){
    bool trovato = false;
	for(int i=0; i < MAX_GIOCATORI; ++i){
		if(Giocatori[i] && !trovato){
			if(Giocatori[i]->id == id){
                Giocatori[i] = NULL;
                trovato = true;
			}
		}
	}
    if(trovato){
        numeroConnessioni--;
        printf("Connessione chiusa, numero connessioni: %d\n", numeroConnessioni);
        fflush(stdout);
    }
    else
        printf("Giocatore %d non trovato\n", id);


}


void *handleClient(void *arg) {
    char buffer[BUFFER_SIZE];
    int leave_flag = 0;
    int *socket_ptr = (int *)arg;
    int socket_fd = *socket_ptr;
    
    // libero il socket pointer
    free(socket_ptr);
    
    printf("Nuovo client connesso\n");
    fflush(stdout);

    GIOCATORE *nuovoGiocatore = malloc(sizeof(GIOCATORE));
    if (!nuovoGiocatore) {
        fprintf(stderr, "Errore allocazione memoria giocatore\n");
        close(socket_fd);
        pthread_exit(NULL);
    }

    // Inizializza il nuovo giocatore
    memset(nuovoGiocatore, 0, sizeof(GIOCATORE));
    nuovoGiocatore->socket = socket_fd;
    nuovoGiocatore->id = -1; // ID non valido inizialmente

    int size = read(socket_fd, buffer, sizeof(buffer) - 1);
    if (size <= 0) {
        fprintf(stderr, "Errore lettura dal socket o connessione chiusa\n");
        leave_flag = 1;
    } else {
        buffer[size] = '\0';
        printf("Buffer ricevuto: %s\n", buffer);

        cJSON *json = cJSON_Parse(buffer);
        if (!json) {
            fprintf(stderr, "Errore parsing JSON\n");
            leave_flag = 1;
        } else {
            cJSON *path = cJSON_GetObjectItem(json, "path");
            cJSON *body = cJSON_GetObjectItem(json, "body");

            if (!cJSON_IsString(path)) {
                fprintf(stderr, "Campo 'path' mancante o non valido\n");
                leave_flag = 1;
            } else if (strcmp(path->valuestring, "/register") != 0) {
                fprintf(stderr, "Path non gestito: %s\n", path->valuestring);
                leave_flag = 1;
            } else if (!cJSON_IsString(body)) {
                fprintf(stderr, "Campo 'body' mancante o non è una stringa\n");
                leave_flag = 1;
            } else {
                // Parsing JSON annidato
                cJSON *body_json = cJSON_Parse(body->valuestring);
                if (!body_json) {
                    fprintf(stderr, "Errore nel parsing del JSON annidato\n");
                    leave_flag = 1;
                } else {
                    cJSON *nickname = cJSON_GetObjectItem(body_json, "nickname");
                    if (!cJSON_IsString(nickname)) {
                        fprintf(stderr, "Campo 'nickname' mancante o non valido\n");
                        leave_flag = 1;
                    } else {
                        strncpy(nuovoGiocatore->nome, nickname->valuestring, sizeof(nuovoGiocatore->nome) - 1);
                        nuovoGiocatore->nome[sizeof(nuovoGiocatore->nome) - 1] = '\0';
                        printf("Giocatore %s registrato\n", nuovoGiocatore->nome);
                    }
                    cJSON_Delete(body_json);
                }
            }
            cJSON_Delete(json);
        }

        if (!leave_flag) {
            // Aggiungi il giocatore alla coda
            bool added = queueAggiunta(nuovoGiocatore);
            if (!added) {
                printf("Server pieno - impossibile aggiungere giocatore\n");
                close(socket_fd);
                free(nuovoGiocatore);
                pthread_exit(NULL);
            }
        
            // Invia id del giocatore al client
            cJSON *response = cJSON_CreateObject();
            cJSON_AddNumberToObject(response, "id", nuovoGiocatore->id);
            char *response_str = cJSON_PrintUnformatted(response);
            send(socket_fd, response_str, strlen(response_str), 0);
            printf("Giocatore %s connesso con ID %d\n", nuovoGiocatore->nome, nuovoGiocatore->id);
            fflush(stdout);

            free(response_str);
            cJSON_Delete(response);
            // Avvia il thread per controllare il router
            pthread_t router_thread;
            pthread_create(&router_thread, NULL, controllaRouterThread, nuovoGiocatore);

            // detach permette al thread di liberare le risorse automaticamente quando termina
            pthread_detach(router_thread);
            
            //thread per gestire eventuali disconnessioni
            pthread_t disconnect_thread;
            pthread_create(&disconnect_thread, NULL, handleClose, nuovoGiocatore);
            pthread_detach(disconnect_thread); // Detach per liberare risorse automaticamente
            return NULL;

            
        }
        else {
            printf("Registrazione giocatore fallita\n");
            fflush(stdout);
            close(socket_fd);
            free(nuovoGiocatore);
        }

        
        
    }

    return NULL;
}

void *handleClose(void *arg) {
    GIOCATORE *giocatore = (GIOCATORE *)arg;

    while (1) {
        // Prepara il set di file descriptor da monitorare con select
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(giocatore->socket, &fds); // Monitora il client

        struct timeval timeout = {1, 0}; // Timeout di 1 secondo

        int result = select(giocatore->socket + 1, &fds, NULL, NULL, &timeout); // Aspetta fino a 1 secondo per vedere se il socket e' pronto per leggere qualcosa
        if (result < 0) {  // Errore
            perror("select error");
            break;
        }

        if (result > 0 && FD_ISSET(giocatore->socket, &fds)) { // Il socket e' pronto
            char buf;
            int n = recv(giocatore->socket, &buf, 1, MSG_PEEK); // Legge i byte senza consumarli

            if (n <= 0) { // Il socket e' stato chiuso
                printf("⚠️ Client %s disconnesso (monitor)\n", giocatore->nome);
                printf("ID giocatore: %d\n", giocatore->stato);
                if(giocatore->stato==IN_GIOCO){//gestione di eventuali partite aperte 
                    inviaVittoriaAltroGiocatore(giocatore);
                    printf("⚠️ Client %s è uscito dalla partita\n", giocatore->nome);
                }else{
                    inviaPareggioDisconnessione(giocatore);
                }
                   
                pthread_mutex_lock(&playerListLock);
                rimuoviGiocoByIdGiocatore(giocatore->id);
                rimuoviRichiestabyGiocatore(giocatore);
                queueRimozione(giocatore->id);
                
                // ✅ LIBERA LA MEMORIA DEL GIOCATORE
                close(giocatore->socket);
                free(giocatore);
                pthread_mutex_unlock(&playerListLock);
                
                
                break;
            }
        }

    }
    return NULL;
}


// Funzione per controllare il router e gestire le richieste
void * controllaRouterThread(void *arg) {
    GIOCATORE *nuovoGiocatore = (GIOCATORE *)arg;

    if(nuovoGiocatore == NULL) {
        return NULL; 
    }
    char buffer[BUFFER_SIZE];
    int leave_flag = 0;
    int socket_fd = nuovoGiocatore->socket;
    printf("Thread di controllo router avviato per il giocatore %s\n", nuovoGiocatore->nome);
    fflush(stdout);
    
    // Inizializza il buffer
    memset(buffer, 0, sizeof(buffer));
    
    // Continua fino alla disconnessione, non solo quando è IN_HOME
    while (!leave_flag) {
        int size = read(socket_fd, buffer, BUFFER_SIZE - 1);

        if (size <= 0) {
            leave_flag = 1; // Indica che il client si è disconnesso o c'è stato un errore
            break;
        }
        buffer[size] = '\0';
        
        // Gestisce tutti i messaggi in base allo stato del giocatore
        controllaRouter(buffer, nuovoGiocatore, socket_fd);
    }
    
    printf("Thread router terminato per giocatore %s\n", nuovoGiocatore->nome);
    return NULL; // Termina il thread
}

void controllaRouter(char* buffer, GIOCATORE *nuovoGiocatore, int socket_nuovo){
        
    cJSON *json = cJSON_Parse(buffer);
    if (!json) return;
    cJSON *path = cJSON_GetObjectItem(json, "path");
    cJSON *body = cJSON_GetObjectItem(json, "body");

    //aggiungi controllo per path se è vuoto
    if (!path || !cJSON_IsString(path)) {
        printf("Campo 'path' mancante o non valido\n");
        cJSON_Delete(json);
        return;
    }

    // Messaggi disponibili solo quando il giocatore è IN_HOME
    if (nuovoGiocatore->stato == IN_HOME) {
        //questo primo path è restituisce al client la lista dei giochi in attesa
        if (strcmp(path->valuestring, "/waiting_games") == 0) {
            serverAspettaPartita(buffer,nuovoGiocatore,socket_nuovo);
        }else if(strcmp(path->valuestring, "/new_game") == 0) {
            serverNuovaPartita(buffer,nuovoGiocatore);
        }
        else if(strcmp(path->valuestring, "/add_request") == 0) {
            serverAggiungiRichiesta(buffer,nuovoGiocatore,body);
        }
        else if(strcmp(path->valuestring, "/remove_request") == 0) {
            serverRimuoviRichiesta(buffer,nuovoGiocatore,body);
        }
        else if(strcmp(path->valuestring, "/accept_request") == 0) {
            serverAccettaRichiesta(buffer,nuovoGiocatore,body);
        }
        else if(strcmp(path->valuestring, "/decline_request") == 0) {
            serverRifiutaRichiesta(buffer,nuovoGiocatore,body);
        }
        else if(strcmp(path->valuestring, "/vittoria_game") == 0){
            serverVittoria(buffer,nuovoGiocatore,body);
        }
        else if(strcmp(path->valuestring, "/pareggio_game") == 0){
            serverPareggio(buffer,nuovoGiocatore,body);
        }
        else if(strcmp(path->valuestring, "/exit_game") == 0){
            serverUscitaPareggio(buffer,nuovoGiocatore,body);
        }
        else if(strcmp(path->valuestring, "/get_sent_requests") == 0){
            serverRichiesteInviate(buffer,nuovoGiocatore,body);
        }
        else if(strcmp(path->valuestring, "/get_received_requests") == 0){
            serverRichiesteRicevute(buffer, nuovoGiocatore,body);
        }
        else {
            printf("Path non riconosciuto per giocatore IN_HOME: %s\n", path->valuestring);
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Comando non disponibile");
        }
    } else if (nuovoGiocatore->stato == IN_GIOCO) {
        if(strcmp(path->valuestring, "/game_move") == 0){
            serverMossaGioco(buffer,nuovoGiocatore,body);
        }
        else if(strcmp(path->valuestring, "/exit_game") == 0){
            serverUscitaGioco(buffer,nuovoGiocatore,body);
        }
        else {
            printf("Path non riconosciuto per giocatore IN_GIOCO: %s\n", path->valuestring);
            inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Comando non disponibile in partita");
        }
    }
    else 
    {
        printf("Stato giocatore non riconosciuto: %d\n", nuovoGiocatore->stato);
        inviaMessaggioSuccesso(0, nuovoGiocatore->socket, "Stato non valido");
    }
    cJSON_Delete(json);
}



int main(){
  
    int fd;
    struct sockaddr_in address;
    socklen_t addrlen=sizeof(address);

    fd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(fd==0){
        perror("errore socket");
        close(fd);
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr=INADDR_ANY; 
    address.sin_port = htons(PORT);
    int opt = 1;

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,&opt,sizeof(opt)) < 0){
		perror("errore setsockopt");
        close(fd);
        return 1;
	}

    if(bind(fd,(struct sockaddr*)&address,sizeof(address))<0){
        perror("errore bind");
        close(fd);
        return 1;
    }

    if(listen(fd, MAX_GIOCATORI)<0){
        perror("errore listen");
        close(fd);
        return 1;
    }

    while(1){
        printf("Sono in attesa\n");
        fflush(stdout);

        int * nuovoSocket = (int*)malloc(sizeof(int));
        if ((*nuovoSocket = accept(fd, (struct sockaddr *)&address,(socklen_t*)&addrlen)) < 0) {
            perror("errore_accept");
            free(nuovoSocket);
            continue;
        }
        printf("Connessione accettata\n");
        fflush(stdout);
        //visto che si aggiorna una variabile che potrebbe essere usata da diversi thread occuore un mutex

        pthread_t thread_id;
        // Creazione del thread per gestire il client
        pthread_create(&thread_id, NULL, handleClient, nuovoSocket);
        //controllo se thread_id è stato creato correttamente
        pthread_detach(thread_id); // Detach per liberare risorse automaticamente
        
        
    }

        close(fd);
        return 0;
}
