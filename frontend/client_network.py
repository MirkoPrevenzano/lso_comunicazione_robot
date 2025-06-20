import socket
import json

s= socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("Tentativo di connessione al server...")
s.connect(("127.0.0.1", 8080))
print("Connessione stabilita!")
def send_to_server(path, body):
    messaggio = {
        "path": path,
        "body": json.dumps(body)
    }
    
    json_data = json.dumps(messaggio)
    print(json_data)
    s.sendall(json_data.encode())
    risposta = s.recv(4096).decode()
    return risposta

def receive_from_server():
    s.setblocking(False)  # Modalità bloccante: aspetta finché arrivano dati
    try:
        data = s.recv(4096).decode()
        if not data:
            return None
        return data
    except Exception as e:
        return None
    finally:
        s.setblocking(True)  # Torna alla modalità non bloccante
