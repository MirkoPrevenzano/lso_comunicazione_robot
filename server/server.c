
#include "./server.h"
//ogni gioco deve sempre avere un proprietario in qualsiasi momento
//GIOCATORE* Giocatori[MAX_GIOCATORI]; verrà trattata come una pseudo-coda.

void aggiorna_numero_connessioni(int * socket_nuovo){
    pthread_mutex_lock(&lock);
    numero_connessioni++;

    if(numero_connessioni>=MAX_GIOCATORI){
        perror("numero max giocatori superato");
        if(socket_nuovo!=NULL){
            send(*socket_nuovo,msg,strlen(msg),0);
            close(*socket_nuovo);
            free(socket_nuovo);
        }
        numero_connessioni--;
        //Se tenta di connettersi un nono giocatore esso dovrà aspettare
        //li inviamo un messaggio : numero max di giocatori raggiunto.
    }
    pthread_mutex_unlock(&lock);
}

void queue_add(GIOCATORE*giocatore_add){
	pthread_mutex_lock(&lock2);

	for(int i=0; i < MAX_GIOCATORI; ++i){
		if(!Giocatori[i]){
			Giocatori[i] = giocatore_add;
			break;
		}
	}

	pthread_mutex_unlock(&lock2);
}


void queue_remove(int id){
	pthread_mutex_lock(&lock);
    pthread_mutex_lock(&lock2);

	for(int i=0; i < MAX_GIOCATORI; ++i){
		if(Giocatori[i]){
			if(Giocatori[i]->id == id){
				Giocatori[i] = NULL;
				break;
			}
		}
	}
    numero_connessioni--;

    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock);
}


void *handle_client(void *arg){
    char buffer[BUFFER_SIZE];
	char nome[30];
	int leave_flag = 0;
    int * socket_nuovo=(int*)arg;
    printf("Si connette\n");
    fflush(stdout);

    GIOCATORE *nuovo_giocatore = (GIOCATORE *)malloc(sizeof(GIOCATORE));
    nuovo_giocatore->socket = socket_nuovo;
    nuovo_giocatore->id = numero_connessioni;

    int size = read(*socket_nuovo, buffer, sizeof(buffer)-1);
    if(size > 0){
        buffer[size] = '\0';
        printf("Buffer ricevuto: %s\n", buffer);
        fflush(stdout);
        cJSON *json = cJSON_Parse(buffer);

    if (json == NULL) {
        printf("Errore nel parser JSON\n");
        leave_flag = 1;
    } else {
        cJSON *path = cJSON_GetObjectItem(json, "path");
        cJSON *body = cJSON_GetObjectItem(json, "body");

        if (path && path->valuestring) {
            if (strcmp(path->valuestring, "/register") == 0) {
                if (body && cJSON_IsString(body)) {
                    // Parsing del JSON annidato in "body"
                    cJSON *body_json = cJSON_Parse(body->valuestring);
                    if (body_json) {
                        cJSON *body_nick = cJSON_GetObjectItem(body_json, "nickname");
                        if (body_nick && cJSON_IsString(body_nick)) {
                            printf("Nickname : %s\n", body_nick->valuestring);
                            strcpy(nuovo_giocatore->nome, body_nick->valuestring);
                        } else {
                            printf("Il campo 'nickname' non è presente o non è una stringa\n");
                            leave_flag = 1;
                        }
                        cJSON_Delete(body_json); // Libera la memoria del JSON annidato
                    } else {
                        printf("Errore  JSON annidato in 'body'\n");
                        leave_flag = 1;
                    }
                } else {
                    printf("Il campo 'body' non è presente o non è una stringa\n");
                    leave_flag = 1;
                }
            } else {
                printf("no path register: %s\n", path->valuestring);
                leave_flag = 1;
            }
        } else {
            printf("path non valido\n");
            leave_flag = 1;
        }
        cJSON_Delete(body);
        cJSON_Delete(path); 


    }

        queue_add(nuovo_giocatore);

        while(1){
            if(leave_flag){
                break;
            }
           handlerInviaGames(socket_nuovo,buffer);
           handlerGames(&leave_flag,socket_nuovo,buffer,nuovo_giocatore);
            
        }
    }
    
    
        send(*(nuovo_giocatore->socket),msg2,strlen(msg2),0);
        close(*(nuovo_giocatore->socket));
        free(nuovo_giocatore->socket);
        queue_remove(nuovo_giocatore->id);
        free(nuovo_giocatore);
        pthread_detach(pthread_self());// Evita memory leak da thread terminati
};

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
