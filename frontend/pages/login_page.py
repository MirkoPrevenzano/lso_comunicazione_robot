import tkinter as tk
from tkinter import ttk
import json
from client_network import send_to_server

class LoginPage(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller

        self.configure(bg="black")
        self.columnconfigure(0, weight=1)
        self.rowconfigure(0, weight=1)

        content_frame = ttk.Frame(self, style="TFrame", padding=30)
        content_frame.grid(row=0, column=0, sticky="nsew")
        content_frame.columnconfigure(0, weight=1)

        title_label = ttk.Label(
            content_frame, 
            text="Benvenuto al gioco del \nTRIS!\n\n\nInserisci il tuo nickname", 
            style="Title.TLabel",
            justify="center"
        )
        title_label.grid(row=0, column=0, pady=(0, 30))

        input_frame = ttk.Frame(content_frame, style="TFrame")
        input_frame.grid(row=1, column=0, sticky="ew", pady=10)
        input_frame.columnconfigure(0, weight=1)

        # Registra la funzione di validazione per limitare a 30 caratteri
        vcmd = (self.register(self.validate_nickname), '%P')
        
        self.username_entry = ttk.Entry(
            input_frame, 
            width=30, 
            font=("Segoe UI", 12),
            validate='key',
            validatecommand=vcmd
        )
        self.username_entry.grid(row=0, column=0, sticky="ew", ipady=5)

        send_button = ttk.Button(
            input_frame, 
            text="->", 
            command=self.on_login_button_click, 
            style="Accent.TButton"
        )
        send_button.grid(row=0, column=1, padx=(10, 0), ipadx=1, ipady=1)

        self.status_label = ttk.Label(content_frame, text="", style="Status.TLabel", justify="center")
        self.status_label.grid(row=2, column=0, pady=(20, 0), sticky="ew")

    def validate_nickname(self, value):
        """Valida che il nickname non superi i 30 caratteri"""
        return len(value) <= 30

    def on_login_button_click(self):
        username = self.username_entry.get().strip()  # Rimuove spazi all'inizio e fine
        
        # Validazione della lunghezza del nickname
        if not username:
            self.status_label.config(text="Inserisci un nickname!", foreground="red")
            return
        
        if len(username) > 30:
            self.status_label.config(text="Il nickname deve essere al massimo 30 caratteri!", foreground="red")
            return

        response = send_to_server("/register", {"nickname": username})

        try:
            # Il server restituisce una stringa JSON, dobbiamo parsarla
            response_data = json.loads(response)
            if isinstance(response_data, dict) and 'id' in response_data:
                player_id = int(response_data['id'])
            else:
                player_id = -1
        except (json.JSONDecodeError, TypeError):
            player_id = -1
            
        # Gestione del fallimento della registrazione
        if(player_id > -1):
            self.status_label.config(text="Login eseguito con successo", foreground="green")
            self.controller.shared_data['nickname'] = username
            self.controller.shared_data['player_id'] = player_id
            self.controller.show_frame("HomePage")
        else:
            self.status_label.config(text="Errore nel login - Chiusura applicazione...", foreground="red")
            
            # Attendi un momento per mostrare il messaggio, poi chiudi
            self.after(2000, self.close_application)

    def close_application(self):
        """Chiude l'applicazione in modo sicuro"""
        try:
            # Chiudi le connessioni di rete se esistono
            from client_network import close_connections
            close_connections()
        except Exception as e:
            print(f"Errore durante la chiusura delle connessioni: {e}")
        
        # Chiudi la finestra principale
        try:
            self.controller.quit()  # Esce dal mainloop
            self.controller.destroy()  # Distrugge la finestra
        except Exception as e:
            print(f"Errore durante la chiusura dell'interfaccia: {e}")
        
        print("👋 Applicazione chiusa")
