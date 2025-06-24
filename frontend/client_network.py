import socket
import json
import select
import time

class SelectBasedClient:
    def __init__(self):
        self.socket = None
        self.connected = False
        
    def connect(self):
        """Stabilisce la connessione principale"""
        try:
            print("Tentativo di connessione al server...")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect(("127.0.0.1", 8080))
            self.connected = True
            print("Connessione stabilita!")
        except Exception as e:
            print(f"Errore connessione: {e}")
            self.connected = False
            
    def send_to_server(self, path, body):
        """Invia dati al server e attende risposta usando select"""
        if not self.connected or not self.socket:
            raise ConnectionError("Non connesso al server")
            
        try:
            messaggio = {
                "path": path,
                "body": json.dumps(body)
            }
            
            json_data = json.dumps(messaggio)
            print(f"📤 Invio: {json_data}")
            self.socket.sendall(json_data.encode())
            
            print("📥 Attendo risposta...")
            # Usa select per attendere la risposta con timeout
            ready, _, _ = select.select([self.socket], [], [], 5.0)  # timeout 5 secondi
            
            if ready:
                risposta = self.socket.recv(4096).decode()
                print(f"📥 Ricevuta: {risposta}")
                return risposta
            else:
                raise TimeoutError("Timeout in attesa della risposta dal server")
                
        except Exception as e:
            print(f"❌ Errore invio: {type(e).__name__}: {e}")
            self.connected = False
            raise
                
    def check_server_messages(self):
        """Controlla se ci sono messaggi dal server (non bloccante)"""
        if not self.connected or not self.socket:
            return []
            
        messages = []
        try:
            # Continua a leggere finché ci sono dati disponibili
            while True:
                # Usa select con timeout 0 per controllo non bloccante
                ready, _, _ = select.select([self.socket], [], [], 0)
                
                if ready:
                    data = self.socket.recv(4096).decode()
                    if not data:
                        break  # Connessione chiusa
                    
                    # Potrebbe esserci più di un messaggio JSON nel buffer
                    # Dividi per newline se i messaggi sono separati così
                    lines = data.strip().split('\n')
                    for line in lines:
                        if line.strip():
                            messages.append(line.strip())
                else:
                    break  # Nessun dato disponibile
                    
        except Exception as e:
            print(f"Errore ricezione: {e}")
            
        return messages
            
    def close(self):
        """Chiude la connessione"""
        self.connected = False
        if self.socket:
            self.socket.close()

# Nessuna istanza globale - ogni chiamata crea la sua connessione
_client = None

def connect_to_server():
    """Stabilisce la connessione al server"""
    global _client
    if _client is None:
        _client = SelectBasedClient()
    _client.connect()
    return _client.connected

def send_to_server(path, body):
    """Funzione di compatibilità per invio"""
    global _client
    if _client is None:
        raise ConnectionError("Client non inizializzato. Chiama connect_to_server() prima.")
    return _client.send_to_server(path, body)

def receive_from_server():
    """Funzione di compatibilità per ricezione (safe) - restituisce TUTTI i messaggi"""
    global _client
    if _client is None:
        return []
    return _client.check_server_messages()

def close_connections():
    """Chiude la connessione"""
    global _client
    if _client is not None:
        _client.close()
        _client = None
