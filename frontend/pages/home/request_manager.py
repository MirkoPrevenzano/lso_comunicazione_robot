"""
Gestione delle richieste di gioco (inviate e ricevute).
Estrae la logica di gestione delle richieste dalla HomePage.
"""


class RequestManager:
    """Gestisce le richieste di gioco inviate e ricevute."""
    
    def __init__(self, home_page):
        self.home_page = home_page
        self.received_requests = []  # Richieste ricevute
        self.sent_requests = []      # Richieste inviate
    
    def add_received_request(self, request):
        """Aggiunge una nuova richiesta ricevuta."""
        # Evita duplicati
        if not any(r['player_id'] == request['player_id'] and 
                  r['game_id'] == request['game_id'] 
                  for r in self.received_requests):
            self.received_requests.append(request)
            self._update_ui_if_needed("received_requests")
            print(f"✅ Aggiunta richiesta ricevuta: {request}")
    
    def remove_received_request(self, player_id, game_id):
        """Rimuove una richiesta ricevuta."""
        print(f"🔍 DEBUG: Tentativo rimozione player_id={player_id}, game_id={game_id}")
        print(f"🔍 DEBUG: Lista attuale ({len(self.received_requests)} richieste):")
        for i, req in enumerate(self.received_requests):
            print(f"🔍 DEBUG:   {i}: {req}")
        
        initial_count = len(self.received_requests)
        
        self.received_requests = [
            r for r in self.received_requests 
            if not (r['player_id'] == player_id and r['game_id'] == game_id)
        ]
        
        final_count = len(self.received_requests)
        removed_count = initial_count - final_count
        
        print(f"🔍 DEBUG: Rimosse {removed_count} richieste (da {initial_count} a {final_count})")
        print(f"🔍 DEBUG: Lista finale:")
        for i, req in enumerate(self.received_requests):
            print(f"🔍 DEBUG:   {i}: {req}")
        
        self._update_ui_if_needed("received_requests")
        print(f"🗑️ Rimossa richiesta ricevuta: player={player_id}, game={game_id}")
    
    def add_sent_request(self, request):
        """Aggiunge una nuova richiesta inviata."""
        # Evita duplicati
        if not any(r['game_id'] == request['game_id'] for r in self.sent_requests):
            self.sent_requests.append(request)
            self._update_ui_if_needed("sent_requests")
            print(f"✅ Aggiunta richiesta inviata: {request}")
    
    def remove_sent_request(self, game_id):
        """Rimuove una richiesta inviata."""
        self.sent_requests = [
            r for r in self.sent_requests 
            if r['game_id'] != game_id
        ]
        self._update_ui_if_needed("sent_requests")
        print(f"🗑️ Rimossa richiesta inviata: game={game_id}")
    
    def update_sent_request_status(self, game_id, status):
        """Aggiorna lo stato di una richiesta inviata."""
        for request in self.sent_requests:
            if request['game_id'] == game_id:
                request['stato'] = status
                self._update_ui_if_needed("sent_requests")
                print(f"📝 Aggiornato stato richiesta {game_id}: {status}")
                break
    
    def clear_all_requests(self):
        """Pulisce tutte le richieste."""
        self.received_requests.clear()
        self.sent_requests.clear()
        self._update_ui_if_needed()
        print("🧹 Tutte le richieste sono state pulite")
    
    def get_received_requests(self):
        """Restituisce la lista delle richieste ricevute."""
        return self.received_requests
    
    def get_sent_requests(self):
        """Restituisce la lista delle richieste inviate."""
        return self.sent_requests
    
    def has_pending_requests(self):
        """Verifica se ci sono richieste in sospeso."""
        return len(self.received_requests) > 0 or len(self.sent_requests) > 0
    
    def _update_ui_if_needed(self, view_type=None):
        """Aggiorna l'UI solo se siamo nella vista corretta."""
        if view_type is None:
            # Aggiorna sempre se non specificato
            if self.home_page.current_view == "received_requests":
                self.home_page.update_received_requests()
            elif self.home_page.current_view == "sent_requests":
                self.home_page.update_sent_requests()
        elif self.home_page.current_view == view_type:
            if view_type == "received_requests":
                self.home_page.update_received_requests()
            elif view_type == "sent_requests":
                self.home_page.update_sent_requests()
