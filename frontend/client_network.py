import socket
import json
import time
def send_to_server(path, body):
    messaggio = {
        "path": path,
        "body": json.dumps(body)
    }
    s= socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print("Tentativo di connessione al server...")
    s.connect(("127.0.0.1", 8080))
    print("Connessione stabilita!")
    json_data = json.dumps(messaggio)
    print(json_data)
    s.sendall(json_data.encode())
    risposta = s.recv(2048).decode()
    s.close()
    return risposta


