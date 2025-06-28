#!/bin/bash


# Funzione per pulire i container al termine
cleanup() {
    echo "🧹 Pulizia in corso..."
    echo "Fermando tutti i client..."
    for i in $(seq 1 $NUM_CLIENTS); do
        sudo docker stop tris-client-$i 2>/dev/null
        sudo docker rm tris-client-$i 2>/dev/null
    done
    echo "Cleaning up X11 permissions..."
    xhost -local:docker
    exit 0
}

# Gestisci CTRL+C per cleanup
trap cleanup SIGINT SIGTERM # Trap per pulire i container al termine. SIGINT e SIGTERM sono i segnali standard per l'interruzione del processo.

echo "🚀 === SISTEMA COMUNICAZIONE ROBOT MULTI-CLIENT ==="
echo

# Richiedi numero di client da avviare
if [ -z "$1" ]; then # -z : controlla se la variabile è vuota
    echo "💭 Quanti client vuoi avviare? (default: 1, max consigliato: 4)"
    read -p "Numero client: " NUM_CLIENTS
    NUM_CLIENTS=${NUM_CLIENTS:-1}
else
    NUM_CLIENTS=$1
fi

# Validazione input
if ! [[ "$NUM_CLIENTS" =~ ^[0-9]+$ ]] || [ "$NUM_CLIENTS" -lt 1 ] || [ "$NUM_CLIENTS" -gt 10 ]; then
    echo "❌ Errore: Il numero deve essere tra 1 e 10"
    exit 1
fi

echo "📊 Avvio di $NUM_CLIENTS client(s)..."
echo

echo "🔧 Setting up X11 permissions for Docker..."
# Allow X11 forwarding for docker
xhost +local:docker

# Export DISPLAY if not set
if [ -z "$DISPLAY" ]; then
    export DISPLAY=:0 # Imposta DISPLAY a :0 se non è già definito
fi

echo "📺 DISPLAY is set to: $DISPLAY"

# Create .Xauthority if it doesn't exist
if [ ! -f "$HOME/.Xauthority" ]; then
    touch "$HOME/.Xauthority"
    xauth generate :0 . trusted
fi

echo "🖥️  Starting server container first..."
sudo docker compose up -d server #-d # -d per eseguire in background

echo "⏳ Waiting for server to be ready..."
sleep 3

echo "🎮 Starting $NUM_CLIENTS client container(s)..."
echo

# Array per tenere traccia dei PID dei processi client
declare -a CLIENT_PIDS=()

# Avvia i client in parallelo
for i in $(seq 1 $NUM_CLIENTS); do
    echo "🚀 Avvio client #$i..."
    
    # Crea un container client con nome univoco
    sudo docker run --rm \
        --name tris-client-$i \
        --network lso_comunicazione_robot_tris-network \
        -e DISPLAY=$DISPLAY \
        -e SERVER_HOST=server \
        -e SERVER_PORT=8080 \
        -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
        -v $HOME/.Xauthority:/root/.Xauthority:rw \
        lso_comunicazione_robot-client &
    
    CLIENT_PIDS[$i]=$! # Salva il PID del processo in background
    
    # Piccola pausa tra i client per evitare conflitti
    sleep 1
done

echo
echo "✅ Tutti i $NUM_CLIENTS client sono stati avviati!"
echo "🔧 Container attivi:"
sudo docker ps --filter "name=tris-" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
echo
echo "📝 Per fermare tutti i client, premi CTRL+C"
echo "📊 Monitoring dei client in corso..."

# Attendi che tutti i client terminino
for i in $(seq 1 $NUM_CLIENTS); do
    wait ${CLIENT_PIDS[$i]} 2>/dev/null # Attendi il processo in background
done

cleanup
