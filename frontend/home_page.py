import json
import tkinter as tk
from tkinter import ttk
import time

from client_network import send_to_server, receive_from_server, connect_to_server, close_connections

# Costanti per stili
CARD_FRAME_STYLE = "Card.TFrame"
ACCENT_BUTTON_STYLE = "Accent.TButton"

# Costanti per stati richieste
STATO_IN_ATTESA = "in attesa"

# Costanti per messaggi di errore
ERROR_SERVER_RESPONSE = "Errore nella risposta del server"
ERROR_CONNECTION = "Errore di connessione o BrokenPipe"

class HomePage(tk.Frame):
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.configure(bg="black")
        self.current_view = "waiting_games"
        
        # Liste locali per le richieste
        self.received_requests = []
        self.sent_requests = []
        
        # Variabili per il polling del server
        self.listener_active = False
        self.server_polling_id = None
        
        # Configura il layout a griglia della pagina principale
        self.rowconfigure(0, weight=0)  # Navbar - fisso
        self.rowconfigure(1, weight=0)  # Separatore
        self.rowconfigure(2, weight=0)  # Sezione nuova partita - ridotto
        self.rowconfigure(3, weight=0)  # Separatore
        self.rowconfigure(4, weight=0)  # Pulsanti di navigazione
        self.rowconfigure(5, weight=1)  # Sezione contenuto dinamico - tutto lo spazio rimanente
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
        separator.grid(row=1, column=0, sticky="ew", padx=(30,0), pady=(0, 10))

        # Sezione nuova partita
        section1 = ttk.Frame(self, style="TFrame")
        section1.grid(row=2, column=0, sticky="ew", padx=20, pady=(0, 5))

        section1.columnconfigure(0, weight=1)

        self.welcome_label = ttk.Label(section1, text="Benvenuto", style="Title.TLabel", justify="center")
        self.welcome_label.pack(pady=(10, 5), padx=(30,0))

        button_new_game = ttk.Button(section1, text="Nuova partita", style=ACCENT_BUTTON_STYLE, command=self.new_game)
        button_new_game.pack(pady=(5,10), padx=(30,0))

        # Separatore
        separator = ttk.Separator(self, orient="horizontal")
        separator.grid(row=3, column=0, sticky="ew", padx=(30,0), pady=(0, 5))

        # Pulsanti di navigazione
        nav_frame = ttk.Frame(self, style="TFrame")
        nav_frame.grid(row=4, column=0, sticky="ew", padx=20, pady=(5, 10))
        nav_frame.columnconfigure(0, weight=1)
        nav_frame.columnconfigure(1, weight=1)
        nav_frame.columnconfigure(2, weight=1)

        self.btn_waiting = ttk.Button(nav_frame, text="Partite in attesa", 
                                     command=lambda: self.change_view("waiting_games"))
        self.btn_waiting.grid(row=0, column=0, padx=5, sticky="ew")

        self.btn_received = ttk.Button(nav_frame, text="Richieste ricevute", 
                                      command=lambda: self.change_view("received_requests"))
        self.btn_received.grid(row=0, column=1, padx=5, sticky="ew")

        self.btn_sent = ttk.Button(nav_frame, text="Richieste effettuate", 
                                  command=lambda: self.change_view("sent_requests"))
        self.btn_sent.grid(row=0, column=2, padx=5, sticky="ew")

        # Sezione contenuto dinamico
        self.content_frame = ttk.Frame(self, style="TFrame")
        self.content_frame.grid(row=5, column=0, sticky="nsew", padx=20, pady=(0, 5))
        
        # Configura il grid del content_frame per espansione
        self.content_frame.rowconfigure(1, weight=1)  # Per il scroll_frame
        self.content_frame.columnconfigure(0, weight=1)
        
        # Label del titolo della sezione
        self.section_title = ttk.Label(self.content_frame, text="Partite in attesa...", style="TLabel")
        self.section_title.grid(row=0, column=0, padx=(10, 0), pady=(0, 5), sticky="w")

        # Frame per scrollbar e canvas con sfondo nero
        self.scroll_frame = ttk.Frame(self.content_frame, style="TFrame")
        self.scroll_frame.grid(row=1, column=0, sticky="nsew", padx=10, pady=5)
        
        # TEMPORANEO: Usa un Frame semplice invece del Canvas per debug
        print("🔧 DEBUG: Creazione layout semplificato senza Canvas...")
        self.content_container = ttk.Frame(self.scroll_frame, style="TFrame")
        self.content_container.pack(fill="both", expand=True)
        
        # Commentiamo il Canvas per ora
        self.canvas = tk.Canvas(self.scroll_frame, highlightthickness=0, bg="black", bd=0, height=250)
        self.scrollbar = ttk.Scrollbar(self.scroll_frame, orient="vertical", command=self.canvas.yview)
        self.content_container = ttk.Frame(self.canvas, style="TFrame")
        
        # # Configura la scrollbar
        self.canvas.configure(yscrollcommand=self.scrollbar.set)
        
        # # Pack scrollbar e canvas - mostra sempre la scrollbar
        self.scrollbar.pack(side="right", fill="y")
        self.canvas.pack(side="left", fill="both", expand=True)
        
        # # Crea la finestra nel canvas
        self.canvas_window = self.canvas.create_window((0, 0), window=self.content_container, anchor="nw")
        
        # # Bind eventi per aggiornare la scrollregion
        self.content_container.bind('<Configure>', self.on_frame_configure)
        self.canvas.bind('<Configure>', self.on_canvas_configure)
        
        # # Abilita lo scroll con la rotella del mouse
        self.canvas.bind("<MouseWheel>", self.on_mousewheel)

        # Aggiorna lo stile del pulsante attivo
        self.update_button_styles()

    def start_server_listener(self):
        """Avvia il monitoraggio del server usando select (non-bloccante)"""
        if self.listener_active:
            return  # Listener già attivo
            
        self.listener_active = True
        self.poll_server_messages()
        print("🔄 Server listener avviato (select-based)")

    def stop_server_listener(self):
        """Ferma il monitoraggio del server"""
        self.listener_active = False
        if self.server_polling_id:
            self.after_cancel(self.server_polling_id)
            self.server_polling_id = None
        print("🛑 Server listener fermato")

    def poll_server_messages(self):
        """Controlla periodicamente se ci sono messaggi dal server usando select"""
        if not self.listener_active:
            return
            
        try:
            # Simile al pattern C: controlla se ci sono dati dal server
            response = receive_from_server()
            if response:
                try:
                    data = json.loads(response)
                    self.handle_server_message(data)
                    print(f"📨 Messaggio server processato: {data.get('path', 'unknown')}")
                except json.JSONDecodeError:
                    print(f"❌ Errore parsing JSON dal server: {response}")
        except Exception as e:
            print(f"❌ Errore polling server: {type(e).__name__}: {e}")
        
        # Programma il prossimo controllo (ogni 100ms, come il timeout nel codice C)
        if self.listener_active:
            self.server_polling_id = self.after(100, self.poll_server_messages)

    def handle_server_message(self, data):
        """Gestisce i messaggi ricevuti dal server"""
        message_type = data.get("path") or data.get("type")
        
        if message_type == "/new_request":
            # Nuova richiesta ricevuta dal server (PUSH)
            new_request = {
                'game_id': data.get('game_id'),
                'player_id': data.get('player_id'),
                'mittente': data.get('player_name', 'Sconosciuto'),
            }
            self.add_received_request(new_request)
            print(f"🔔 Nuova richiesta ricevuta da {data.get('player_name')} per partita {data.get('player_id')}")
        elif message_type == "/remove_request":
            # Richiesta annullata (PUSH)
            player_id = data.get('player_id')
            game_id = data.get('game_id')
            # Rimuovi la richiesta dalla lista locale usando player_id e game_id
            self.cancel_request(player_id, game_id)
            print(f"🗑️ Richiesta annullata da {data.get('player_name')} per partita {game_id}")
            
        elif message_type == "request_accepted":
            # Partita iniziata
            print("🎮 Partita iniziata!")
            self.controller.shared_data["simbolo_assegnato"] = data.get("simbolo", "")
            self.controller.shared_data["nomePartecipante"] = data.get("nickname_partecipante", "")
            self.controller.shared_data["game_id"] = data.get("game_id", "")
            self.controller.shared_data["game_data"] = data.get("game_data", {})
            
            # Passa a GamePage
            self.stop_periodic_update_content()
            self.stop_server_listener()
            self.controller.show_frame("GamePage")
        elif message_type == "request_declined":
            # Una delle tue richieste è stata rifiutata
            player_id = data.get('player_id')
            game_id = data.get('game_id')
            self.update_sent_request_status(player_id, game_id, 'declined')
            print(f"❌ Richiesta player:{player_id} game:{game_id} rifiutata")
            
        else:
            print(f"📨 Messaggio server non gestito: {data}")

    def add_received_request(self, request):
        """Aggiunge una nuova richiesta ricevuta alla lista locale (SOLO da server push)"""
        # Evita duplicati basati su id_richiesta
        self.received_requests.append(request)
        print(f"📥 Richiesta ricevuta aggiunta: {request}")
        
        # Se siamo nella vista richieste ricevute, aggiorna immediatamente la UI
        if self.current_view == "received_requests":
            self.update_received_requests()
        
        # Mostra notifica anche se non siamo nella vista richieste
        if self.current_view != "received_requests":
            self.welcome_label.config(text=f"📩 Nuova richiesta da {request['mittente']}", foreground="blue")

    def add_sent_request(self, request):
        """Aggiunge una nuova richiesta inviata alla lista locale (SOLO quando success)"""
        self.sent_requests.append(request)
        print(f"📤 Richiesta inviata aggiunta: {request}")
        
        # Se siamo nella vista richieste inviate, aggiorna immediatamente la UI
        if self.current_view == "sent_requests":
            self.update_sent_requests()

    def update_sent_request_status(self, player_id, game_id, new_status):
        """Aggiorna lo stato di una richiesta inviata usando player_id e game_id"""
        for request in self.sent_requests:
            if request.get('player_id') == player_id and request.get('game_id') == game_id:
                request['stato'] = new_status
                print(f"🔄 Aggiornato stato richiesta player:{player_id} game:{game_id} -> {new_status}")
                break
        
        # Aggiorna la vista se siamo nelle richieste inviate
        if self.current_view == "sent_requests":
            self.update_sent_requests()

    def update_button_styles(self):
        """Aggiorna lo stile dei pulsanti per evidenziare quello attivo"""
        # Reset tutti i pulsanti allo stile normale
        self.btn_waiting.configure(style="TButton")
        self.btn_received.configure(style="TButton")
        self.btn_sent.configure(style="TButton")
        
        # Evidenzia il pulsante attivo
        if self.current_view == "waiting_games":
            self.btn_waiting.configure(style=ACCENT_BUTTON_STYLE)
        elif self.current_view == "received_requests":
            self.btn_received.configure(style=ACCENT_BUTTON_STYLE)
        elif self.current_view == "sent_requests":
            self.btn_sent.configure(style=ACCENT_BUTTON_STYLE)

    def change_view(self, view_type):
        """Cambia la vista corrente"""
        self.current_view = view_type
        self.update_button_styles()
        
        # Aggiorna il titolo e il contenuto
        if view_type == "waiting_games":
            self.section_title.config(text="Partite in attesa...")
            self.update_waiting_games()
        elif view_type == "received_requests":
            self.section_title.config(text="Richieste ricevute...")
            self.update_received_requests()
        elif view_type == "sent_requests":
            self.section_title.config(text="Richieste effettuate...")
            self.update_sent_requests()

    def periodic_update_content(self):
        """DISABILITATO COMPLETAMENTE per debug del segfault"""
        print("🔄 Polling automatico completamente disabilitato per debug")
        # Commentiamo tutto per testare
        self._content_after_id = self.after(3000, self.periodic_update_content)
        if self.current_view == "waiting_games":
             self.update_waiting_games()
             print("🔄 Aggiornamento partite in attesa...")

    def stop_periodic_update_content(self):
        """Ferma l'aggiornamento periodico"""
        if hasattr(self, '_content_after_id'):
            self.after_cancel(self._content_after_id)
            del self._content_after_id

    def update_data(self):
        nickname = self.controller.shared_data.get('nickname', "Utente")
        self.welcome_label.config(text=f"Benvenuto, {nickname}!")
        self.change_view(self.current_view)
        self.periodic_update_content()
        # Avvia il polling del server
        self.start_server_listener()

    def __del__(self):
        """Cleanup quando l'oggetto viene distrutto"""
        self.stop_server_listener()
        self.stop_periodic_update_content()

    def clear_content(self):
        """Pulisce il contenuto del container con protezione"""
        try:
            if hasattr(self, 'content_container') and self.content_container.winfo_exists():
                for widget in self.content_container.winfo_children():
                    if widget.winfo_exists():
                        widget.destroy()
        except tk.TclError as e:
            print(f"⚠️ Errore durante clear_content: {e}")
        except Exception as e:
            print(f"❌ Errore generico in clear_content: {e}")

    def update_waiting_games(self):
        """Aggiorna la lista delle partite in attesa"""
        try:
            print("🔄 Richiesta partite in attesa...")
            games = send_to_server("/waiting_games", {})
            print(f"🔄 Ricevuta risposta: {games}")
            
            try:
                games = json.loads(games)
                games = games.get("partite", [])
                print(f"🔄 Partite parsate: {games}")
                
                # Limita il numero di partite per evitare overflow
                if len(games) > 20:
                    games = games[:20]
                    print("⚠️ Limitato a 20 partite per sicurezza")
                    
            except json.JSONDecodeError as e:
                print(f"❌ Errore parsing JSON: {e}")
                games = []
                
        except ConnectionError as e:
            print(f"❌ Errore di connessione: {e}")
            games = []
            if hasattr(self, 'welcome_label'):
                self.welcome_label.config(text="Errore di connessione al server", foreground="red")
        except Exception as e:
            print(f"❌ Errore generico: {type(e).__name__}: {e}")
            games = []
            if hasattr(self, 'welcome_label'):
                self.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")

        # Aggiorna UI in modo sicuro
        try:
            self.clear_content()
            
            if not games:
                if hasattr(self, 'content_container') and self.content_container.winfo_exists():
                    ttk.Label(self.content_container, text="Nessuna partita in attesa", style="TLabel").pack(pady=(10, 0))
                return 
            
            print(f"🔄 Creazione widget per {len(games)} partite...")
            for i, game in enumerate(games):
                print(f"🔄 Creazione widget {i+1}/{len(games)}: {game}")
                self.add_waiting_game_widget(game)
                
            print("✅ Tutti i widget creati con successo")
            
        except Exception as e:
            print(f"❌ Errore durante aggiornamento UI: {type(e).__name__}: {e}")
            import traceback
            traceback.print_exc()

    def update_received_requests(self):
        """Mostra la lista delle richieste ricevute (SOLO lista locale - NO polling)"""
        self.clear_content()
        if not self.received_requests:
            ttk.Label(self.content_container, text="Nessuna richiesta ricevuta", style="TLabel").pack(pady=(10, 0))
            return
        
        print(f"🔄 Aggiornamento UI richieste ricevute: {len(self.received_requests)} richieste")
        for request in self.received_requests:
            self.add_received_request_widget(request)

    def update_sent_requests(self):
        """Mostra la lista delle richieste effettuate (SOLO lista locale - NO polling)"""
        self.clear_content()
        if not self.sent_requests:
            ttk.Label(self.content_container, text="Nessuna richiesta effettuata", style="TLabel").pack(pady=(10, 0))
            return
        
        print(f"🔄 Aggiornamento UI richieste inviate: {len(self.sent_requests)} richieste")
        for request in self.sent_requests:
            self.add_sent_request_widget(request)

    def add_waiting_game_widget(self, game):
        """Aggiunge un widget per una partita in attesa SENZA BOTTONI (solo label cliccabili)"""
        try:
            print(f"🔧 DEBUG: Inizio creazione widget SEMPLIFICATO per {game}")
            
            if not hasattr(self, 'content_container') or not self.content_container:
                print("❌ content_container non disponibile")
                return
            
            # Crea il frame principale
            frame = ttk.Frame(self.content_container, relief="raised", borderwidth=2)
            frame.pack(fill="x", padx=4, pady=2)
            
            # Crea la label con le informazioni
            id_partita = game.get('id_partita', 'N/A')
            proprietario = game.get('proprietario', 'Sconosciuto')[:20]
            
            info = f"ID: {id_partita} | Giocatore: {proprietario} | ⏳ CLICCA QUI PER UNIRTI"
            
            # Una sola label cliccabile invece di label + bottone
            label = ttk.Label(frame, text=info, cursor="hand2", 
                            background="black", relief="raised", 
                            padding=(10, 5))
            label.pack(fill="x", padx=5, pady=5)
            
            # Bind del click direttamente sulla label
            game_id = game['id_partita']
            label.bind("<Button-1>", lambda e, gid=game_id: self.join_game(gid))
            
            print(f"🔧 DEBUG: Widget semplificato creato con successo per {game}")
            
            # Forza aggiornamento
            frame.update_idletasks()
            
        except Exception as e:
            print(f"❌ Errore creazione widget semplificato: {type(e).__name__}: {e}")
            import traceback
            traceback.print_exc()

    def add_received_request_widget(self, request):
        """Aggiunge un widget per una richiesta ricevuta"""
        frame = ttk.Frame(self.content_container, style=CARD_FRAME_STYLE, relief="raised", borderwidth=1)
        frame.pack(fill="x", padx=4, pady=2)
        
        info = f"Da: {request['mittente']} Partita: {request['game_id']}| 📩 Richiesta di sfida"
        label = ttk.Label(frame, text=info, style="TLabel")
        label.pack(side="left", padx=10, pady=5)
        
        btn_frame = ttk.Frame(frame)
        btn_frame.pack(side="right", padx=10, pady=5)
        
        accept_btn = ttk.Button(btn_frame, text="Accetta", style=ACCENT_BUTTON_STYLE,
                               command=lambda pid=request.get('player_id'), gid=request.get('game_id'): self.accept_request(pid, gid))
        accept_btn.pack(side="right", padx=2)
        
        decline_btn = ttk.Button(btn_frame, text="Rifiuta", 
                                command=lambda pid=request.get('player_id'), gid=request.get('game_id'): self.decline_request(pid, gid))
        decline_btn.pack(side="right", padx=2)

    def add_sent_request_widget(self, request):
        """Aggiunge un widget per una richiesta effettuata"""
        frame = ttk.Frame(self.content_container, style=CARD_FRAME_STYLE, relief="raised", borderwidth=1)
        frame.pack(fill="x", padx=4, pady=2)
        
        request_stato = request.get('stato', STATO_IN_ATTESA)
        if request_stato == STATO_IN_ATTESA:
            status_icon = "⏳"
        elif request_stato == 'accepted':
            status_icon = "✅"
        else:
            status_icon = "❌"
        info = f"Partita: {request['game_id']} | {status_icon} {request_stato.title()}"
        label = ttk.Label(frame, text=info, style="TLabel")
        label.pack(side="left", padx=10, pady=5)
        
        if request_stato in [STATO_IN_ATTESA, None]:
            cancel_btn = ttk.Button(frame, text="Annulla", 
                                   command=lambda pid=request.get('player_id'), gid=request.get('game_id'): self.cancel_request(pid, gid))
            cancel_btn.pack(side="right", padx=10, pady=5)

    def new_game(self):
        """Crea una nuova partita"""
        response = send_to_server("/new_game", {"nickname": self.controller.shared_data['nickname']})
        try:
            data = json.loads(response)
            if data.get("success") == 1:
                self.welcome_label.config(text="Partita creata con successo!", foreground="green")
                # Aggiorna la vista se siamo nelle partite in attesa
                if self.current_view == "waiting_games":
                    self.update_waiting_games()
            else:
                self.welcome_label.config(text="Errore nella creazione della partita", foreground="red")
        except json.JSONDecodeError:
            self.welcome_label.config(text=ERROR_SERVER_RESPONSE, foreground="red")
        except Exception as e:
            self.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")

    def join_game(self, game_id):
        """Invia una richiesta per unirsi a una partita specifica"""
        response = send_to_server("/add_request", {
            "id": game_id
        })
        try:
            data = json.loads(response)
            if data.get("success") == 1:
                # Aggiungi la richiesta alla lista locale SOLO se il server ha confermato successo
                new_request = {
                    'game_id': game_id,
                    'player_id': self.controller.shared_data.get('player_id', 'unknown'),
                    'stato': STATO_IN_ATTESA
                }
                self.welcome_label.config(text=data.get("message"), foreground="green")
                self.add_sent_request(new_request)
            else:
                self.welcome_label.config(text=data.get("message"), foreground="red")
        except json.JSONDecodeError:
            self.welcome_label.config(text=ERROR_SERVER_RESPONSE, foreground="red")
        except Exception as e:
            self.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")

    def accept_request(self, player_id, game_id):
        """Accetta una richiesta ricevuta"""
        response = send_to_server("/accept_request", {
            "player_id": player_id,
            "game_id": game_id,
            "nickname": self.controller.shared_data['nickname']
        })
        try:
            data = json.loads(response)
            if data.get("success") == 1:
                # Rimuovi la richiesta dalla lista locale usando player_id e game_id
                self.received_requests = [r for r in self.received_requests 
                                        if not (r.get('player_id') == player_id and r.get('game_id') == game_id)]
                
                self.welcome_label.config(text="Richiesta accettata!", foreground="green")
                print(f"✅ Richiesta player:{player_id} game:{game_id} accettata e rimossa dalla lista")
                
                # Vai alla GamePage se fornito game_id
                if data.get('game_id'):
                    self.controller.shared_data['game_id'] = data.get('game_id')
                    self.stop_periodic_update_content()
                    self.stop_server_listener()
                    self.controller.show_frame("GamePage")
                else:
                    self.update_received_requests()
            else:
                self.welcome_label.config(text="Errore nell'accettare la richiesta", foreground="red")
        except Exception as e:
            self.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")

    def decline_request(self, player_id, game_id):
        """Rifiuta una richiesta ricevuta"""
        response = send_to_server("/decline_request", {
            "player_id": player_id,
            "game_id": game_id,
            "nickname": self.controller.shared_data['nickname']
        })
        try:
            data = json.loads(response)
            if data.get("success") == 1:
                # Rimuovi la richiesta dalla lista locale usando player_id e game_id
                self.received_requests = [r for r in self.received_requests 
                                        if not (r.get('player_id') == player_id and r.get('game_id') == game_id)]
                
                self.welcome_label.config(text="Richiesta rifiutata", foreground="orange")
                print(f"❌ Richiesta player:{player_id} game:{game_id} rifiutata e rimossa dalla lista")
                self.update_received_requests()
            else:
                self.welcome_label.config(text="Errore nel rifiutare la richiesta", foreground="red")
        except Exception as e:
            self.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")

    def cancel_request(self, player_id, game_id):
        """Annulla una richiesta effettuata"""
        print(f"🗑️ DEBUG: Annullamento richiesta player_id={player_id}, game_id={game_id}")
        
        try:
            response = send_to_server("/remove_request", {
                "game_id": game_id, 
            })
            print(f"🗑️ DEBUG: Risposta server: {response}")
            
            data = json.loads(response)
            print(f"🗑️ DEBUG: Data parsata: {data}")
            
            if data.get("success") == 1:
                # Rimuovi la richiesta dalla lista locale usando player_id e game_id
                self.sent_requests = [r for r in self.sent_requests 
                                    if not (r.get('player_id') == player_id and r.get('game_id') == game_id)]
                
                message = data.get("message", "Richiesta annullata")
                print(f"🗑️ DEBUG: Messaggio: {message}")
                self.welcome_label.config(text=message, foreground="orange")
                print(f"🗑️ Richiesta player:{player_id} game:{game_id} annullata e rimossa dalla lista")
                self.update_sent_requests()
            else:
                error_message = data.get("message", "Errore nell'annullare la richiesta")
                print(f"🗑️ DEBUG: Errore: {error_message}")
                self.welcome_label.config(text=error_message, foreground="red")
        except json.JSONDecodeError as e:
            print(f"🗑️ DEBUG: Errore JSON: {e}")
            self.welcome_label.config(text=ERROR_SERVER_RESPONSE, foreground="red")
        except Exception as e:
            print(f"🗑️ DEBUG: Errore generico: {type(e).__name__}: {str(e)}")
            self.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")

    def on_frame_configure(self, event):
        """Aggiorna la scrollregion quando il contenuto cambia (PROTETTO)"""
        try:
            bbox = self.canvas.bbox("all")
            if bbox is not None:
                self.canvas.configure(scrollregion=bbox)
            else:
                # Se bbox è None, usa una scrollregion di default
                self.canvas.configure(scrollregion=(0, 0, 0, 0))
        except Exception as e:
            print(f"❌ Errore in on_frame_configure: {e}")

    def on_canvas_configure(self, event):
        """Configura la larghezza del frame interno quando il canvas cambia dimensione (PROTETTO)"""
        try:
            canvas_width = event.width
            if hasattr(self, 'canvas_window') and self.canvas_window:
                self.canvas.itemconfig(self.canvas_window, width=canvas_width)
        except Exception as e:
            print(f"❌ Errore in on_canvas_configure: {e}")

    def on_mousewheel(self, event):
        """Gestisce lo scroll con la rotella del mouse"""
        self.canvas.yview_scroll(int(-1*(event.delta/120)), "units")