import tkinter as tk
from tkinter import ttk
from pages.login_page import LoginPage
from pages.home.home_page import HomePage
from client_network import connect_to_server, close_connections
from pages.game.game_page import GamePage
from server_polling import ServerPollingManager
import atexit

FONT_FAMILY = "Segoe UI"

class MainApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Client GUI")
        self.geometry("500x500") 
        self.resizable(False , False)

        #Dizionario per salvare info globali
        self.shared_data= {}
        
        # Inizializza il ServerPollingManager centralizzato
        self.server_polling = ServerPollingManager(self)
        
        # Imposta lo sfondo nero
        self.configure(bg="black")

        style = ttk.Style()
        style.theme_use("clam")  # Stile più moderno
        style.configure("TFrame", background="black")
        style.configure("Title.TLabel", background="black", foreground="white", font=(FONT_FAMILY, 18, "bold"))
        style.configure("TLabel", background="black", foreground="white", font=(FONT_FAMILY, 11))
        style.configure("Status.TLabel", background="black", foreground="white", font=(FONT_FAMILY, 10, "italic"))
        style.configure("TEntry", fieldbackground="black", foreground="white", insertcolor="white")
        style.configure("Accent.TButton", background="#2ecc71", foreground="white", font=(FONT_FAMILY, 10, "bold"))
        style.map("Accent.TButton",
                  background=[("active", "#27ae60")],
                  foreground=[("active", "white")])

        # Contenitore principale
        self.container = tk.Frame(self, bg="black")
        self.container.pack(fill="both", expand=True)

        self.frames = {}

        # Inizializza le pagine
        for page in (LoginPage, HomePage, GamePage):
            page_name = page.__name__
            frame = page(parent=self.container, controller=self)
            self.frames[page_name] = frame
            frame.grid(row=0, column=0, sticky="nsew")

        self.show_frame("LoginPage")




    def show_frame(self, page_name):
        """Mostra una pagina specifica."""
        # Memorizza il frame corrente per il ServerPollingManager
        self.current_frame = page_name
        
        frame = self.frames[page_name]
        frame.tkraise()
        if hasattr(frame, "update_data"):
            frame.update_data()
    
    def get_server_polling(self):
        """Restituisce il riferimento al ServerPollingManager centralizzato"""
        return self.server_polling
    
    def start_global_polling(self):
        """Avvia il polling del server centralizzato"""
        self.server_polling.start_listener()
    
    def stop_global_polling(self):
        """Ferma il polling del server centralizzato"""
        self.server_polling.stop_listener()
    
   
        


if __name__ == "__main__":
    # Stabilisci la connessione principale
    if not connect_to_server():
        print("Impossibile connettersi al server")
        exit(1)
    
    # Registra la chiusura delle connessioni all'uscita
    atexit.register(close_connections)
    
    # Avvia l'applicazione GUI
    app = MainApp()
    app.mainloop()