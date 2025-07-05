"""Gestione delle azioni per richieste e partite nella home page"""

import json
import time
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
        """Carica le richieste inviate con retry in caso di risposta errata"""
        max_retries = 3
        for attempt in range(max_retries):
            try:
                print(f"🔄 DEBUG: Richiesta richieste inviate (tentativo {attempt + 1}/{max_retries})...")
                response = send_to_server("/get_sent_requests", {})
                print(f"🔄 DEBUG: Risposta ricevuta: {response}")
                
                data = json.loads(response)
                print(f"🔄 DEBUG: Data parsata: {data}")
                
                # Validazione: verifica che la risposta sia corretta per questa richiesta
                if data.get("path") == "/send_requests":
                    requests = data.get("requests", [])
                    print(f"🔄 DEBUG: Richieste trovate: {len(requests)}")
                    self.home_page.request_manager.set_sent_requests(requests)
                    print("✅ Richieste inviate caricate con successo")
                    return  # Successo, esci dalla funzione
                elif "partite" in data:
                    # Risposta di /waiting_games ricevuta per errore
                    print(f"⚠️ DEBUG: Ricevuta risposta di /waiting_games invece di /get_sent_requests (tentativo {attempt + 1})")
                    if attempt < max_retries - 1:
                        time.sleep(0.2)  # Pausa più lunga prima del retry
                        continue
                elif data.get("path") == "/received_requests":
                    # Risposta di /get_received_requests ricevuta per errore
                    print(f"⚠️ DEBUG: Ricevuta risposta di /get_received_requests invece di /get_sent_requests (tentativo {attempt + 1})")
                    if attempt < max_retries - 1:
                        time.sleep(0.2)  # Pausa più lunga prima del retry
                        continue
                else:
                    error_msg = data.get("message", "Errore nel caricamento delle richieste inviate")
                    print(f"❌ DEBUG: Errore dal server: {error_msg}")
                    messagebox.showerror("Errore", error_msg)
                    return
                    
            except json.JSONDecodeError as e:
                print(f"❌ DEBUG: Errore JSON decode: {e}")
                print(f"❌ DEBUG: Risposta raw: {response}")
                if attempt < max_retries - 1:
                    time.sleep(0.5)
                    continue
                else:
                    messagebox.showerror("Errore", ERROR_SERVER_RESPONSE)
                    return
            except Exception as e:
                print(f"❌ DEBUG: Errore generico: {type(e).__name__}: {e}")
                if attempt < max_retries - 1:
                    time.sleep(2.0)
                    continue
                else:
                    messagebox.showerror("Errore", f"Si è verificato un errore: {str(e)}")
                    return
        
        # Se arriviamo qui, tutti i tentativi sono falliti
        print("❌ DEBUG: Tutti i tentativi di caricamento richieste inviate falliti")
        messagebox.showerror("Errore", "Impossibile caricare le richieste inviate dopo 3 tentativi")
    
    def upload_received_requests(self):
        """Carica le richieste ricevute con retry in caso di risposta errata"""
        max_retries = 3
        for attempt in range(max_retries):
            try:
                print(f"🔄 DEBUG: Richiesta richieste ricevute (tentativo {attempt + 1}/{max_retries})...")
                response = send_to_server("/get_received_requests", {})
                print(f"🔄 DEBUG: Risposta ricevuta: {response}")
                
                data = json.loads(response)
                print(f"🔄 DEBUG: Data parsata: {data}")
                
                # Validazione: verifica che la risposta sia corretta per questa richiesta
                if data.get("path") == "/received_requests":
                    requests = data.get("requests", [])
                    print(f"🔄 DEBUG: Richieste trovate: {len(requests)}")
                    self.home_page.request_manager.set_received_requests(requests)
                    print("✅ Richieste ricevute caricate con successo")
                    return  # Successo, esci dalla funzione
                elif "partite" in data:
                    # Risposta di /waiting_games ricevuta per errore
                    print(f"⚠️ DEBUG: Ricevuta risposta di /waiting_games invece di /get_received_requests (tentativo {attempt + 1})")
                    if attempt < max_retries - 1:
                        time.sleep(2.0)  # Pausa più lunga prima del retry
                        continue
                elif data.get("path") == "/send_requests":
                    # Risposta di /get_sent_requests ricevuta per errore
                    print(f"⚠️ DEBUG: Ricevuta risposta di /get_sent_requests invece di /get_received_requests (tentativo {attempt + 1})")
                    if attempt < max_retries - 1:
                        time.sleep(2.0)  # Pausa più lunga prima del retry
                        continue
                else:
                    error_msg = data.get("message", "Errore nel caricamento delle richieste ricevute")
                    print(f"❌ DEBUG: Errore dal server: {error_msg}")
                    messagebox.showerror("Errore", error_msg)
                    return
                    
            except json.JSONDecodeError as e:
                print(f"❌ DEBUG: Errore JSON decode: {e}")
                print(f"❌ DEBUG: Risposta raw: {response}")
                if attempt < max_retries - 1:
                    time.sleep(2.0)
                    continue
                else:
                    messagebox.showerror("Errore", ERROR_SERVER_RESPONSE)
                    return
            except Exception as e:
                print(f"❌ DEBUG: Errore generico: {type(e).__name__}: {e}")
                if attempt < max_retries - 1:
                    time.sleep(2.0)
                    continue
                else:
                    messagebox.showerror("Errore", f"Si è verificato un errore: {str(e)}")
                    return
        
        # Se arriviamo qui, tutti i tentativi sono falliti
        print("❌ DEBUG: Tutti i tentativi di caricamento richieste ricevute falliti")
        messagebox.showerror("Errore", "Impossibile caricare le richieste ricevute dopo 3 tentativi")
