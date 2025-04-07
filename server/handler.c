#include <stdio.h>
#include <cjson/cJSON.h>
#include "handler.h"

void handle_prestito(cJSON *body){
    if (body && cJSON_IsString(body)) {
        printf("Dati ricevuti per prestito: %s\n", body->valuestring);
    } else {
        printf("Body non valido per prestito\n");
    }
}

void handle_restituzione(cJSON *body){
    if (body && cJSON_IsString(body)) {
        printf("Dati ricevuti per restituzione: %s\n", body->valuestring);
    } else {
        printf("Body non valido per restituzione\n");
    }
}

bool handle_login(cJSON *body) {
    if (body && cJSON_IsString(body)) {
        printf("Dati ricevuti per login: %s\n", body->valuestring);
    } else {
        printf("Body non valido per login\n");
    }
    return true;
}    


cJSON *handle_get_film(cJSON *body){
    if (body && cJSON_IsString(body)) {
        printf("Dati ricevuti per get_film: %s\n", body->valuestring);
    } else {
        printf("Body non valido per get_film\n");
    }
    return NULL;  
}

void handle_send_msg(cJSON *body){
    if (body && cJSON_IsString(body)) {
        printf("Dati ricevuti per send_msg: %s\n", body->valuestring);
    } else {
        printf("Body non valido per send_msg\n");
    }
    
}

bool handle_check_prestito(cJSON *body){
    if (body && cJSON_IsString(body)) {
        printf("Dati ricevuti per prestito: %s\n", body->valuestring);
    } else {
        printf("Body non valido per prestito\n");
    }
    return true;
}

cJSON *handle_get_msg(cJSON *body){
    if (body && cJSON_IsString(body)) {
        printf("Dati ricevuti per get_msg: %s\n", body->valuestring);
    } else {
        printf("Body non valido per get_msg\n");
    }
    return NULL;
}