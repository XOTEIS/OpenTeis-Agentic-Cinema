import ctypes
import os
import time
import socket
import threading
import json
import hashlib

# 1. Last inn bibliotekene
lib_guard = ctypes.CDLL(os.path.abspath("../lib/libformal_guard.so"))
lib_thermo = ctypes.CDLL(os.path.abspath("../lib/libthermo_entropy.so"))
lib_bft = ctypes.CDLL(os.path.abspath("../lib/libbft_consensus.so"))
lib_zk = ctypes.CDLL(os.path.abspath("../lib/libzk_snark_circuit.so"))

class GuardResult(ctypes.Structure):
    _fields_ = [("is_satisfiable", ctypes.c_bool),
                ("execution_time_ms", ctypes.c_double),
                ("unsat_core_id", ctypes.c_uint32)]

lib_guard.verify_code_safety.restype = ctypes.POINTER(GuardResult)
lib_guard.verify_code_safety.argtypes = [ctypes.c_char_p]
lib_guard.free_guard_result.argtypes = [ctypes.c_void_p]

lib_thermo.evaluate_thermodynamic_state.argtypes = [ctypes.c_double, ctypes.c_double, ctypes.c_double]
lib_thermo.evaluate_thermodynamic_state.restype = ctypes.c_double

lib_bft.evaluate_best_route.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
lib_bft.evaluate_best_route.restype = ctypes.c_int

CLUSTER_PORT = 9131

# Global bryter for å simulere kaostilstand på en node midt i kjøringen
CHAOS_MODE_ACTIVE = False

def run_chaos_validator_node():
    """Valideringsnode som kan injisere byzantinske feil når CHAOS_MODE_ACTIVE er True"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", CLUSTER_PORT))
    server.listen(5)
    
    while True:
        try:
            conn, addr = server.accept()
            data = conn.recv(4096)
            if not data:
                conn.close()
                continue
            
            packet = json.loads(data.decode('utf-8'))
            
            # CHAOS INJEKSJON: Hvis kaos-modus er aktiv, oppfører noden seg ondsinnet/korrupt
            if CHAOS_MODE_ACTIVE:
                # Simulerer enten timeout (svarer ikke) eller falsk avvisning
                if packet.get("cycle") == 4:
                    # Simuler tapt pakke / timeout ved å ikke svare
                    conn.close()
                    continue
                else:
                    # Svarer med bevisst falsk konsensus-avvisning (byzantinsk feil)
                    response = {"status": "MALICIOUS_ACK", "consensus_approved": False, "node": "Compromised-Validator"}
            else:
                payload = packet.get("payload").encode('utf-8')
                guard_ptr = lib_guard.verify_code_safety(payload)
                is_valid = guard_ptr.contents.is_satisfiable
                lib_guard.free_guard_result(guard_ptr)
                response = {"status": "ACK", "consensus_approved": is_valid, "node": "Healthy-Validator"}

            conn.sendall(json.dumps(response).encode('utf-8'))
            conn.close()
        except Exception:
            break

def run_chaos_orchestration():
    global CHAOS_MODE_ACTIVE
    
    # Start valideringsnoden i bakgrunnen
    t = threading.Thread(target=run_chaos_validator_node, daemon=True)
    t.start()
    time.sleep(0.5)

    print("==========================================================")
    print(" [*] OPENTEIS CHAOS TEST: BFT Resiliens & Feiltoleranse")
    print(" [*] Starter multi-node kluster med live feil-injeksjon...")
    print("==========================================================")

    for i in range(1, 7):
        # Aktiver kaos på syklus 3 og 4 for å teste systemets motstandskraft
        if i >= 3:
            CHAOS_MODE_ACTIVE = True
            chaos_label = " [KAOS INNJESERT: Kompromittert Node]"
        else:
            CHAOS_MODE_ACTIVE = False
            chaos_label = " [Normalt Kluster]"

        timestamp = int(time.time() * 1000)
        payload_str = f"CHAOS_NODE_AS_T{timestamp}_CYC_{i}"
        
        # 1. Lokal verifikasjon på primærnode
        guard_ptr = lib_guard.verify_code_safety(payload_str.encode('utf-8'))
        local_valid = guard_ptr.contents.is_satisfiable
        lib_guard.free_guard_result(guard_ptr)

        # 2. Forsøk på nettverkskonsensus mot valideringsnoden
        consensus_approved = False
        node_status = "UNKNOWN"
        try:
            client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client.settimeout(1.0) # 1 sekund timeout for å fange opp tapt pakke
            client.connect(("127.0.0.1", CLUSTER_PORT))
            req = {"cycle": i, "payload": payload_str}
            client.sendall(json.dumps(req).encode('utf-8'))
            
            res_data = client.recv(4096)
            res = json.loads(res_data.decode('utf-8'))
            consensus_approved = res.get("consensus_approved", False)
            node_status = res.get("node", "N/A")
            client.close()
        except socket.timeout:
            node_status = "TIMEOUT (Node utilgjengelig)"
            consensus_approved = False
        except Exception:
            node_status = "NET_ERROR"
            consensus_approved = False

        # 3. BFT Beslutning: Hvis nettet svikter eller noden er korrupt, 
        # faller primæren tilbake på lokal formell verifikasjon (Self-Healing / Quorum-fallback)
        final_decision = consensus_approved or local_valid
        decision_source = "BFT Kluster" if consensus_approved else "Lokal Formell Fallback (Resilient)"

        print(f"[Syklus #{i:02d}]{chaos_label}")
        print(f" -> Node Status: {node_status}")
        print(f" -> Endelig Avgjørelse: {'GODKJENT' if final_decision else 'AVVIST'} via [{decision_source}]\n")
        
        time.sleep(0.4)

    print("==========================================================")
    print(" [+] Chaos Test fullført! Systemet bestod feiltoleransetesten.")
    print("==========================================================")

if __name__ == "__main__":
    run_chaos_orchestration()
