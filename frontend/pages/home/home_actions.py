"""Gestione delle azioni per richieste e partite nella home page"""

import json
from client_network import send_to_server
from constants import STATO_IN_ATTESA, ERROR_SERVER_RESPONSE


class HomeActions:
    """Gestisce tutte le azioni relative alle richieste e partite"""
    
    def __init__(self, home_page):
        self.home_page = home_page
    
    def new_game(self):
        """Crea una nuova partita"""
        try:
            response = send_to_server("/new_game", {
                "nickname": self.home_page.controller.shared_data['nickname']
            })
            
            data = json.loads(response)
            if data.get("success") == 1:
                self.home_page.welcome_label.config(text=data.get("message"), foreground="green")
                # Aggiorna la vista se siamo nelle partite in attesa
                if self.home_page.current_view == "waiting_games":
                    self.home_page.update_waiting_games()
            else:
                self.home_page.welcome_label.config(text=data.get("message"), foreground="red")
                
        except json.JSONDecodeError:
            self.home_page.welcome_label.config(text=ERROR_SERVER_RESPONSE, foreground="red")
        except Exception as e:
            self.home_page.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")
    
    def add_request(self, game_id):
        """Invia una richiesta per unirsi a una partita specifica (ex join_game)"""
        try:
            response = send_to_server("/add_request", {
                "id": game_id
            })
            
            data = json.loads(response)
            if data.get("success") == 1:
                # Aggiungi la richiesta alla lista locale SOLO se il server ha confermato successo
                new_request = {
                    'game_id': game_id,
                    'player_id': self.home_page.controller.shared_data.get('player_id', 'unknown'),
                    'stato': STATO_IN_ATTESA
                }
                self.home_page.welcome_label.config(text=data.get("message"), foreground="green")
                self.home_page.request_manager.add_sent_request(new_request)
            else:
                self.home_page.welcome_label.config(text=data.get("message"), foreground="red")
                
        except json.JSONDecodeError:
            self.home_page.welcome_label.config(text=ERROR_SERVER_RESPONSE, foreground="red")
        except Exception as e:
            self.home_page.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")
    
    def accept_request(self, player_id, game_id):
        """Accetta una richiesta ricevuta"""
        try:
            response = send_to_server("/accept_request", {
                "player_id": player_id,
                "game_id": game_id,
            })
            
            data = json.loads(response)
            if data.get("success") == 1:
                # Rimuovi la richiesta dalla lista locale usando request_manager
                self.home_page.request_manager.remove_received_request(player_id, game_id)
                
                self.home_page.welcome_label.config(text="Richiesta accettata!", foreground="green")
                print(f"✅ Richiesta player:{player_id} game:{game_id} accettata e rimossa dalla lista")
                
                # Vai alla GamePage se fornito game_id
                self.home_page.controller.shared_data['game_id'] = data.get('game_id')
                self.home_page.controller.shared_data['player_id_partecipante'] = data.get('player_id')
                self.home_page.controller.shared_data['simbolo'] = data.get('simbolo', 'X')
                self.home_page.controller.shared_data['nickname_partecipante'] = data.get('nickname_partecipante', 'Sconosciuto')
                self.home_page.controller.shared_data['game_data'] = data.get('game_data', {})

                self.home_page.stop_periodic_update_content()
                self.home_page.controller.show_frame("GamePage")
            else:
                self.home_page.welcome_label.config(text="Errore nell'accettare la richiesta", foreground="red")
                
        except json.JSONDecodeError:
            self.home_page.welcome_label.config(text=ERROR_SERVER_RESPONSE, foreground="red")
        except Exception as e:
            self.home_page.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")
    
    def decline_request(self, player_id, game_id):
        """Rifiuta una richiesta ricevuta"""
        try:
            response = send_to_server("/decline_request", {
                "player_id": player_id,
                "game_id": game_id,
                "nickname": self.home_page.controller.shared_data['nickname']
            })
            
            data = json.loads(response)
            if data.get("success") == 1:
                # Rimuovi la richiesta dalla lista locale usando request_manager
                self.home_page.request_manager.remove_received_request(player_id, game_id)
                
                self.home_page.welcome_label.config(text="Richiesta rifiutata", foreground="orange")
                print(f"❌ Richiesta player:{player_id} game:{game_id} rifiutata e rimossa dalla lista")
                self.home_page.update_received_requests()
            else:
                self.home_page.welcome_label.config(text="Errore nel rifiutare la richiesta", foreground="red")
                
        except json.JSONDecodeError:
            self.home_page.welcome_label.config(text=ERROR_SERVER_RESPONSE, foreground="red")
        except Exception as e:
            self.home_page.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")
    
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
                # Rimuovi la richiesta dalla lista locale usando request_manager
                self.home_page.request_manager.remove_sent_request(game_id)
                
                message = data.get("message", "Richiesta annullata")
                print(f"🗑️ DEBUG: Messaggio: {message}")
                self.home_page.welcome_label.config(text=message, foreground="orange")
                print(f"🗑️ Richiesta player:{player_id} game:{game_id} annullata e rimossa dalla lista")
                self.home_page.update_sent_requests()
            else:
                error_message = data.get("message", "Errore nell'annullare la richiesta")
                print(f"🗑️ DEBUG: Errore: {error_message}")
                self.home_page.welcome_label.config(text=error_message, foreground="red")
                
        except json.JSONDecodeError as e:
            print(f"🗑️ DEBUG: Errore JSON: {e}")
            self.home_page.welcome_label.config(text=ERROR_SERVER_RESPONSE, foreground="red")
        except Exception as e:
            print(f"🗑️ DEBUG: Errore generico: {type(e).__name__}: {str(e)}")
            self.home_page.welcome_label.config(text=f"Errore: {str(e)}", foreground="red")
