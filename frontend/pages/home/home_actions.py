"""Gestione delle azioni per richieste e partite nella home page"""

import json
from tkinter import messagebox
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
                messagebox.showinfo("Partita Creata", data.get("message", "Partita creata con successo!"))               
            else:
                messagebox.showerror("Errore", data.get("message", "Errore nella creazione della partita"))
                
        except json.JSONDecodeError:
            messagebox.showerror("Errore", ERROR_SERVER_RESPONSE)
        except Exception as e:
            messagebox.showerror("Errore", f"Si è verificato un errore: {str(e)}")
    
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
                messagebox.showinfo("Richiesta Inviata", data.get("message", "Richiesta inviata con successo!"))
                self.home_page.request_manager.add_sent_request(new_request)
            else:
                messagebox.showerror("Errore", data.get("message", "Errore nell'invio della richiesta"))
                
        except json.JSONDecodeError:
            messagebox.showerror("Errore", ERROR_SERVER_RESPONSE)
        except Exception as e:
            messagebox.showerror("Errore", f"Si è verificato un errore: {str(e)}")

    
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
                messagebox.showinfo("Richiesta Accettata", data.get("message", "Richiesta accettata con successo!"))
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
                messagebox.showerror("Errore", data.get("message", "Errore nell'accettare la richiesta"))
                
        except json.JSONDecodeError:
            messagebox.showerror("Errore", ERROR_SERVER_RESPONSE)
        except Exception as e:
            messagebox.showerror("Errore", f"Si è verificato un errore: {str(e)}")
    
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
                messagebox.showinfo("Richiesta Rifiutata", data.get("message", "Richiesta rifiutata con successo!"))
                print(f"❌ Richiesta player:{player_id} game:{game_id} rifiutata e rimossa dalla lista")
                self.home_page.update_received_requests()
            else:
                messagebox.showerror("Errore", data.get("message", "Errore nel rifiutare la richiesta"))
                
        except json.JSONDecodeError:
            messagebox.showerror("Errore", ERROR_SERVER_RESPONSE)
        except Exception as e:
            messagebox.showerror("Errore", f"Si è verificato un errore: {str(e)}")
    
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
                messagebox.showinfo("Richiesta Annullata", message)
                print(f"🗑️ Richiesta player:{player_id} game:{game_id} annullata e rimossa dalla lista")
                self.home_page.update_sent_requests()
            else:
                error_message = data.get("message", "Errore nell'annullare la richiesta")
                print(f"🗑️ DEBUG: Errore: {error_message}")
                messagebox.showerror("Errore", error_message)
                
        except json.JSONDecodeError as e:
            messagebox.showerror("Errore", ERROR_SERVER_RESPONSE)
        except Exception as e:
            print(f"🗑️ DEBUG: Errore generico: {type(e).__name__}: {str(e)}")
            messagebox.showerror("Errore", f"Si è verificato un errore: {str(e)}")

    def upload_send_requests(self):
        """Carica le richieste inviate"""
        try:
            response = send_to_server("/get_sent_requests", {})
            data = json.loads(response)
            if data.get("path") == "/send_requests":
                self.home_page.request_manager.set_sent_requests(data.get("requests", []))
                print("✅ Richieste inviate caricate con successo")
            else:
                messagebox.showerror("Errore", data.get("message", "Errore nel caricamento delle richieste inviate"))
                
        except json.JSONDecodeError:
            messagebox.showerror("Errore", ERROR_SERVER_RESPONSE)
        except Exception as e:
            messagebox.showerror("Errore", f"Si è verificato un errore: {str(e)}")
    
    def upload_received_requests(self):
        """Carica le richieste ricevute"""
        try:
            response = send_to_server("/get_received_requests", {})
            data = json.loads(response)
            if data.get("path") == "/received_requests":
                self.home_page.request_manager.set_received_requests(data.get("requests", []))
                print("✅ Richieste ricevute caricate con successo")
            else:
                messagebox.showerror("Errore", data.get("message", "Errore nel caricamento delle richieste ricevute"))
                
        except json.JSONDecodeError:
            messagebox.showerror("Errore", ERROR_SERVER_RESPONSE)
        except Exception as e:
            messagebox.showerror("Errore", f"Si è verificato un errore: {str(e)}")
