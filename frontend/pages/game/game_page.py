import json
import tkinter as tk
from tkinter import ttk, messagebox

from client_network import receive_from_server, send_to_server

class GamePage(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.configure(bg="black")

        # id della partita
        self.id = None
        self.nomePartecipante = ""
        self.TRIS = [[0 for _ in range(3)] for _ in range(3)]
        self.esito = None
        self.turno = None
        self.simbolo_assegnato = None
        
        # Layout
        self.rowconfigure(0, weight=0)  # Navbar
        self.rowconfigure(1, weight=0)  # Info giocatori
        self.rowconfigure(2, weight=0)  # Turno
        self.rowconfigure(3, weight=1)  # Griglia gioco
        self.columnconfigure(0, weight=1)

        # Navbar
        navbar = ttk.Frame(self, style="TFrame")
        navbar.grid(row=0, column=0, sticky="ew")
        navbar.columnconfigure(0, weight=0) #pulsante indietro
        navbar.columnconfigure(1, weight=1) #titolo centrato
        navbar.columnconfigure(2, weight=0)

        home_button = ttk.Button(
            navbar,
            text="<- Home",
            style="TButton",   
            command = self.torna_home
        )
        home_button.grid(row=0, column=0, padx=10, pady=5, sticky="w")


        # Titolo della pagina centrato
        title = ttk.Label(navbar, text="Partita in corso", style="Title.TLabel", justify="center")
        title.grid(row=0, column=1, pady=10)

        # Info giocatori
        self.players_label = ttk.Label(self, text="Partecipante: {self.controller.shared_data}", style="TLabel", justify="center")
        self.players_label.grid(row=1, column=0, pady=5)
        

        # Turno
        self.turn_label = ttk.Label(self, text="Turno:", style="TLabel", justify="center")
        self.turn_label.grid(row=2, column=0, pady=5)
        # Label Simbolo Assegnato (in basso a destra)
        self.simbolo_label = ttk.Label(self, text="Simbolo Assegnato: ", style="TLabel", justify="right")
        self.simbolo_label.place(relx=1.0, rely=1.0, anchor="se", x=-10, y=-10)

        # Griglia di gioco
        grid_frame = ttk.Frame(self, style="TFrame")
        grid_frame.grid(row=3, column=0, pady=20)

        self.buttons = [[None for _ in range(3)] for _ in range(3)]
        for i in range(3):
            grid_frame.rowconfigure(i, weight=1)
            for j in range(3):
                grid_frame.columnconfigure(j, weight=1)
                btn = ttk.Button(
                    grid_frame,
                    text="",
                    style="TButton",
                    command=lambda row=i, col=j: self.handle_click(row, col),
                    width=6
                )
                btn.grid(row=i, column=j, padx=5, pady=5, ipadx=10, ipady=10, sticky="nsew")
                self.buttons[i][j] = btn

        # Stato del gioco
        self.current_turn = None

    def update_data(self):
        self.simbolo_assegnato = self.controller.shared_data.get("simbolo", "")
        self.nomePartecipante = self.controller.shared_data.get("nickname_partecipante", "")
        self.id = self.controller.shared_data.get("game_id", "")
        self.player_id = self.controller.shared_data.get("player_id_partecipante", "")

        self._assegna_simbolo()
        self.aggiorna_dati(self.controller.shared_data.get("game_data", {}))

    

    def handle_click(self, row, col):
        self.send_move_to_server(row, col , self.id)

    def send_move_to_server(self,row, col, game_id):
        """Invia la mossa al server."""
        path = "/game_move"
        messaggio = {  
            "row": row,
            "col": col,
            "game_id": game_id  
        }
        risposta = send_to_server(path,messaggio)
        
        if risposta:
           try: 
                data = json.loads(risposta)
                if data.get("error"):
                    messagebox.showerror("Errore", data.get("error", "Errore sconosciuto"))
                else:
                    self.aggiorna_dati(data)
           except json.JSONDecodeError:
               print("Errore nella decodifica della risposta JSON.")
        else:
            print("Errore nell'invio della mossa al server.")

    def aggiorna_dati(self, dati_game):
        """Aggiorna gli attributi e la UI con i dati ricevuti dal server."""
        self.TRIS = dati_game.get("TRIS", [[0]*3 for _ in range(3)])
        if dati_game.get("esito") is not None: 
            self.esito = dati_game.get("esito")
        self.turno = dati_game.get("turno")
        if self._gestisci_esito(dati_game.get("messaggio", "")):
            return
        self._aggiorna_labels()
        self._aggiorna_griglia()

    def _gestisci_esito(self, messaggio):
        """Gestisce la visualizzazione dell'esito della partita."""
        if self.esito is not None and self.esito != 0:
            esito_text = "Partita terminata: "
            if self.esito == 1:
                esito_text += "Vittoria!"
            elif self.esito == 2:
                esito_text += "Sconfitta!"
            else:
                esito_text += "Pareggio!"
            messagebox.showinfo("Esito partita", esito_text)
            if( messaggio):
                self._gestisci_messaggio(messaggio)
            else:
                self.controller.show_frame("HomePage")
            return True
        return False

    def _aggiorna_labels(self):
        """Aggiorna le label dei partecipanti e del turno."""
        self.players_label.config(text=f"Partecipante: {self.nomePartecipante}")
        if self.turno == 0:
            simbolo = "✗"
        elif self.turno == 1:
            simbolo = "◯"
        else:
            simbolo = str(self.turno)
        self.turn_label.config(text=f"Turno: {simbolo}")
    
    def _assegna_simbolo(self):
        if self.simbolo_assegnato == 'X':
            simbolo = "✗"
        elif self.simbolo_assegnato == 'O':
            simbolo = "◯"
        else:
            simbolo = str(self.turno)
        self.simbolo_label.config(text=f"Simbolo Assegnato: {simbolo}")

    def _aggiorna_griglia(self):
        """Aggiorna la griglia di gioco."""
        for i in range(3):
            for j in range(3):
                value = self.TRIS[i][j]
                if value == 0:
                    text = ""
                elif value == 1:
                    text = "X"
                elif value == 2:
                    text = "O"
                else:
                    text = str(value)
                if(self.buttons[i][j]['text']!= text):
                    self.buttons[i][j].config(text=text)
    
    
    def torna_home(self):
        """Torna alla pagina principale."""
        result = messagebox.askyesno("Conferma", "Vuoi tornare alla pagina principale? Il tuo abbandono darà sconfitta a tavolino.")

        if result:
            self.id = None
            self.nomePartecipante = ""
            self.esito = None
            self.turno = None
            self.TRIS = [[0 for _ in range(3)] for _ in range(3)]
            send_to_server("/exit_game", {"game_id": self.id})
            self.controller.show_frame("HomePage")
    
    def _gestisci_messaggio(self, messaggio):
        """Richiede se vuole rifare una nuova partita."""
        result =messagebox.askyesno("Nuova Partita", messaggio)
        if result:
            self.reset_game()
            self.controller.show_frame("HomePage")
        else:
            self.torna_home()


    def reset_game(self):
        """Resetta lo stato del gioco."""
        self.id = None
        self.nomePartecipante = ""
        self.esito = None
        self.turno = None
        self.TRIS = [[0 for _ in range(3)] for _ in range(3)]
        self._aggiorna_griglia()
        self._aggiorna_labels()
        self.simbolo_label.config(text="Simbolo Assegnato: ")