"""Gestione dei widget per la home page"""

import tkinter as tk
from tkinter import ttk
import traceback

# Importa le costanti necessarie
try:
    from constants import CARD_FRAME_STYLE, STATO_IN_ATTESA
except ImportError:
    # Fallback se constants.py non è disponibile
    CARD_FRAME_STYLE = "Card.TFrame"
    STATO_IN_ATTESA = "in_attesa"


class HomeWidgets:
    """Gestisce la creazione di tutti i widget per la home page"""
    
    def __init__(self, home_page):
        self.home_page = home_page
    
    def add_waiting_game_widget(self, game):
        """Aggiunge un widget per una partita in attesa (solo label cliccabili)"""
        try:
            print(f"⚙ DEBUG: Inizio creazione widget per {game}")
            
            if not hasattr(self.home_page, 'content_container') or not self.home_page.content_container:
                print("✗ content_container non disponibile")
                return
            
            # Crea il frame principale
            frame = ttk.Frame(self.home_page.content_container, relief="raised", borderwidth=2)
            frame.pack(fill="x", padx=4, pady=2)
            
            # Crea la label con le informazioni
            id_partita = game.get('id_partita', 'N/A')
            
            # Gestione intelligente layout con nome giocatore su riga separata
            proprietario = game.get('proprietario', 'Sconosciuto')
           
            
            
            # Layout su due righe: nome giocatore sopra, info partita sotto
            info = f"Giocatore: {proprietario}\nID: {id_partita} | ⟳ CLICCA QUI PER UNIRTI"
            
            # Una sola label cliccabile invece di label + bottone
            label = ttk.Label(frame, text=info, cursor="hand2", 
                            background="black", relief="raised", 
                            padding=(10, 5), justify="left")
            label.pack(fill="x", padx=5, pady=5)
            
            # Bind del click direttamente sulla label
            game_id = game['id_partita']
            label.bind("<Button-1>", lambda e, gid=game_id: self.home_page.actions.add_request(gid))
            
            print(f"⚙ DEBUG: Widget semplificato creato con successo per {game}")
            
            # Forza aggiornamento
            frame.update_idletasks()
            
        except Exception as e:
            print(f"✗ Errore creazione widget semplificato: {type(e).__name__}: {e}")
            traceback.print_exc()

    def add_received_request_widget(self, request):
        """Versione semplificata senza bottoni TTK (solo label)"""
        try:
            frame = ttk.Frame(self.home_page.content_container, relief="raised", borderwidth=1)
            frame.pack(fill="x", padx=4, pady=2)
            
            # Gestione intelligente lunghezza nickname mittente con a capo
            mittente_originale = request.get('mittente', 'Sconosciuto')
            game_id = request.get('game_id', 'N/A')
            player_id = request.get('player_id', 'N/A')
            
            info = f"Da: {mittente_originale} \n Partita: {game_id} | ✉ Richiesta di sfida"
            
            
            label = ttk.Label(frame, text=info, justify="left")
            label.pack(side="left", padx=10, pady=5, fill="x", expand=True)
            
            # Due label cliccabili invece di bottoni
            accept_label = ttk.Label(frame, text="✓ ", cursor="hand2", 
                                background="green", foreground="white", 
                                relief="raised", padding=(5, 2))
            accept_label.pack(side="right", padx=2)
            accept_label.bind("<Button-1>", 
                            lambda e, pid=player_id, gid=game_id: self.home_page.actions.accept_request(pid, gid))
            
            decline_label = ttk.Label(frame, text="✗ ", cursor="hand2",
                                    background="red", foreground="white",
                                    relief="raised", padding=(5, 2))
            decline_label.pack(side="right", padx=2)  
            decline_label.bind("<Button-1>",
                            lambda e, pid=player_id, gid=game_id: self.home_page.actions.decline_request(pid, gid))
            
            frame.update_idletasks()
            
        except Exception as e:
            print(f"✗ Errore widget semplice: {e}")

    def add_sent_request_widget(self, request):
        """Aggiunge un widget per una richiesta effettuata"""
        try:
            frame = ttk.Frame(self.home_page.content_container, style=CARD_FRAME_STYLE, relief="raised", borderwidth=1)
            frame.pack(fill="x", padx=4, pady=2)
            
            request_stato = request.get('stato', STATO_IN_ATTESA)
            
            # Determina l'icona in base allo stato
            if request_stato == STATO_IN_ATTESA or request_stato is None:
                status_icon = "⟳"
            elif request_stato == "declined":
                status_icon = "✗"
            elif request_stato == "accepted":
                status_icon = "✓"
            else:
                # Stato sconosciuto - icona di default
                status_icon = "?"
                
            if request_stato in [STATO_IN_ATTESA, None]:
                info = f"Partita: {request['game_id']} | {status_icon} {request_stato.title()} | ♻ CLICCA PER ANNULLARE"
                label = ttk.Label(frame, text=info, style="TLabel", cursor="hand2", relief="raised")
                label.bind("<Button-1>", 
                          lambda e, pid=request.get('player_id'), gid=request.get('game_id'): 
                          self.home_page.actions.cancel_request(pid, gid))
            else:   
                info = f"Partita: {request['game_id']} | {status_icon} {request_stato.title()}"
                label = ttk.Label(frame, text=info, style="TLabel")
                
            label.pack(side="left", padx=10, pady=5)
            
            # Forza aggiornamento
            frame.update_idletasks()
            
        except Exception as e:
            print(f"✗ Errore creazione widget richiesta inviata: {type(e).__name__}: {e}")
            traceback.print_exc()


class HomeScrolling:
    """Gestisce le funzionalità di scrolling per la home page"""
    
    def __init__(self, home_page):
        self.home_page = home_page
    
    def on_frame_configure(self, event):
        """Aggiorna la scrollregion quando il contenuto cambia (PROTETTO)"""
        try:
            if hasattr(self.home_page, 'canvas') and self.home_page.canvas:
                bbox = self.home_page.canvas.bbox("all")
                if bbox is not None:
                    self.home_page.canvas.configure(scrollregion=bbox)
                else:
                    # Se bbox è None, usa una scrollregion di default
                    self.home_page.canvas.configure(scrollregion=(0, 0, 0, 0))
        except Exception as e:
            print(f"✗ Errore in on_frame_configure: {e}")

    def on_mousewheel(self, event):
        """Gestisce lo scroll con la rotella del mouse"""
        try:
            if hasattr(self.home_page, 'canvas') and self.home_page.canvas:
                self.home_page.canvas.yview_scroll(int(-1*(event.delta/120)), "units")
        except Exception as e:
            print(f"✗ Errore in on_mousewheel: {e}")
