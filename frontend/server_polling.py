"""Gestione del polling del server centralizzato per tutta l'applicazione"""

import json
from client_network import receive_from_server


class ServerPollingManager:
    """Gestisce il polling del server e i messaggi ricevuti per tutta l'applicazione"""
    
    def __init__(self, controller):
        self.controller = controller
        self.listener_active = False
        self.server_polling_id = None
        
        # Handler per i diversi tipi di messaggio
        self.message_handlers = {
            "/new_request": self._handle_new_request,
            "/remove_request": self._handle_remove_request,
            "/accept_request": self._handle_request_accepted,
            "/decline_request": self._handle_request_declined,
            "/game_start": self._handle_request_accepted,  # Partita iniziata
            "/game_response": self._handle_game_update,  # Aggiornamento partita
            "/game_exit": self._handle_game_end,  # Partita terminata
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
            # Usa il controller invece di home_page per annullare il timer
            self.controller.after_cancel(self.server_polling_id)
            self.server_polling_id = None
        print("🛑 Server listener fermato")
    
    def _get_home_page(self):
        """Ottieni il riferimento alla HomePage"""
        return self.controller.frames.get("HomePage")
    
    def _get_game_page(self):
        """Ottieni il riferimento alla GamePage"""
        return self.controller.frames.get("GamePage")
    
    def _get_current_frame_name(self):
        """Ottieni il nome del frame corrente"""
        return getattr(self.controller, 'current_frame', None)
    
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
            self.server_polling_id = self.controller.after(100, self._poll_server_messages)
    
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
        home_page = self._get_home_page()
        if not home_page:
            print("⚠️ HomePage non trovata per gestire nuova richiesta")
            return
            
        new_request = {
            'game_id': data.get('game_id'),
            'player_id': data.get('player_id'),
            'mittente': data.get('player_name', 'Sconosciuto'),
        }
        home_page.request_manager.add_received_request(new_request)
        print(f"🔔 Nuova richiesta ricevuta da {data.get('player_name')} per partita {data.get('game_id')}")
    
    def _handle_remove_request(self, data):
        """Gestisce richiesta annullata"""
        home_page = self._get_home_page()
        if not home_page:
            print("⚠️ HomePage non trovata per gestire rimozione richiesta")
            return
            
        player_id = data.get('player_id')
        game_id = data.get('game_id')
        
        print(f"🗑️ DEBUG: Handling remove_request - player_id={player_id}, game_id={game_id}")
        print(f"🗑️ DEBUG: Richieste ricevute prima rimozione: {len(home_page.request_manager.received_requests)}")
        for i, req in enumerate(home_page.request_manager.received_requests):
            print(f"🗑️ DEBUG: Richiesta {i}: {req}")
        
        # Rimuovi la richiesta dalla lista locale usando player_id e game_id
        home_page.request_manager.remove_received_request(player_id, game_id)
        
        print(f"🗑️ DEBUG: Richieste ricevute dopo rimozione: {len(home_page.request_manager.received_requests)}")
        for i, req in enumerate(home_page.request_manager.received_requests):
            print(f"🗑️ DEBUG: Richiesta {i}: {req}")
        
        # ✅ Non aggiornare l'UI qui se già gestito dal request_manager
        # Il request_manager chiama automaticamente _update_ui_if_needed
        
        mittente_name = data.get('player_name', data.get('mittente', f'Player{player_id}'))
        print(f"🗑️ Richiesta annullata da {mittente_name} per partita {game_id}")
    
    def _handle_request_accepted(self, data):
        """Gestisce partita iniziata"""
        print("🎮 Partita iniziata!")
        home_page = self._get_home_page()
        
        self.controller.shared_data['game_id'] = data.get('game_id')
        self.controller.shared_data['player_id_partecipante'] = data.get('player_id')
        self.controller.shared_data['simbolo'] = data.get('simbolo', 'O')
        self.controller.shared_data['nickname_partecipante'] = data.get('nickname_partecipante', 'Sconosciuto')
        self.controller.shared_data['game_data'] = data.get('game_data', {})
        
        # Ferma l'aggiornamento periodico della HomePage se disponibile
        if home_page and hasattr(home_page, 'stop_periodic_update_content'):
            home_page.stop_periodic_update_content()
        
        # Passa a GamePage
        self.controller.show_frame("GamePage")
    
    def _handle_request_declined(self, data):
        """Gestisce richiesta rifiutata"""
        print("❌ Richiesta rifiutata dal server")
        home_page = self._get_home_page()
        if not home_page:
            print("⚠️ HomePage non trovata per gestire richiesta rifiutata")
            return
            
        game_id = data.get('game_id')
        home_page.request_manager.update_sent_request_status(game_id, 'declined')
        print(f"❌ game:{game_id} rifiutata")
    
    def _handle_game_update(self, data):
        """Gestisce aggiornamento partita - funziona indipendentemente dalla pagina corrente"""
        print(f"🔄 Aggiornamento partita ricevuto: {data}")
        
        # Aggiorna i dati condivisi sempre
        self.controller.shared_data['game_id'] = data.get('game_id')
        self.controller.shared_data['player_id_partecipante'] = data.get('player_id')
        self.controller.shared_data['game_data'] = data.get('game_data', {})
        
        # Verifica quale pagina è correntemente attiva
        current_frame = self._get_current_frame_name()
        
        # Aggiorna la GamePage se disponibile
        game_page = self._get_game_page()
        if game_page and hasattr(game_page, 'aggiorna_dati'):
            try:
                game_data = data.get('game_data', {})
                print(f"🔧 DEBUG: Aggiornando GamePage con dati: {game_data}")
                game_page.aggiorna_dati(game_data)
                print(f"✅ GamePage aggiornata (frame corrente: {current_frame})")
            except Exception as e:
                print(f"❌ Errore aggiornamento GamePage: {type(e).__name__}: {e}")
                import traceback
                traceback.print_exc()
        else:
            print("⚠️ GamePage non trovata o non ha metodo aggiorna_dati")
                
    def _handle_game_end(self, data):
        """Gestisce fine partita"""
        print(f"🏁 Fine partita ricevuta: {data}")
        # TODO: Implementare gestione fine partita
        # Potrebbe tornare alla HomePage, mostrare risultati, etc.
    
