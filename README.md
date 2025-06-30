# 🎮 Sistema TRIS Multi-Client

Sistema Docker containerizzato per comunicazione tra server C e client Python GUI con supporto multi-client.

## 🔨 Costruzione Immagini

**IMPORTANTE**: Prima di avviare i client, costruisci le immagini Docker:

```bash
# Costruisci entrambe le immagini (server + client)
sudo docker compose --profile single-client build

# Oppure costruisci singolarmente
sudo docker build -f Dockerfile.server -t lso_comunicazione_robot-server .
sudo docker build -f Dockerfile.client -t lso_comunicazione_robot-client .
```

> ⚠️ **Nota sul Profilo**: Il client nel docker-compose.yml usa il profilo `single-client` per evitare conflitti con il sistema multi-client. Questo significa che `docker compose build` senza `--profile` costruisce solo il server.

## 🚀 Avvio Rapido

### Metodo 1: Script Docker Base
```bash
# Compila le immagini
./docker-run.sh build

# Avvia server + client
./docker-run.sh all

# Oppure avvia solo il client (server deve essere già attivo)
./docker-run.sh client
```

### Metodo 2: Script di Gestione Avanzato
```bash
# Mostra tutti i comandi disponibili
./manage_clients.sh help

# Avvia 2 client
./manage_clients.sh start 2

# Mostra stato del sistema
./manage_clients.sh status

# Ferma tutti i client
./manage_clients.sh stop
```

## 📋 Comandi Disponibili

### Script `docker-run.sh`
- **Build**: `./docker-run.sh build` (compila immagini)
- **Server**: `./docker-run.sh server` (avvia solo server)
- **Client**: `./docker-run.sh client` (avvia solo client)
- **All**: `./docker-run.sh all` (avvia server + client)
- **Stop**: `./docker-run.sh stop` (ferma tutti i container)
- **Clean**: `./docker-run.sh clean` (pulizia completa)
- **Logs**: `./docker-run.sh logs [service]` (mostra log)

### Script `manage_clients.sh`
```bash
./manage_clients.sh start [N]     # Avvia N client (default: 1)
./manage_clients.sh stop          # Ferma tutti i client
./manage_clients.sh status        # Mostra stato sistema
./manage_clients.sh logs [NAME]   # Mostra log container
./manage_clients.sh restart       # Riavvia server
./manage_clients.sh clean         # Pulizia completa
```



Il sistema gestisce automaticamente:
- **Permessi X11**: `xhost +local:docker`
- **Socket condiviso**: `/tmp/.X11-unix`
- **Autorizzazione**: `~/.Xauthority`
- **Variabile DISPLAY**: Auto-rilevamento

## 📊 Monitoraggio

### Controlla Container Attivi
```bash
sudo docker ps --filter "name=tris"
```

### Log del Server
```bash
sudo docker logs tris-server
```

### Log di un Client
```bash
sudo docker logs tris-client-1
```

## 🐛 Risoluzione Problemi

### GUI non si Apre
```bash
# Verifica X11
echo $DISPLAY
xeyes  # Test GUI

# Reset permessi X11
./docker-run.sh stop
xhost +local:docker
./docker-run.sh client
```

### Container non si Avviano
```bash
# Rebuild immagini
./docker-run.sh clean
./docker-run.sh build

# Verifica rete
sudo docker network ls | grep tris
```

### Pulizia completa
```bash
./docker-run.sh clean
```



## 📦 Prerequisiti Altri PC

```bash
# Installa Docker
sudo apt install docker.io docker-compose-plugin

# Installa X11
sudo apt install x11-xserver-utils xauth

# Copia progetto e esegui
git clone <repo> && cd <project>
chmod +x *.sh
./docker-run.sh build
./docker-run.sh all
```
