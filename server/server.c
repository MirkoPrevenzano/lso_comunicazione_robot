
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_GIOCATORI 8
#define MAX_GAME 4

typedef struct {
    int * socket;
    int id;
    char nome[30]; //nome di massimo 30 char
    int in_partita;//valore 0 o 1
}GIOCATORE;

int static numero_connessioni=0;
char *msg = "numero max di giocatori raggiunto.";
char *msg1 = "errore";
char *msg2 = "disconnesione";
GIOCATORE* Giocatori[MAX_GIOCATORI];
GIOCATORE* Game[2][MAX_GAME];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

//totale di 4 partite contemporaneamnete
//l'idea è avere 4 slot (lobby) in cui giocatori entrano e giocano contro l'avversario
//se si hanno più di 4 partite si dice al client di aspettare , finche non si liberà un posto.
//GIOCATORE* Giocatori[MAX_GIOCATORI]; verrà trattata come una pseudo-coda.

void nuovo_game(int i,int*flag,GIOCATORE* giocatore){};

void aggiorna_numero_connessioni(int * socket_nuovo){
    pthread_mutex_lock(&lock);
    numero_connessioni++;

    if(numero_connessioni>=MAX_GIOCATORI){
        perror("numero max giocatori superato");
        if(socket_nuovo!=NULL){
            send(socket_nuovo,msg,strlen(msg),0);
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


void handle_client(void *arg){
    char buffer[BUFFER_SIZE];
	char nome[30];
	int leave_flag = 0;
    int * socket_nuovo=(int*)arg;
    
    GIOCATORE *nuovo_giocatore = (GIOCATORE *)malloc(sizeof(GIOCATORE));
    nuovo_giocatore->socket = socket_nuovo;
    nuovo_giocatore->id = numero_connessioni;
    nuovo_giocatore->in_partita=0;

    if(recv(nuovo_giocatore->socket, nome, 30, 0) <= 0 || strlen(nome) <  2 || strlen(nome) >= 30-1){
		send(socket_nuovo,msg1,strlen(msg1),0);
		leave_flag = 1;
	}else
		strcpy(nuovo_giocatore->nome, nome);

    queue_add(nuovo_giocatore);

    while(1){
        if(leave_flag){
			break;
		}
        //l'idea è che nella GUI si trovano 4 pulsanti , ognuno di loro rappresenta una lobby
        //quando il client clicca una lobby(pulsante) invia un numero al server
        //il server controlla se la lobby ha un posto libero
        int valread = read(socket_nuovo,buffer,sizeof(buffer));
        int num=0;
        if(valread>0)
            num = atoi(buffer);//dovrei svuotare il buffer?
        
        switch(num){
            case 1:
                nuovo_game(1,&leave_flag,nuovo_giocatore);
                break;
            case 2:
                nuovo_game(2,&leave_flag,nuovo_giocatore);
                break;
            case 3:
                nuovo_game(3,&leave_flag,nuovo_giocatore);
                break;
            case 4:
                nuovo_game(4,&leave_flag,nuovo_giocatore);
                break;
        }


        }
    
        send(nuovo_giocatore->socket,msg2,strlen(msg2),0);
        close(nuovo_giocatore->socket);
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

    if(bind(fd,(struct sockadddr*)&address,sizeof(address))<0){
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
        int * socket_nuovo = (int*)malloc(sizeof(int));
        if ((socket_nuovo = accept(fd, (struct sockaddr *)&address,(socklen_t)&addrlen)) < 0) {
            perror("errore_accept");
            free(socket_nuovo);
            continue;
        }else
            aggiorna_numero_connessioni(socket_nuovo);
            //visto che si aggiorna una variabile che potrebbe essere usata da diversi thread occuore un mutex

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, socket_nuovo) != 0) {
            perror("errore_pthread_create");
            send(socket_nuovo,msg1,strlen(msg1),0);
            close(*socket_nuovo);
            free(socket_nuovo);
        } 
    }

        close(fd);
        return 0;
    }

