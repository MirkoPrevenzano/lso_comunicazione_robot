#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <cjson/cJSON.h> //installata libreria esterna
#include "handler.h"

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER 2048

void *handle_client(void *arg) {

    int client_sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER];

    int size = read(client_sock, buffer, sizeof(buffer)-1);
    if(size > 0){
        buffer[size] = '\0';

        cJSON *json = cJSON_Parse(buffer);

        if(json==NULL){
            printf("Error nel parser");
        }else{
            cJSON *path = cJSON_GetObjectItem(json, "path");
            cJSON *body = cJSON_GetObjectItem(json, "body");
            if (path && path->valuestring) {
                if(strcmp(path->valuestring,"/prestito")==0){
                    if (body && cJSON_IsString(body)) {
                        handle_prestito(body);
                    } else {
                        printf("Body non valido o mancante\n");
                    }                }
                else if(strcmp(path->valuestring,"/restituzione")==0){
                    if (body && cJSON_IsString(body)) {
                        handle_restituzione(body);
                    } else {
                        printf("Body non valido o mancante\n");
                    }
                }
                else if (strcmp(path->valuestring, "/login") == 0) {
                    if (body && cJSON_IsString(body)) {
                        bool login_success = handle_login(body);
                   
                        char response_message[BUFFER];
                        if (login_success) {
                            snprintf(response_message, sizeof(response_message), "Login eseguito con successo");
                        } else {
                            snprintf(response_message, sizeof(response_message), "Login fallito: credenziali errate");
                        }
                        write(client_sock, response_message, strlen(response_message));
                    } else {
                        printf("Body non valido o mancante\n");
                        write(client_sock, "Body non valido o mancante", strlen("Body non valido o mancante"));
                    }
                }
                else if(strcmp(path->valuestring,"/get-film")==0){
                    if (body && cJSON_IsString(body)) {
                        handle_get_film(body);
                    } else {
                        printf("Body non valido o mancante\n");
                    }
                }
                else if(strcmp(path->valuestring,"/invio-msg")==0){
                    if (body && cJSON_IsString(body)) {
                        handle_send_msg(body);
                    } else {
                        printf("Body non valido o mancante\n");
                    }
                }
                else if(strcmp(path->valuestring,"/get-msg")==0){
                    if (body && cJSON_IsString(body)) {
                        handle_get_msg(body);
                    } else {
                        printf("Body non valido o mancante\n");
                    }
                }
                else if(strcmp(path->valuestring,"/check-prestito")==0){
                    if (body && cJSON_IsString(body)) {
                        bool response = handle_check_prestito(body);
                        char response_message[BUFFER];
                        /*La funzione snprintf è utilizzata per formattare una stringa 
                            e scriverla in un buffer, garantendo che non venga superata la 
                            dimensione massima del buffer.*/
                        snprintf(response_message, sizeof(response_message), "prestito:%s", response ? "true" : "false");
                        write(client_sock, response_message, strlen(response_message));
                    } else {
                        printf("Body non valido o mancante\n");
                    }
                }
                else{
                    printf("Path non esistente: %s\n", path->valuestring);
                }
            }
        }
    }
    else if(size == 0){
        printf("Il client ha chiuso la connessione");
    }else{
        perror("Error client read");
    }

    close(client_sock);
    return NULL;
}

int main(){
   
    int new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int opt = 1;
    //sock_stream permette di aprire una connessione con protocollo di trasporto affidabile
    //af_inet si intende per dominio con indirizzo ip v4
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd<0){
        perror("error sokcet");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /*abilita il riutilizzo immediato dell'indirizzo (IP e porta) del server, 
    anche se precedentemente usato e in stato di attesa (TIME_WAIT) dopo la chiusura. 
    opt è un intero (solitamente 1 per abilitare). 
    Utile per riavviare rapidamente i server.*/
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr=INADDR_ANY; //si indica da qualsiasi sorgente
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    if(listen(server_fd, MAX_CLIENTS)<0){
        perror("listen error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto su porta %d...\n", PORT);

    while(1){
        new_socket = accept(server_fd, (struct sockaddr *)& address, &addrlen);
        if(new_socket<0){
            perror("accept error");
        }

        int *client_sock = malloc(sizeof(int));
        if (client_sock == NULL) {
            perror("Errore nell'allocazione della memoria per client_sock");
            close(new_socket);
            continue;
        }
        *client_sock = new_socket;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_sock) != 0) {
            perror("Errore nella creazione del thread");
            close(*client_sock);
            free(client_sock);
            continue;
        }        
        pthread_detach(tid);
    }

    

    




    return 0;
}