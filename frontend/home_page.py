import tkinter as tk
from tkinter import ttk

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

        back_button = ttk.Button(navbar, text="⬅", command=lambda: controller.show_frame("LoginPage"), style="Accent.TButton")
        back_button.grid(row=0, column=0, padx=10, pady=4, sticky="w")

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

    def update_data(self):
        nickname = self.controller.shared_data.get('nickname', "Utente")
        self.welcome_label.config(text=f"Benvenuto, {nickname}!")

    def new_game(self):
        print("Creo nuova partita")
