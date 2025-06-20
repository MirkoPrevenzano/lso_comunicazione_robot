import json
import tkinter as tk
from tkinter import ttk

from client_network import receive_from_server, send_to_server

class AttendPage(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.configure(bg="black")

        # Layout
        self.rowconfigure(0, weight=1)
        self.rowconfigure(1, weight=3)
        self.columnconfigure(0, weight=1)

        # Navbar con bottone indietro
        navbar = ttk.Frame(self, style="TFrame")
        navbar.grid(row=0, column=0, sticky="ew")
        navbar.columnconfigure(0, weight=0)
        navbar.columnconfigure(1, weight=1)

        back_button = ttk.Button(navbar, text="⬅", style="Accent.TButton", command=self.on_back)
        back_button.grid(row=0, column=0, padx=(10, 0), pady=10, sticky="w")

        # Testo centrale
        center_frame = ttk.Frame(self, style="TFrame")
        center_frame.grid(row=1, column=0, sticky="nsew")
        center_frame.columnconfigure(0, weight=1)
        center_frame.rowconfigure(0, weight=1)

        wait_label = ttk.Label(center_frame, text="In attesa di un giocatore...", style="Title.TLabel", justify="center")
        wait_label.grid(row=0, column=0, pady=80, padx=20, sticky="n")
        

    def update_data(self):
        self.ascolta_server()
    
    def on_back(self):
        """Gestisce il ritorno alla HomePage."""
        try:
            # Invia una richiesta al server per uscire dalla partita PRIMA di pulire i dati condivisi
            response = send_to_server("/left_game", {"game_id": self.controller.shared_data.get("game_id", "")})
            if response:
                try:
                    data = json.loads(response)
                    if data.get("success") == 1:
                        print("Uscito dalla partita con successo.")
                        self.controller.show_frame("HomePage")
                    else:
                        print("Errore nell'uscita dalla partita:", data.get("message", "Errore sconosciuto."))
                except json.JSONDecodeError:
                    print("Errore nella decodifica della risposta dal server.")
            else:
                print("Nessuna risposta dal server.")
        except Exception as e:
            print(f"Errore di connessione: {e}")
        finally:
            # Pulisce i dati condivisi SOLO dopo aver tentato la comunicazione col server
            self.controller.shared_data.clear()
    
    def ascolta_server(self):
        try:
            response = receive_from_server(timeout=5)  # 5 secondi timeout
            print("Ricevuto:", response)
            if response:
                
                    print("aaa", response)
                    data = json.loads(response)
                    print("Ricevuto dati dal server:", data)
                    if data.get("type") == "start_game":
                        # Salva i dati nella shared_data del controller
                        self.controller.shared_data["simbolo_assegnato"] = data.get("simbolo", "")
                        self.controller.shared_data["nomePartecipante"] = data.get("nickname_partecipante", "")
                        self.controller.shared_data["game_id"] = data.get("game_id", "")
                        self.controller.shared_data["game_data"] = data.get("game_data", {})
                    
                        # Passa a GamePage
                        self.controller.show_frame("GamePage")
                        print("Partita iniziata con successo.")
                        return  # Esci per evitare richiami multipli
                    else:
                        print("Errore nell'uscita dalla partita:", data.get("message", "Errore sconosciuto."))
            else:
                self.after(500, self.ascolta_server)  # Riprova dopo 500 ms se non c'è risposta  
        except Exception as e:
            print(f"Errore durante l'ascolto: {e}")
            self.after(1000, self.ascolta_server)  # Riprova dopo 1 secondo

