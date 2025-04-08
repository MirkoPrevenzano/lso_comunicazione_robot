import tkinter as tk
from tkinter import ttk

class HomePage(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller

        ttk.Label(self, text="Benvenuto nella Home Page!").pack(pady=20)

        back_button = ttk.Button(self, text="Logout", command=lambda: controller.show_frame("LoginPage"))
        back_button.pack()