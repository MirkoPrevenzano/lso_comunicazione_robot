import json
import tkinter as tk
from tkinter import ttk

from client_network import send_to_server

class HomePage(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.configure(bg="black")

        # Configura il layout a griglia della pagina principale
        self.rowconfigure(0, weight=1)  # Navbar
        self.rowconfigure(1, weight=0)  # Separatore
        self.rowconfigure(2, weight=1)  # Sezione nuova partita
        self.rowconfigure(3, weight=0)  # Separatore
        self.rowconfigure(4, weight=3)  # Sezione partite in attesa
        self.columnconfigure(0, weight=1)

        # Navbar
        navbar = ttk.Frame(self, style="TFrame")
        navbar.grid(row=0, column=0, sticky="nsew")

        navbar.columnconfigure(0, weight=0)
        navbar.columnconfigure(1, weight=1)


        navbar_label = ttk.Label(navbar, text="Tris", justify="center", style="Title.TLabel")
        navbar_label.grid(row=0, column=1, pady=(10, 5), sticky="n")

        # Separatore
        separator = ttk.Separator(self, orient="horizontal")
        separator.grid(row=1, column=0, sticky="ew", padx=(30,0),pady=(0, 10))

        # Sezione nuova partita
        section1 = ttk.Frame(self, style="TFrame")
        section1.grid(row=2, column=0, sticky="nsew", padx=20, pady=(0, 10))

        section1.columnconfigure(0, weight=1)

        self.welcome_label = ttk.Label(section1, text="Benvenuto", style="Title.TLabel", justify="center")
        self.welcome_label.pack(pady=(20, 10) , padx=(30,0))

        button_new_game = ttk.Button(section1, text="Nuova partita", style="Accent.TButton", command=self.new_game)
        button_new_game.pack(pady=(10,0), padx=(30,0))
        # Separatore

        separator = ttk.Separator(self, orient="horizontal")
        separator.grid(row=3, column=0, sticky="ew", padx=(30,0),pady=(0, 5))

        # Sezione partite in attesa
        section2 = ttk.Frame(self, style="TFrame")
        section2.grid(row=4, column=0, sticky="nsew", padx=20, pady=(0, 5))
        # Placeholder contenuto
        section2_label = ttk.Label(section2, text="Partite in attesa...", style="TLabel")
        section2_label.pack(padx=(30, 0))
        self.section2_label = section2_label

        self.games_container = ttk.Frame(section2, style="TFrame")
        self.games_container.pack(fill="both", expand=True, padx=10, pady=5)



    def periodic_update_waiting_games(self):
        # Salva l'id del timer per poterlo cancellare quando si cambia frame
        self._waiting_games_after_id = self.after(2000, self.periodic_update_waiting_games)
        self.update_waiting_games()

    def stop_periodic_update_waiting_games(self):
        # Ferma il timer se attivo
        if hasattr(self, '_waiting_games_after_id'):
            self.after_cancel(self._waiting_games_after_id)
            del self._waiting_games_after_id

    def update_data(self):
        nickname = self.controller.shared_data.get('nickname', "Utente")
        self.welcome_label.config(text=f"Benvenuto, {nickname}!")
        self.update_waiting_games()
        self.periodic_update_waiting_games()

    
   
        
        
    
    def update_waiting_games(self):
        try:
            games = send_to_server("/waiting_games", {})
            print("Risposta server:", repr(games))
            try:
                games = json.loads(games)
                games = games.get("partite", [])
            except Exception as e:
                print("Errore parsing JSON:", e)
                games = []
        except Exception as e:
            print("Errore di connessione o BrokenPipe:", e)
            games = []

        for widget in self.games_container.winfo_children():
            widget.destroy()
        if not games:
            ttk.Label(self.games_container, text="Nessuna partita in attesa", style="TLabel").pack(pady=(10, 0))
            return 
        for game in games:
            self.add_waiting_game_widget(game)

    def add_waiting_game_widget(self, game):
        frame = ttk.Frame(self.games_container, style="Card.TFrame", relief="raised", borderwidth=1)
        frame.pack(fill="x", padx=4, pady=2)
        info = f"ID: {game['id_partita']} | Giocatore: {game['proprietario']} | ⏳ In attesa"
        label = ttk.Label(frame, text=info, style="TLabel")
        label.pack(side="left", padx=10, pady=5)
        frame.bind("<Button-1>", lambda e, gid=game['id_partita']: self.join_game(gid))
        label.bind("<Button-1>", lambda e, gid=game['id_partita']: self.join_game(gid))

    def new_game(self):
        response = send_to_server("/new_game", {"nickname": self.controller.shared_data['nickname']})
        #self.update_waiting_games()
        print(response)
        try:
            data = json.loads(response)
            if data.get("success") == 1:
                game_id = data.get("id_game")
                self.controller.shared_data['game_id'] = game_id
                self.stop_periodic_update_waiting_games()
                self.controller.show_frame("AttendPage")
            else:
                self.welcome_label.config(text="Errore nella creazione della partita", foreground="red")
        except json.JSONDecodeError:
            self.welcome_label.config(text="Errore nella risposta del server", foreground="red")
        except Exception as e:
            self.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")
            print(f"Errore: {str(e)}")
        
    def join_game(self, game_id):
        #considera di non passare il nickname
        response = send_to_server("/join_game", {"game_id": game_id, "nickname": self.controller.shared_data['nickname']}) 
        print(response)
        try:
            data = json.loads(response)
            if data.get("success") == 1:
                self.welcome_label.config(text="Partita unita con successo", foreground="green")
                self.controller.shared_data['game_id'] = game_id
                self.stop_periodic_update_waiting_games()
                self.controller.show_frame("GamePage")
            else:
                errore = data.get("message", "Errore nell'unione alla partita")
                self.welcome_label.config(text=errore, foreground="red")
        except Exception as e:
            self.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")