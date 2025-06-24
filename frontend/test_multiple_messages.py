#!/usr/bin/env python3
"""
Test per verificare se il client gestisce correttamente più messaggi consecutivi dal server
"""

import json
import time
import threading
from client_network import connect_to_server, send_to_server, receive_from_server, close_connections

def test_multiple_messages():
    """Test per verificare la gestione di più messaggi"""
    print("🔄 Test gestione multipli messaggi dal server...")
    
    # Connetti al server
    if not connect_to_server():
        print("❌ Impossibile connettersi al server")
        return
    
    print("✅ Connesso al server")
    
    # Simula la ricezione di messaggi in un loop simile a quello della GUI
    messages_received = []
    start_time = time.time()
    
    for i in range(10):  # Test per 10 cicli
        try:
            # Simula il polling come nella GUI
            messages = receive_from_server()
            if messages:
                print(f"📨 Ciclo {i+1}: Ricevuti {len(messages)} messaggi")
                for msg in messages:
                    try:
                        data = json.loads(msg)
                        messages_received.append(data)
                        print(f"  - Messaggio: {data.get('path', 'unknown')}")
                    except json.JSONDecodeError as e:
                        print(f"  - Errore parsing: {e}")
            else:
                print(f"📭 Ciclo {i+1}: Nessun messaggio")
            
            time.sleep(0.1)  # 100ms come nella GUI
            
        except Exception as e:
            print(f"❌ Errore nel ciclo {i+1}: {e}")
    
    end_time = time.time()
    
    print(f"\n📊 RISULTATI TEST:")
    print(f"   - Durata: {end_time - start_time:.2f}s")
    print(f"   - Messaggi totali ricevuti: {len(messages_received)}")
    print(f"   - Tipi di messaggio: {set(msg.get('path', 'unknown') for msg in messages_received)}")
    
    # Chiudi connessione
    close_connections()
    print("🔌 Connessione chiusa")

def test_message_buffer():
    """Test per verificare se i messaggi si accumulano nel buffer"""
    print("\n🔄 Test accumulo messaggi nel buffer...")
    
    if not connect_to_server():
        print("❌ Impossibile connettersi al server")
        return
    
    print("✅ Connesso al server")
    
    # Invia una richiesta che potrebbe generare messaggi
    try:
        response = send_to_server("/waiting_games", {})
        print(f"📤 Richiesta inviata, risposta: {response[:100]}...")
    except Exception as e:
        print(f"❌ Errore invio richiesta: {e}")
    
    # Aspetta un po' per far accumulare messaggi
    print("⏳ Attesa 2 secondi per accumulo messaggi...")
    time.sleep(2)
    
    # Ora leggi tutti i messaggi accumulati
    try:
        messages = receive_from_server()
        if messages:
            print(f"📨 Trovati {len(messages)} messaggi accumulati nel buffer")
            for i, msg in enumerate(messages):
                print(f"  {i+1}. {msg[:50]}...")
        else:
            print("📭 Nessun messaggio accumulato")
    except Exception as e:
        print(f"❌ Errore lettura buffer: {e}")
    
    close_connections()

if __name__ == "__main__":
    print("🧪 AVVIO TEST GESTIONE MULTIPLI MESSAGGI")
    print("=" * 50)
    
    test_multiple_messages()
    test_message_buffer()
    
    print("\n✅ Test completati")
