#!/bin/bash

# Script avanzato per gestire i client del sistema comunicazione robot

show_help() {
    echo "🎮 === GESTORE CLIENT SISTEMA ROBOT ==="
    echo
    echo "Uso: $0 [COMANDO] [OPZIONI]"
    echo
    echo "COMANDI:"
    echo "  start [N]     - Avvia N client (default: 1)"
    echo "  stop          - Ferma tutti i client"
    echo "  status        - Mostra stato di server e client"
    echo "  logs [NAME]   - Mostra log di un container specifico"
    echo "  restart       - Riavvia il server"
    echo "  clean         - Pulisce tutti i container e immagini"
    echo "  help          - Mostra questo aiuto"
    echo
    echo "ESEMPI:"
    echo "  $0 start 3           # Avvia 3 client"
    echo "  $0 stop              # Ferma tutti i client"
    echo "  $0 logs tris-server  # Mostra log del server"
    echo "  $0 status            # Mostra stato sistema"
}

setup_x11() {
    echo "🔧 Configurazione X11..."
    xhost +local:docker
    
    if [ -z "$DISPLAY" ]; then
        export DISPLAY=:0
    fi
    
    if [ ! -f "$HOME/.Xauthority" ]; then
        touch "$HOME/.Xauthority"
        xauth generate :0 . trusted
    fi
}

start_server() {
    echo "🖥️  Avvio server..."
    sudo docker compose up -d server
    sleep 2
    
    if sudo docker ps | grep -q tris-server; then
        echo "✅ Server avviato con successo!"
    else
        echo "❌ Errore nell'avvio del server"
        return 1
    fi
}

start_clients() {
    local num_clients=${1:-1}
    
    echo "🎮 Avvio di $num_clients client(s)..."
    
    # Verifica che il server sia attivo
    if ! sudo docker ps | grep -q tris-server; then
        echo "⚠️  Server non attivo, avvio in corso..."
        start_server || return 1
    fi
    
    setup_x11
    
    for i in $(seq 1 $num_clients); do
        echo "🚀 Avvio client #$i..."
        
        sudo docker run -d \
            --name tris-client-$i \
            --network lso_comunicazione_robot_tris-network \
            -e DISPLAY=$DISPLAY \
            -e SERVER_HOST=server \
            -e SERVER_PORT=8080 \
            -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
            -v $HOME/.Xauthority:/root/.Xauthority:rw \
            lso_comunicazione_robot-client
        
        sleep 1
    done
    
    echo "✅ $num_clients client(s) avviati!"
}

stop_clients() {
    echo "🛑 Fermando tutti i client..."
    
    # Trova tutti i container client
    local clients=$(sudo docker ps -a --filter "name=tris-client" --format "{{.Names}}")
    
    if [ -z "$clients" ]; then
        echo "ℹ️  Nessun client da fermare"
        return 0
    fi
    
    echo "📋 Client trovati: $clients"
    
    for client in $clients; do
        echo "🛑 Fermando $client..."
        sudo docker stop $client
        sudo docker rm $client
    done
    
    echo "🧹 Pulizia X11..."
    xhost -local:docker
    
    echo "✅ Tutti i client sono stati fermati!"
}

show_status() {
    echo "📊 === STATO SISTEMA ==="
    echo
    echo "🖥️  SERVER:"
    if sudo docker ps | grep -q tris-server; then
        echo "✅ Server ATTIVO"
        sudo docker ps --filter "name=tris-server" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
    else
        echo "❌ Server NON ATTIVO"
    fi
    
    echo
    echo "🎮 CLIENT:"
    local clients=$(sudo docker ps --filter "name=tris-client" --format "{{.Names}}")
    if [ -n "$clients" ]; then
        echo "✅ Client ATTIVI:"
        sudo docker ps --filter "name=tris-client" --format "table {{.Names}}\t{{.Status}}"
    else
        echo "❌ Nessun client attivo"
    fi
    
    echo
    echo "🌐 RETE:"
    sudo docker network ls | grep tris-network || echo "❌ Rete non trovata"
}

show_logs() {
    local container_name=$1
    
    if [ -z "$container_name" ]; then
        echo "📋 Container disponibili:"
        sudo docker ps -a --filter "name=tris" --format "{{.Names}}"
        echo
        echo "Uso: $0 logs [NOME_CONTAINER]"
        return 1
    fi
    
    if sudo docker ps -a | grep -q $container_name; then
        echo "📄 Log di $container_name:"
        sudo docker logs $container_name
    else
        echo "❌ Container '$container_name' non trovato"
    fi
}

restart_server() {
    echo "🔄 Riavvio server..."
    sudo docker compose restart server
    echo "✅ Server riavviato!"
}

clean_all() {
    echo "🧹 Pulizia completa del sistema..."
    
    # Ferma tutti i container
    stop_clients
    sudo docker compose down
    
    # Rimuovi le immagini
    echo "🗑️  Rimozione immagini..."
    sudo docker rmi lso_comunicazione_robot-server lso_comunicazione_robot-client 2>/dev/null || true
    
    # Pulisci rete
    sudo docker network prune -f
    
    echo "✅ Pulizia completata!"
}

# Main script logic
case "$1" in
    "start")
        start_clients $2
        ;;
    "stop")
        stop_clients
        ;;
    "status")
        show_status
        ;;
    "logs")
        show_logs $2
        ;;
    "restart")
        restart_server
        ;;
    "clean")
        clean_all
        ;;
    "help"|"-h"|"--help"|"")
        show_help
        ;;
    *)
        echo "❌ Comando sconosciuto: $1"
        echo "Usa '$0 help' per vedere i comandi disponibili"
        exit 1
        ;;
esac
