import json
import tkinter as tk
from tkinter import ttk, messagebox

from client_network import send_to_server

class GamePage(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.configure(bg="black")

        # id della partita
        self.id = None
        self.nomePartecipante = ""
        self.TRIS = [[0 for _ in range(3)] for _ in range(3)]
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
        grid_frame.grid(row=3, column=0, pady=20, sticky="")
        
        self.buttons = [[None for _ in range(3)] for _ in range(3)]
        for i in range(3):
            grid_frame.rowconfigure(i, weight=1, uniform="row")
            for j in range(3):
                grid_frame.columnconfigure(j, weight=1, uniform="col")
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
                if data.get("success") == 0:
                    messagebox.showerror("Errore", data.get("message", "Errore sconosciuto"))
                else:
                    self.aggiorna_dati(data.get("game_data", {}), 
                                       esito=data.get("esito"), 
                                       messaggio=data.get("messaggio"))
           except json.JSONDecodeError:
               print("Errore nella decodifica della risposta JSON.")
        else:
            print("Errore nell'invio della mossa al server.")

    def aggiorna_dati(self, dati_game, esito=None, messaggio=None):
        """Aggiorna gli attributi e la UI con i dati ricevuti dal server."""
        # TRIS arriva come string di 9 caratteri (es: "000120000")
        tris_string = dati_game.get("TRIS", "000000000")
        
        print(f"🔧 DEBUG: TRIS ricevuto: '{tris_string}'")
        
        # Converte la stringa di 9 caratteri in matrice 3x3
        self.TRIS = [[0 for _ in range(3)] for _ in range(3)]
        for i in range(9):
            row = i // 3  # posizione 0-2 → riga 0, posizione 3-5 → riga 1, etc.
            col = i % 3   # posizione 0,3,6 → col 0, posizione 1,4,7 → col 1, etc.
            self.TRIS[row][col] = int(tris_string[i])
            
        
        self.turno = dati_game.get("turno")
        if esito is not None and self._gestisci_esito(messaggio, esito):
            return
        self._aggiorna_labels()
        self._aggiorna_griglia()

    def _gestisci_esito(self, messaggio, esito):
        """Gestisce la visualizzazione dell'esito della partita."""
        if esito is not None and esito != 0:
            esito_text = "Partita terminata: "
            if esito == 1:
                esito_text += "Vittoria!"
            elif esito == 2:
                esito_text += "Pareggio!"
            else:
                esito_text += "Sconfitta!"
            messagebox.showinfo("Esito partita", esito_text)
            if( messaggio):
                self._gestisci_messaggio(messaggio, esito)
            else:
                self.controller.show_frame("HomePage")
            return True
        return False

    def _aggiorna_labels(self):
        """Aggiorna le label dei partecipanti e del turno."""
        # Gestione intelligente lunghezza nickname partecipante
        if len(self.nomePartecipante) <= 15:
            nome_display = self.nomePartecipante
        else:
            # Tronca nickname lunghi con "..."
            nome_display = self.nomePartecipante[:12] + "..."
        
        self.players_label.config(text=f"Partecipante: {nome_display}")
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
            self.nomePartecipante = ""
            self.turno = None
            self.TRIS = [[0 for _ in range(3)] for _ in range(3)]
            send_to_server("/exit_game", {"game_id": self.controller.shared_data['game_id']})
            self.id = None
            self.controller.show_frame("HomePage")
    
    def _gestisci_messaggio(self, messaggio, esito):
        """Richiede se vuole rifare una nuova partita."""
        result =messagebox.askyesno("Nuova Partita:", messaggio)
        print(f"🔧 DEBUG: Risposta alla richiesta di nuova partita: {result}")
        self.reset_game()
        if(esito == 1):
            path = "/vittoria_game"
            
            risposta =send_to_server(path, {"game_id": self.controller.shared_data['game_id'], "risposta": result})
            risposta = json.loads(risposta)
            if(risposta.get("success") == 1):
                messagebox.showinfo("Partita", risposta.get("message"))
            else:
                messagebox.showerror("Errore", risposta.get("message", "Errore sconosciuto"))
            self.controller.show_frame("HomePage")
            #il server deve semplicemente fare reset però in caso di successo deve inviare messaggio
            
        elif esito == 2:
            path = "/pareggio_game"
            risposta = send_to_server(path, {"game_id": self.controller.shared_data['game_id'], "risposta": result})
            risposta = json.loads(risposta)
            if risposta.get("success") == 1:
                messagebox.showinfo("Rivincita accettata", risposta.get("message"))
            else:
                messagebox.showerror("Errore", risposta.get("message", "Errore sconosciuto"))
                self.controller.show_frame("HomePage")
            

    def gestione_risposta_pareggio(self, data):
        """Gestisce la risposta del server per la rivincita dopo un pareggio"""
        if data.get("success") == 1:
            messagebox.showinfo("Rivincita accettata", data.get("message"))
            self.controller.shared_data['game_id'] = data.get("game_id", "")
            self.id = data.get("game_id", "")
            self.nomePartecipante = data.get("nome_partecipante", "")
        else:
            messagebox.showerror("Rivincita rifiutata", data.get("message", "Errore sconosciuto"))
            self.reset_game()
            self.controller.show_frame("HomePage")

    def reset_game(self):
        """Resetta lo stato del gioco."""
        self.id = None
        self.nomePartecipante = ""
        self.turno = None
        self.TRIS = [[0 for _ in range(3)] for _ in range(3)]
        self._aggiorna_griglia()
        self._aggiorna_labels()
        self.simbolo_label.config(text="Simbolo Assegnato: ")