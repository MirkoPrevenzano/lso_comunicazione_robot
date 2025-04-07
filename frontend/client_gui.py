import tkinter as tk
from tkinter import ttk
from client_network import send_to_server

def on_send_button_click():
    
    path = path_entry.get()
    body = body_entry.get()
    if path and body:
        send_to_server(path, body)
        status_label.config(text="Richiesta inviata con successo!", foreground="green")
    else:
        status_label.config(text="Inserisci sia il path che il body!", foreground="red")

# Creazione della finestra principale
root = tk.Tk()
root.title("Client GUI")
root.geometry("400x300")

# Creazione dei widget
frame = ttk.Frame(root, padding="10")
frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

# Etichetta e campo di input per il path
ttk.Label(frame, text="Path:").grid(row=0, column=0, sticky=tk.W, pady=5)
path_entry = ttk.Entry(frame, width=40)
path_entry.grid(row=0, column=1, pady=5)

# Etichetta e campo di input per il body
ttk.Label(frame, text="Body:").grid(row=1, column=0, sticky=tk.W, pady=5)
body_entry = ttk.Entry(frame, width=40)
body_entry.grid(row=1, column=1, pady=5)

# Pulsante per inviare la richiesta
send_button = ttk.Button(frame, text="Invia", command=on_send_button_click)
send_button.grid(row=2, column=1, pady=10, sticky=tk.E)

# Etichetta per lo stato
status_label = ttk.Label(frame, text="")
status_label.grid(row=3, column=0, columnspan=2, pady=10)

# Avvio del loop principale
root.mainloop()