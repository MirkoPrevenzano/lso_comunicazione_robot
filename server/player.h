#ifndef PLAYER_H
#define PLAYER_H

enum StatoGiocatore {
    IN_GIOCO ,
    IN_HOME,

};
typedef struct {
    int socket;
    int id;
    char nome[30]; //nome di massimo 30 char
    enum StatoGiocatore stato; // stato del giocatore

}GIOCATORE;

#endif