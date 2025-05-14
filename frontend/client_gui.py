import tkinter as tk
from tkinter import ttk
from login_page import LoginPage
from home_page import HomePage

class MainApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Client GUI")
        self.geometry("530x400")
        self.resizable(False , False)

        #Dizionario per salvare info globali
        self.shared_data= {}
        # Imposta lo sfondo nero
        self.configure(bg="black")

        style = ttk.Style()
        style.theme_use("clam")  # Stile più moderno
        style.configure("TFrame", background="black")
        style.configure("Title.TLabel", background="black", foreground="white", font=("Segoe UI", 18, "bold"))
        style.configure("TLabel", background="black", foreground="white", font=("Segoe UI", 11))
        style.configure("Status.TLabel", background="black", foreground="white", font=("Segoe UI", 10, "italic"))
        style.configure("TEntry", fieldbackground="black", foreground="white", insertcolor="white")
        style.configure("Accent.TButton", background="#2ecc71", foreground="white", font=("Segoe UI", 10, "bold"))
        style.map("Accent.TButton",
                  background=[("active", "#27ae60")],
                  foreground=[("active", "white")])

        # Contenitore principale
        self.container = tk.Frame(self, bg="black")
        self.container.pack(fill="both", expand=True)

        self.frames = {}

        # Inizializza le pagine
        for page in (LoginPage, HomePage):
            page_name = page.__name__
            frame = page(parent=self.container, controller=self)
            self.frames[page_name] = frame
            frame.grid(row=0, column=0, sticky="nsew")

        self.show_frame("LoginPage")

    def show_frame(self, page_name):
        """Mostra una pagina specifica."""
        frame = self.frames[page_name]
        frame.tkraise()
        if hasattr(frame, "update_data"):
            frame.update_data()


if __name__ == "__main__":
    app = MainApp()
    app.mainloop()