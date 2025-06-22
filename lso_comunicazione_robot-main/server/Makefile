# Variabili
CC = gcc
CFLAGS = -Wall -g

# File oggetto
OBJECTS = server.o player.o handler.o game.o request.o

# Nome dell'eseguibile finale
TARGET = server

# Regola principale per compilare l'eseguibile
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) -lcjson

# Regola per compilare il file server.o
server.o: server.c
	$(CC) $(CFLAGS) -c server.c -o server.o

# Regola per compilare il file player.o
player.o: player.c
	$(CC) $(CFLAGS) -c player.c -o player.o

# Regola per compilare il file handler.o
handler.o: handler.c
	$(CC) $(CFLAGS) -c handler.c -o handler.o

# Regola per compilare il file game.o
game.o: game.c
	$(CC) $(CFLAGS) -c game.c -o game.o

# Regola per compilare il file request.o
request.o: request.c
	$(CC) $(CFLAGS) -c request.c -o request.o

# Pulizia dei file oggetto e dell'eseguibile
clean:
	rm -fv $(OBJECTS) $(TARGET)