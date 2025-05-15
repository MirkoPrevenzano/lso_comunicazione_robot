#include "./server.h"

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

void aggiorna_numero_connessioni(int * socket_nuovo){
    pthread_mutex_lock(&lock);

    if(numero_connessioni>=MAX_GIOCATORI){
        perror("numero max giocatori superato");
        if(socket_nuovo!=NULL){
            send(*socket_nuovo,msg,strlen(msg),0);
            close(*socket_nuovo);
            free(socket_nuovo);
        }
        //Se tenta di connettersi un nono giocatore esso dovrà aspettare
        //li inviamo un messaggio : numero max di giocatori raggiunto.
    }else{
        numero_connessioni++;
        printf("Connessione accettata, numero connessioni: %d\n", numero_connessioni);
        fflush(stdout);
    }
    pthread_mutex_unlock(&lock);
}

void queue_add(GIOCATORE*giocatore_add){
	pthread_mutex_lock(&playerListLock);
    bool trovato = false;
	for(int i=0; i < MAX_GIOCATORI; ++i){
		if(!Giocatori[i] && !trovato){
            trovato = true;
			Giocatori[i] = giocatore_add;
		}
	}

	pthread_mutex_unlock(&playerListLock);
}


void queue_remove(int id){
	pthread_mutex_lock(&lock);
    pthread_mutex_lock(&playerListLock);
    bool trovato = false;
	for(int i=0; i < MAX_GIOCATORI; ++i){
		if(Giocatori[i] && !trovato){
			if(Giocatori[i]->id == id){
                Giocatori[i] = NULL;
                trovato = true;
			}
		}
	}
    pthread_mutex_unlock(&playerListLock);
    if(trovato){
        numero_connessioni--;
        printf("Connessione chiusa, numero connessioni: %d\n", numero_connessioni);
        fflush(stdout);
    }
    else
        printf("Giocatore %d non trovato\n", id);

    pthread_mutex_unlock(&lock);
}


void *handle_client(void *arg) {
    char buffer[BUFFER_SIZE];
    int leave_flag = 0;
    int *socket_nuovo = (int *)arg;
    int socket_fd = *socket_nuovo;
    
    printf("Nuovo client connesso\n");
    fflush(stdout);

    GIOCATORE *nuovo_giocatore = malloc(sizeof(GIOCATORE));
    if (!nuovo_giocatore) {
        fprintf(stderr, "Errore allocazione memoria giocatore\n");
        close(socket_fd);
        free(socket_nuovo);
        pthread_exit(NULL);
    }

    nuovo_giocatore->socket = socket_nuovo;
    nuovo_giocatore->id = numero_connessioni;

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
            queue_add(nuovo_giocatore);
        
            // Invia "1" al client per indicare il successo
            const char *success_msg = "1";
            send(socket_fd, success_msg, strlen(success_msg), 0);
        } else {
            // Invia "0" al client per indicare un errore
            const char *error_msg = "0";
            send(socket_fd, error_msg, strlen(error_msg), 0);
        }

        // Main loop (attualmente vuoto)
        // ...dopo la registrazione...
        while (!leave_flag) {
            int size = read(socket_fd, buffer, sizeof(buffer) - 1);
            if (size <= 0) {
                leave_flag = 1;
                break;
            }
            buffer[size] = '\0';
            cJSON *json = cJSON_Parse(buffer);
            if (!json) continue;
            cJSON *path = cJSON_GetObjectItem(json, "path");
            cJSON *body = cJSON_GetObjectItem(json, "body");
            if (strcmp(path->valuestring, "/waiting_games") == 0) {
                printf("Richiesta di giochi in attesa ricevuta\n");
                handlerInviaGames(socket_nuovo);
            }
            if(strcmp(path->valuestring, "/new_game") == 0) {
                printf("Richiesta di creazione partita ricevuta\n");
                new_game(&leave_flag,buffer,nuovo_giocatore);

            }
            if (strcmp(path->valuestring, "/close") == 0) {
                printf("Richiesta di disconnessione\n");
                remove_game_by_player_id(nuovo_giocatore->id);
                leave_flag = 1;
            }
            // ...altre richieste...
            cJSON_Delete(json);
        }
    }

    // Pulizia finale
    const char *msg2 = "Disconnessione effettuata.\n";
    send(socket_fd, msg2, strlen(msg2), 0);
    close(socket_fd);
    free(socket_nuovo);
    queue_remove(nuovo_giocatore->id);
    free(nuovo_giocatore);
    
    pthread_detach(pthread_self());
    return NULL;
}

int main(){
  
    // --- PARTITA DI TEST ---
    GAME *test_game = malloc(sizeof(GAME));
    memset(test_game, 0, sizeof(GAME));
    test_game->id = 1;
    test_game->turno = 0;
    test_game->esito = -1;

    GIOCATORE *proprietario = malloc(sizeof(GIOCATORE));
    memset(proprietario, 0, sizeof(GIOCATORE));
    strcpy(proprietario->nome, "proprietario_test");
    proprietario->id = 0;
    proprietario->socket = NULL;

    // ATTENZIONE: usa il nome corretto del campo nella struct GAME!
    test_game->giocatoreParticipante[0] = proprietario; // o test_game->giocatori[0]
    test_game->giocatoreParticipante[1] = NULL;         // o test_game->giocatori[1]

    Partite[0] = test_game;

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
        }else{
            printf("Aggiungo connessione\n");
            fflush(stdout);
            aggiorna_numero_connessioni(socket_nuovo);
        }
        //visto che si aggiorna una variabile che potrebbe essere usata da diversi thread occuore un mutex

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, socket_nuovo) != 0) {
            perror("errore_pthread_create");
            send(*socket_nuovo,msg1,strlen(msg1),0);
            close(*socket_nuovo);
            free(socket_nuovo);
        } 
    }

        close(fd);
        return 0;
    }
