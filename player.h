#ifndef PLAYER_H
#define PLAYER_H

enum stato_giocatore {
    IN_GIOCO ,
    IN_HOME,

};
typedef struct {
    int socket;
    int id;
    char nome[30]; //nome di massimo 30 char
    enum stato_giocatore stato; // stato del giocatore

}GIOCATORE;

#endif