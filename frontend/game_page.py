import tkinter as tk
from tkinter import ttk

class GamePage(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.configure(bg="black")

        self.id = None
        self.nomePartecipante = ""
        self.TRIS = [[0 for _ in range(3)] for _ in range(3)]
        self.esito = None
        self.turno = None

        # Layout
        self.rowconfigure(0, weight=0)  # Navbar
        self.rowconfigure(1, weight=0)  # Info giocatori
        self.rowconfigure(2, weight=0)  # Turno
        self.rowconfigure(3, weight=1)  # Griglia gioco
        self.columnconfigure(0, weight=1)

        # Navbar
        navbar = ttk.Frame(self, style="TFrame")
        navbar.grid(row=0, column=0, sticky="ew")
        navbar.columnconfigure(0, weight=1)

        title = ttk.Label(navbar, text="Partita in corso", style="Title.TLabel", justify="center")
        title.grid(row=0, column=0, pady=10)

        # Info giocatori
        self.players_label = ttk.Label(self, text="Partecipante:", style="TLabel", justify="center")
        self.players_label.grid(row=1, column=0, pady=5)

        # Turno
        self.turn_label = ttk.Label(self, text="Turno:", style="TLabel", justify="center")
        self.turn_label.grid(row=2, column=0, pady=5)

        # Griglia di gioco
        grid_frame = ttk.Frame(self, style="TFrame")
        grid_frame.grid(row=3, column=0, pady=20)

        self.buttons = []
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
                self.buttons.append(btn)

        # Stato del gioco
        self.current_turn = None

    def handle_click(self, row, col):
        # Ignora se la cella è già stata cliccata
        print(f"Clicked on cell ({row}, {col})")

    def aggiorna_dati(self, dati_game):
        """Aggiorna gli attributi e la UI con i dati ricevuti dal server."""
        self.id = dati_game.get("id")
        self.nomePartecipante = dati_game.get("nomePartecipante", "")
        self.TRIS = dati_game.get("TRIS", [[0]*3 for _ in range(3)])
        self.esito = dati_game.get("esito")
        self.turno = dati_game.get("turno")

        # Aggiorna le label
        self.players_label.config(text=f"Partecipante: {self.nomePartecipante}")
        self.turn_label.config(text=f"Turno: {self.turno}")

        # Aggiorna la griglia
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
                self.buttons[i][j].config(text=text)