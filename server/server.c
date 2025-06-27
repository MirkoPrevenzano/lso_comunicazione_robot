#include "./server.h"
#include "./handler.h"

int numero_connessioni = 0;
int numero_partite = 0;

GAME* Partite[MAX_GAME] = { NULL };
GIOCATORE* Giocatori[MAX_GIOCATORI] = { NULL };

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t playerListLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gameListLock = PTHREAD_MUTEX_INITIALIZER;

char *msg = "numero max di giocatori raggiunto.";
char *msg1 = "errore";
char *msg2 = "disconnesione";


//ogni gioco deve sempre avere un proprietario in qualsiasi momento
//GIOCATORE* Giocatori[MAX_GIOCATORI]; verrà trattata come una pseudo-coda.

GIOCATORE * SearchPlayerByid(int id_player){
    pthread_mutex_lock(&playerListLock);
    for(int i = 0 ; i<MAX_GIOCATORI;i++){
        if(Giocatori[i]->id==id_player){
            pthread_mutex_unlock(&playerListLock);
            return Giocatori[i];
        }
    }
            
    pthread_mutex_unlock(&playerListLock);
    return NULL;
}

bool queue_add(GIOCATORE*giocatore_add){
    //aggiunto controllo per evitare che giocatore_add sia NULL
    if (!giocatore_add) {
        printf("Errore: giocatore_add è NULL\n");
        return false;
    }
    
	pthread_mutex_lock(&playerListLock);
    bool trovato = false;
	for(int i=0; i < MAX_GIOCATORI; ++i){
		if(!Giocatori[i] && !trovato){
            trovato = true;
            numero_connessioni++;
			Giocatori[i] = giocatore_add;
            Giocatori[i]->id = i;
            Giocatori[i]->stato = IN_HOME; 
		}
	}

	pthread_mutex_unlock(&playerListLock);
    return trovato;
}

//l'utilizzo di due mutex può causare deadlock  se in un altro thread si cerca di accedere a Giocatori e Partite contemporaneamente
//uso playerListLock anche per numero_connessioni
void queue_remove(int id){
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
        numero_connessioni--;
        printf("Connessione chiusa, numero connessioni: %d\n", numero_connessioni);
        fflush(stdout);
    }
    else
        printf("Giocatore %d non trovato\n", id);


}


void *handle_client(void *arg) {
    char buffer[BUFFER_SIZE];
    int leave_flag = 0;
    int *socket_ptr = (int *)arg;
    int socket_fd = *socket_ptr;
    
    // ✅ LIBERA SUBITO il socket pointer
    free(socket_ptr);
    
    printf("Nuovo client connesso\n");
    fflush(stdout);

    GIOCATORE *nuovo_giocatore = malloc(sizeof(GIOCATORE));
    if (!nuovo_giocatore) {
        fprintf(stderr, "Errore allocazione memoria giocatore\n");
        close(socket_fd);
        pthread_exit(NULL);
    }

    // Inizializza il nuovo giocatore
    memset(nuovo_giocatore, 0, sizeof(GIOCATORE));
    nuovo_giocatore->socket = socket_fd;
    nuovo_giocatore->id = -1; // ID non valido inizialmente

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
                        strncpy(nuovo_giocatore->nome, nickname->valuestring, sizeof(nuovo_giocatore->nome) - 1);
                        nuovo_giocatore->nome[sizeof(nuovo_giocatore->nome) - 1] = '\0';
                        printf("Giocatore %s registrato\n", nuovo_giocatore->nome);
                    }
                    cJSON_Delete(body_json);
                }
            }
            cJSON_Delete(json);
        }

        if (!leave_flag) {
            // Aggiungi il giocatore alla coda
            bool added = queue_add(nuovo_giocatore);
            if (!added) {
                printf("Server pieno - impossibile aggiungere giocatore\n");
                close(socket_fd);
                free(nuovo_giocatore);
                pthread_exit(NULL);
            }
        
            // Invia id del giocatore al client
            cJSON *response = cJSON_CreateObject();
            cJSON_AddNumberToObject(response, "id", nuovo_giocatore->id);
            char *response_str = cJSON_PrintUnformatted(response);
            send(socket_fd, response_str, strlen(response_str), 0);
            printf("Giocatore %s connesso con ID %d\n", nuovo_giocatore->nome, nuovo_giocatore->id);
            fflush(stdout);

            free(response_str);
            cJSON_Delete(response);
            // Avvia il thread per controllare il router
            pthread_t router_thread;
            pthread_create(&router_thread, NULL, checkRouterThread, nuovo_giocatore);
            //cosa fa detach?
            // detach permette al thread di liberare le risorse automaticamente quando termina
            pthread_detach(router_thread); // Detach per liberare risorse automaticamente
            
            //thread per gestire eventuali disconnessioni
            pthread_t disconnect_thread;
            pthread_create(&disconnect_thread, NULL, handle_close, nuovo_giocatore);
            pthread_detach(disconnect_thread); // Detach per liberare risorse automaticamente
            return NULL;

            
        }
        else {
            printf("Registrazione giocatore fallita\n");
            fflush(stdout);
            close(socket_fd);
            free(nuovo_giocatore);
        }

        
        
    }

    return NULL;
}

void *handle_close(void *arg) {
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
                pthread_mutex_lock(&playerListLock);
                remove_game_by_player_id(giocatore->id);
                remove_request_by_player(giocatore);
                queue_remove(giocatore->id);
                
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
void * checkRouterThread(void *arg) {
    GIOCATORE *nuovo_giocatore = (GIOCATORE *)arg;

    if(nuovo_giocatore == NULL) {
        return NULL; 
    }
    char buffer[BUFFER_SIZE];
    int leave_flag = 0;
    int socket_fd = nuovo_giocatore->socket;
    printf("Thread di controllo router avviato per il giocatore %s\n", nuovo_giocatore->nome);
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
        checkRouter(buffer, nuovo_giocatore, socket_fd, &leave_flag);
    }
    
    printf("Thread router terminato per giocatore %s\n", nuovo_giocatore->nome);
    return NULL; // Termina il thread
}

void checkRouter(char* buffer, GIOCATORE*nuovo_giocatore, int socket_nuovo, int *leave_flag){
        
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
        if (nuovo_giocatore->stato == IN_HOME) {
            //questo primo path è restituisce al client la lista dei giochi in attesa
            if (strcmp(path->valuestring, "/waiting_games") == 0) {
                pthread_mutex_lock(&gameListLock);
                printf("Richiesta di giochi in attesa ricevuta\n");
                handlerInviaGames(socket_nuovo);
                pthread_mutex_unlock(&gameListLock);
            }else if(strcmp(path->valuestring, "/new_game") == 0) {
                pthread_mutex_lock(&gameListLock);
                printf("Richiesta di creazione partita ricevuta\n");
                new_game(leave_flag,buffer,nuovo_giocatore);
                pthread_mutex_unlock(&gameListLock);
            }
            else if(strcmp(path->valuestring, "/add_request") == 0) {
                printf("Richiesta di aggiunta a una partita ricevuta\n");
                printf("Body ricevuto: %s\n", body->valuestring);
                fflush(stdout);
                
                // Parsa il JSON body
                cJSON *body_json = cJSON_Parse(body->valuestring);
                if (!body_json) {
                    printf("Errore nel parsing del JSON body\n");
                    *leave_flag = 1;
                } else {
                    cJSON *id_item = cJSON_GetObjectItem(body_json, "id");
                    if (id_item && cJSON_IsNumber(id_item)) {
                        int id_partita = id_item->valueint;
                        printf("ID partita: %d\n", id_partita);
                        // Aggiungi la richiesta alla partita
                        aggiungi_richiesta(id_partita, nuovo_giocatore);
                    } else {
                        printf("Il campo 'id' non è presente o non è un numero.\n");
                        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                    }
                    cJSON_Delete(body_json);
                }
            }
            else if(strcmp(path->valuestring, "/remove_request") == 0) {
                printf("Richiesta di rimozione da una partita ricevuta\n");
                printf("Body ricevuto: %s\n", body->valuestring);
                fflush(stdout);
                
                // Parsa il JSON body
                cJSON *body_json = cJSON_Parse(body->valuestring);
                if (!body_json) {
                    printf("Errore nel parsing del JSON body\n");
                    send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                } else {
                    cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
                    if (id_item && cJSON_IsNumber(id_item)) {
                        int id_partita = id_item->valueint;
                        printf("ID partita: %d\n", id_partita);
                        // Rimuovi la richiesta dalla partita
                        rimuovi_richiesta(id_partita, nuovo_giocatore); 
                    } else {
                        printf("Il campo 'game_id' non è presente o non è un numero.\n");
                        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                    }
                    cJSON_Delete(body_json);
                }
            }
            else if(strcmp(path->valuestring, "/accept_request") == 0) {
               printf("Richiesta di accetazione una partita ricevuta\n");
                printf("Body ricevuto: %s\n", body->valuestring);
                fflush(stdout);
                
                // Parsa il JSON body
                cJSON *body_json = cJSON_Parse(body->valuestring);
                if (!body_json) {
                    printf("Errore nel parsing del JSON body\n");
                    send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                } else {
                    cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
                    cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "player_id");
                    //bisogan ora inviare al player id il messaggio ed eseguire joinlobby
                     if (id_item && cJSON_IsNumber(id_item)) {
                        int id_partita = id_item->valueint;
                        printf("ID partita: %d\n", id_partita);
                        if (id_item_2 && cJSON_IsNumber(id_item_2)) {
                            int id_player = id_item_2->valueint;
                            printf("ID Giocatore della richiesta: %d\n", id_player);
                            GIOCATORE * nuovo_giocatore_2 = SearchPlayerByid(id_player);
                            accetta_richiesta(searchRichiesta(id_partita, SearchPlayerByid(id_player)),id_partita,nuovo_giocatore, nuovo_giocatore_2);
                        }else{
                        printf("Il campo 'player_id' non è presente o non è un numero.\n");
                        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                    }}else {
                        printf("Il campo 'game_id' non è presente o non è un numero.\n");
                        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                    }
                    cJSON_Delete(body_json);
                }
            }
            else if(strcmp(path->valuestring, "/decline_request") == 0) {
               printf("rifiuto di una richiesta ricevuta\n");
                printf("Body ricevuto: %s\n", body->valuestring);
                fflush(stdout);
                
                // Parsa il JSON body
                cJSON *body_json = cJSON_Parse(body->valuestring);
                if (!body_json) {
                    printf("Errore nel parsing del JSON body\n");
                    send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                } else {
                    cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id");
                    cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "player_id");
                    if (id_item && cJSON_IsNumber(id_item)) {
                        int id_partita = id_item->valueint;
                        printf("ID partita: %d\n", id_partita);
                        if (id_item_2 && cJSON_IsNumber(id_item_2)) {
                            int id_player = id_item_2->valueint;
                            printf("ID Giocatore della richiesta: %d\n", id_player);
                            GIOCATORE * nuovo_giocatore_2 = SearchPlayerByid(id_player);
                            rifiuta_richiesta(searchRichiesta(id_partita, nuovo_giocatore_2),id_partita,nuovo_giocatore,nuovo_giocatore_2); 
                        }else{
                            printf("Il campo 'player_id' non è presente o non è un numero.\n");
                            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                        }
                    } else {
                        printf("Il campo 'game_id' non è presente o non è un numero.\n");
                        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                    }
                    cJSON_Delete(body_json);
                }
            }
            else {
                printf("Path non riconosciuto per giocatore IN_HOME: %s\n", path->valuestring);
                send_success_message(0, nuovo_giocatore->socket, "Comando non disponibile");
            }
        }
        else if (nuovo_giocatore->stato == IN_GIOCO) {
            //inviare la griglia
            if(strcmp(path->valuestring, "/waiting_game_response") == 0){
                cJSON *body_json = cJSON_Parse(body->valuestring);
                if (!body_json) {
                    printf("Errore nel parsing del JSON body\n");
                    send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                } else {
                    cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id"); 
                     if (id_item && cJSON_IsNumber(id_item)) {
                            int id_partita = id_item->valueint;
                            printf("ID partita: %d\n", id_partita);
                            pthread_mutex_lock(&gameListLock);
                            printf("Richiesta della griglia di gioco ricevuta\n");
                            HandlerInviaMovesPartita(nuovo_giocatore,searchPartitaById(id_partita));
                            pthread_mutex_unlock(&gameListLock);
                   }else{
                        printf("Il campo 'game_id' non è presente o non è un numero.\n");
                        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                   }
                }
            }
            if(strcmp(path->valuestring, "/game_move") == 0){
                cJSON *body_json = cJSON_Parse(body->valuestring);
                if (!body_json) {
                    printf("Errore nel parsing del JSON body\n");
                    send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                } else {
                    cJSON *id_item = cJSON_GetObjectItem(body_json, "game_id"); 
                    cJSON *id_item_2 = cJSON_GetObjectItem(body_json, "col");
                    cJSON *id_item_3 = cJSON_GetObjectItem(body_json, "row");  
                     if (id_item && cJSON_IsNumber(id_item)) {
                            int id_partita = id_item->valueint;
                            printf("ID partita: %d\n", id_partita);
                            if (id_item_2 && cJSON_IsNumber(id_item_2)) {
                                    if (id_item_3 && cJSON_IsNumber(id_item_3)) {
                                        int col = id_item_2->valueint;
                                        int row = id_item_3->valueint;
                                        GAME*partita=searchPartitaById(id_partita);
                                        aggiorna_partita(partita,nuovo_giocatore,col,row);
                                        handler_game_response(nuovo_giocatore,partita);
                            }else{
                                    printf("Il campo 'row' non è presente o non è un numero.\n");
                                    send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                        }}else{
                            printf("Il campo 'col' non è presente o non è un numero.\n");
                            send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                   }}else{
                        printf("Il campo 'game_id' non è presente o non è un numero.\n");
                        send_success_message(0, nuovo_giocatore->socket, "Parametri non validi");
                }}}
            if(strcmp(path->valuestring, "/exit_game") == 0){
                //ricevere intenzione di uscire dalla partita
            }
        }
        else {
            printf("Stato giocatore non riconosciuto: %d\n", nuovo_giocatore->stato);
            send_success_message(0, nuovo_giocatore->socket, "Stato non valido");
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

        int * socket_nuovo = (int*)malloc(sizeof(int));
        if ((*socket_nuovo = accept(fd, (struct sockaddr *)&address,(socklen_t*)&addrlen)) < 0) {
            perror("errore_accept");
            free(socket_nuovo);
            continue;
        }
        printf("Connessione accettata\n");
        fflush(stdout);
        //visto che si aggiorna una variabile che potrebbe essere usata da diversi thread occuore un mutex

        pthread_t thread_id;
        // Creazione del thread per gestire il client
        pthread_create(&thread_id, NULL, handle_client, socket_nuovo);
        //controllo se thread_id è stato creato correttamente
        pthread_detach(thread_id); // Detach per liberare risorse automaticamente
        
        
    }

        close(fd);
        return 0;
    }


