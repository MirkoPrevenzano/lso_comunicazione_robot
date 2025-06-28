#!/bin/bash

# Script per gestire i container Docker

# Verifica che Docker sia disponibile
if ! command -v docker &> /dev/null; then
    echo "❌ Docker non è installato"
    exit 1
fi

# Verifica che Docker daemon sia in esecuzione
if ! docker info &> /dev/null; then
    echo "❌ Docker daemon non è in esecuzione"
    echo "💡 Prova: sudo systemctl start docker"
    exit 1
fi

case "$1" in
    "build")
        echo "🔨 Building containers..."
        docker compose build
        ;;
    "server")
        echo "🚀 Starting server..."
        docker compose up server
        ;;
    "client")
        echo "🖥️  Starting client..."
        # Permetti al client di accedere al display X11
        xhost +local:docker 2>/dev/null || echo "⚠️  xhost non disponibile, GUI potrebbe non funzionare"
        docker compose --profile client up client
        ;;
    "all")
        echo "🚀 Starting server and client..."
        xhost +local:docker 2>/dev/null || echo "⚠️  xhost non disponibile, GUI potrebbe non funzionare"
        docker compose --profile client up
        ;;
    "stop")
        echo "🛑 Stopping containers..."
        docker compose down
        ;;
    "clean")
        echo "🧹 Cleaning up..."
        docker compose down
        docker compose rm -f
        docker system prune -f
        ;;
    "logs")
        if [ -z "$2" ]; then
            echo "📋 Showing all logs..."
            docker compose logs -f
        else
            echo "📋 Showing logs for $2..."
            docker compose logs -f "$2"
        fi
        ;;
    *)
        echo "🐳 Docker Compose Manager per Tris Game"
        echo ""
        echo "Utilizzo: $0 {build|server|client|all|stop|clean|logs [service]}"
        echo ""
        echo "Comandi:"
        echo "  build   - Compila le immagini Docker"
        echo "  server  - Avvia solo il server"
        echo "  client  - Avvia solo il client (richiede server in esecuzione)"
        echo "  all     - Avvia server e client insieme"
        echo "  stop    - Ferma tutti i container"
        echo "  clean   - Pulisce container e immagini"
        echo "  logs    - Mostra i log (opzionale: specifica servizio)"
        echo ""
        exit 1
        ;;
esac