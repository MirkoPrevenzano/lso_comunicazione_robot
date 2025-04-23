
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_GIOCATORI 20

typedef struct {
    int * socket;
    pthread_t thread;
    char nome[20]; //nome di massimo 20 char
    int in_partita;//valore 0 o 1
}GIOCATORE;

int numero_connessioni=0;
GIOCATORE Giocatori[MAX_GIOCATORI];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//totale di 10 partite contemporaneamnete
//l'idea è avere 10 slot (lobby) in cui giocatori entrano e giocano contro l'avversario
//se si hanno più di 10 partite si dice al client di aspettare , finche non si liberà un posto.
int cerca_indice(){};

void handle_client(void *arg){
    
   
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
            perror("accept");
            free(socket_nuovo);
            continue;
        }else
            numero_connessioni++;

        if(numero_connessioni>20){
            perror("numero max giocatori superato");
            close(fd);
            return 1;
        }
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, socket_nuovo) != 0) {
            perror("pthread_create");
            close(*socket_nuovo);
            free(socket_nuovo);
        } else {
            pthread_detach(thread_id); // Evita memory leak da thread terminati
        }
    }

        close(fd);
        return 0;
    }

