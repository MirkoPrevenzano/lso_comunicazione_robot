"""Client di rete con gestione select() per comunicazione non-bloccante"""

import socket
import json
import select
import time
import os

BUFFER_SIZE = 4096

# Configurazione server da variabili d'ambiente
SERVER_HOST = os.getenv('SERVER_HOST', '127.0.0.1')
SERVER_PORT = int(os.getenv('SERVER_PORT', '8080'))

class SelectBasedClient:
    def __init__(self):
        self.socket = None
        self.connected = False
        self.receive_buffer = ""  # ✅ Buffer come attributo dell'istanza
        
    def connect(self):
        """Stabilisce la connessione principale"""
        try:
            print(f"Tentativo di connessione al server {SERVER_HOST}:{SERVER_PORT}...")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((SERVER_HOST, SERVER_PORT))
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
        """Riceve messaggi dal server con gestione del buffer migliorata"""
        if not self.connected or not self.socket:
            return []
            
        try:
            ready, _, _ = select.select([self.socket], [], [], 0)
            if not ready:
                return []
            
            new_data = self.socket.recv(BUFFER_SIZE).decode('utf-8')
            if not new_data:
                return []
            
            self.receive_buffer += new_data
            print(f"🔍 DEBUG: Buffer ricevuto: {repr(new_data)}")
            print(f"🔍 DEBUG: Buffer totale: {repr(self.receive_buffer)}")
            
            messages = []
            while True:
                try:
                    # Trova la fine del primo messaggio JSON completo
                    brace_count = 0
                    end_pos = -1
                    
                    for i, char in enumerate(self.receive_buffer):
                        if char == '{':
                            brace_count += 1
                        elif char == '}':
                            brace_count -= 1
                            if brace_count == 0:
                                end_pos = i + 1
                                break
                    
                    if end_pos == -1:
                        break
                    
                    # Estrai il messaggio JSON completo
                    json_message = self.receive_buffer[:end_pos].strip()
                    self.receive_buffer = self.receive_buffer[end_pos:].strip()
                    
                    if json_message:
                        print(f"🔍 DEBUG: Messaggio estratto: {repr(json_message)}")
                        try:
                            parsed = json.loads(json_message)
                            messages.append(parsed)
                            print(f"🔍 DEBUG: Messaggio parsato correttamente: {parsed}")
                        except json.JSONDecodeError as e:
                            print(f"❌ DEBUG: Errore parsing messaggio: {json_message}")
                            print(f"❌ DEBUG: Errore: {e}")
                    
                except Exception as e:
                    print(f"❌ DEBUG: Errore nel loop di parsing: {e}")
                    break
            
            print(f"🔍 DEBUG: Messaggi restituiti ({len(messages)}): {messages}")
            return messages
            
        except Exception as e:
            print(f"❌ Errore in check_server_messages: {type(e).__name__}: {e}")
            return []
    
    def close(self):
        """Chiude la connessione"""
        self.connected = False
        if self.socket:
            self.socket.close()

# Istanza globale del client
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
    """Funzione di compatibilità per ricezione"""
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