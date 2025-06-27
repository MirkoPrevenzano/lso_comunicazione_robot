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
            "/accept_request": self._handle_request_accepted,
            "/decline_request": self._handle_request_declined,
            "/game/start": self._handle_request_accepted,  # Partita iniziata
            "/update_game": self._hand_game_update,  # Aggiornamento partita
            "/game_exit": self._hand_game_end,  # Partita terminata
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
                        # ✅ response è già un dizionario parsato da client_network.py
                        if isinstance(response, dict):
                            data = response  # Usa direttamente il dizionario
                        elif isinstance(response, str):
                            data = json.loads(response)  # Parsa solo se è una stringa
                        else:
                            print(f"❌ Tipo messaggio non supportato: {type(response)}")
                            continue
                            
                        self._handle_server_message(data)
                        print(f"📨 Messaggio server processato: {data.get('path', 'unknown')}")
                    except json.JSONDecodeError as e:
                        print(f"❌ Errore parsing JSON dal server: {response}")
                        print(f"❌ Errore: {e}")
                    except Exception as e:
                        print(f"❌ Errore generico nel processare messaggio: {type(e).__name__}: {e}")
        except Exception as e:
            print(f"❌ Errore polling server: {type(e).__name__}: {e}")
        
        # Programma il prossimo controllo (ogni 100ms, come il timeout nel codice C)
        if self.listener_active:
            self.server_polling_id = self.home_page.after(100, self._poll_server_messages)
    
    def _handle_server_message(self, data):
        """Gestisce i messaggi ricevuti dal server usando il pattern handler."""
        print(f"🔍 DEBUG: Ricevuto messaggio dal server: {data}")
        message_type = data.get("path") or data.get("type")
        
        # ✅ Ignora i messaggi di risposta alle partite (senza path)
        if not message_type and "partite" in data:
            print(f"📨 Messaggio risposta partite ignorato: {data}")
            return
        
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
        
        print(f"🗑️ DEBUG: Handling remove_request - player_id={player_id}, game_id={game_id}")
        print(f"🗑️ DEBUG: Richieste ricevute prima rimozione: {len(self.home_page.request_manager.received_requests)}")
        for i, req in enumerate(self.home_page.request_manager.received_requests):
            print(f"🗑️ DEBUG: Richiesta {i}: {req}")
        
        # Rimuovi la richiesta dalla lista locale usando player_id e game_id
        self.home_page.request_manager.remove_received_request(player_id, game_id)
        
        print(f"🗑️ DEBUG: Richieste ricevute dopo rimozione: {len(self.home_page.request_manager.received_requests)}")
        for i, req in enumerate(self.home_page.request_manager.received_requests):
            print(f"🗑️ DEBUG: Richiesta {i}: {req}")
        
        # ✅ Non aggiornare l'UI qui se già gestito dal request_manager
        # Il request_manager chiama automaticamente _update_ui_if_needed
        
        mittente_name = data.get('player_name', data.get('mittente', f'Player{player_id}'))
        print(f"🗑️ Richiesta annullata da {mittente_name} per partita {game_id}")
    
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
        self.home_page.controller.show_frame("GamePage")
    
    def _handle_request_declined(self, data):
        """Gestisce richiesta rifiutata"""
        print("❌ Richiesta rifiuptata dal server")
        game_id = data.get('game_id')
        self.home_page.request_manager.update_sent_request_status(game_id, 'declined')
        print(f"❌ game:{game_id} rifiutata")
    
    def _hand_game_update(self, data):
        request = {
            "game_id": data.get("game_id"),
            "TRIS": data.get("TRIS", [[0]*3 for _ in range(3)]),
            "esito": data.get("esito"),
            "turno": data.get("turno"),
            "messaggio": data.get("messaggio", ""),
        }
        self.game_page.aggiorna_dati(request)
        print(f"🔄 Aggiornamento partita ricevuto: {request}")
        """Gestisce aggiornamento partita"""
    
    def _hand_game_end(self, data):
        """Gestisce fine partita"""
    
