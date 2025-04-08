import tkinter as tk
from tkinter import ttk
from client_network import send_to_server

class LoginPage(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller

        ttk.Label(self, text="Username:").grid(row=0, column=0, sticky=tk.W, pady=5)
        self.username_entry = ttk.Entry(self, width=40)
        self.username_entry.grid(row=0, column=1, pady=5)

        ttk.Label(self, text="Password:").grid(row=1, column=0, sticky=tk.W, pady=5)
        self.password_entry = ttk.Entry(self, width=40, show="*")
        self.password_entry.grid(row=1, column=1, pady=5)

        send_button = ttk.Button(self, text="Login", command=self.on_login_button_click)
        send_button.grid(row=2, column=1, pady=10, sticky=tk.E)

        self.status_label = ttk.Label(self, text="")
        self.status_label.grid(row=3, column=0, columnspan=2, pady=10)

    def on_login_button_click(self):
        """Gestisce il click sul pulsante di login."""
        username = self.username_entry.get()
        password = self.password_entry.get()

        if username and password:
            response = send_to_server("/login", {"username": username, "password": password})
            if response == "Login eseguito con successo":
                self.status_label.config(text=response, foreground="green")
                self.controller.show_frame("HomePage")
            else:
                self.status_label.config(text="Credenziali errate! Riprova", foreground="red")
        else:
            self.status_label.config(text="Inserisci sia username che password!", foreground="red")