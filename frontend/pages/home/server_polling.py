"""Gestione del polling del server per la home page"""

import json
from client_network import receive_from_server


class ServerPollingManager:
    """Gestisce il polling del server e i messaggi ricevuti"""
    
    def __init__(self, home_page):
        self.home_page = home_page
        self.listener_active = False
        self.server_polling_id = None
        
        # Handler per i diversi tipi di messaggio
        self.message_handlers = {
            "/new_request": self._handle_new_request,
            "/remove_request": self._handle_remove_request,
            "request_accepted": self._handle_request_accepted,
            "/decline_request": self._handle_request_declined
        }
    
    def start_listener(self):
        """Avvia il polling del server."""
        if self.listener_active:
            return  # Listener già attivo    
        self.listener_active = True
        self._poll_server_messages()
        print("🔄 Server listener avviato (select-based)")
    
    def stop_listener(self):
        """Ferma il monitoraggio del server"""
        self.listener_active = False
        if self.server_polling_id:
            self.home_page.after_cancel(self.server_polling_id)
            self.server_polling_id = None
        print("🛑 Server listener fermato")
    
    def _poll_server_messages(self):
        """Controlla periodicamente se ci sono messaggi dal server usando select."""
        if not self.listener_active:
            return    
        try:
            # Simile al pattern C: controlla se ci sono dati dal server
            messages = receive_from_server()
            if messages:
                print(f"📨 Ricevuti {len(messages)} messaggi dal server")
                for response in messages:
                    try:
                        data = json.loads(response)
                        self._handle_server_message(data)
                        print(f"📨 Messaggio server processato: {data.get('path', 'unknown')}")
                    except json.JSONDecodeError:
                        print(f"❌ Errore parsing JSON dal server: {response}")
        except Exception as e:
            print(f"❌ Errore polling server: {type(e).__name__}: {e}")
        
        # Programma il prossimo controllo (ogni 100ms, come il timeout nel codice C)
        if self.listener_active:
            self.server_polling_id = self.home_page.after(100, self._poll_server_messages)
    
    def _handle_server_message(self, data):
        """Gestisce i messaggi ricevuti dal server usando il pattern handler."""
        message_type = data.get("path") or data.get("type")
        
        print(f"🔍 DEBUG: Gestione messaggio tipo '{message_type}' con dati: {data}")
        
        # Usa il pattern handler per gestire i diversi tipi di messaggio
        handler = self.message_handlers.get(message_type)
        if handler:
            handler(data)
        else:
            print(f"📨 Messaggio server non gestito: {data}")
    
    def _handle_new_request(self, data):
        """Gestisce nuova richiesta ricevuta dal server"""
        new_request = {
            'game_id': data.get('game_id'),
            'player_id': data.get('player_id'),
            'mittente': data.get('player_name', 'Sconosciuto'),
        }
        self.home_page.request_manager.add_received_request(new_request)
        print(f"🔔 Nuova richiesta ricevuta da {data.get('player_name')} per partita {data.get('game_id')}")
    
    def _handle_remove_request(self, data):
        """Gestisce richiesta annullata"""
        player_id = data.get('player_id')
        game_id = data.get('game_id')
        
        # Rimuovi la richiesta dalla lista locale usando player_id e game_id
        self.home_page.request_manager.remove_received_request(player_id, game_id)
        
        # Aggiorna UI se siamo nella vista richieste ricevute
        if self.home_page.current_view == "received_requests":
            self.home_page.update_received_requests()
            
        print(f"🗑️ Richiesta annullata da {data.get('mittente')} per partita {game_id}")
    
    def _handle_request_accepted(self, data):
        """Gestisce partita iniziata"""
        print("🎮 Partita iniziata!")
        self.home_page.controller.shared_data.update({
            "simbolo_assegnato": data.get("simbolo", ""),
            "nomePartecipante": data.get("nickname_partecipante", ""),
            "game_id": data.get("game_id", ""),
            "game_data": data.get("game_data", {})
        })
        
        # Passa a GamePage
        self.home_page.stop_periodic_update_content()
        self.stop_listener()
        self.home_page.controller.show_frame("GamePage")
    
    def _handle_request_declined(self, data):
        """Gestisce richiesta rifiutata"""
        print("❌ Richiesta rifiutata dal server")
        game_id = data.get('game_id')
        self.home_page.request_manager.update_sent_request_status(game_id, 'declined')
        print(f"❌ game:{game_id} rifiutata")
