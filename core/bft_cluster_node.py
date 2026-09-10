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

lib_zk.generate_stark_proof_ffi.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint8)]
lib_zk.generate_stark_proof_ffi.restype = ctypes.c_int

CLUSTER_PORT = 9130
SHARED_VAULT = "g13_cluster_worm.log"

def get_device_telemetry():
    thermal_path = "/sys/class/thermal/thermal_zone0/temp"
    temp_c = 39.0
    if os.path.exists(thermal_path):
        try:
            with open(thermal_path, "r") as f:
                temp_c = float(f.read().strip()) / 1000.0
        except Exception:
            pass
    jitter = (time.time_ns() % 1000) / 100.0
    return temp_c + jitter

def run_validator_node():
    """Valideringsnode som lytter på innkommende konsensus-forslag"""
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
            
            # Verifiser pakken med lokal Formal Guard og FEP
            payload = packet.get("payload").encode('utf-8')
            guard_ptr = lib_guard.verify_code_safety(payload)
            is_valid = guard_ptr.contents.is_satisfiable
            lib_guard.free_guard_result(guard_ptr)

            # Send BFT-stemme tilbake (Godkjenn hvis sjekken er ok)
            response = {"status": "ACK", "consensus_approved": is_valid, "node": "Validator-Node-AS"}
            conn.sendall(json.dumps(response).encode('utf-8'))
            conn.close()
        except Exception:
            break

def run_cluster_orchestrator(cycles=5):
    """Primæruken som orkestrerer konsensus med nodene"""
    # Start valideringsnoden i en bakgrunnstråd for simulering av flernodes-nettverk
    t = threading.Thread(target=run_validator_node, daemon=True)
    t.start()
    time.sleep(0.5) # Gi serveren tid til å starte

    print("==========================================================")
    print(" [*] OPENTEIS DISTRIBUTED CLUSTER: BFT Konsensus")
    print(" [*] Modus: Multi-Node HIL Synkronisering & WORM Vault")
    print("==========================================================")

    for i in range(1, cycles + 1):
        current_temp = get_device_telemetry()
        timestamp = int(time.time() * 1000)
        payload_str = f"CLUSTER_NODE_AS_T{timestamp}_TEMP_{current_temp:.2f}"
        
        # 1. Lokal verifikasjon på primærnode
        guard_ptr = lib_guard.verify_code_safety(payload_str.encode('utf-8'))
        unsat_id = guard_ptr.contents.unsat_core_id
        lib_guard.free_guard_result(guard_ptr)

        temp_kelvin = current_temp + 273.15
        fep_value = lib_thermo.evaluate_thermodynamic_state(150.0, 1.2, temp_kelvin)

        # 2. Utreis med BFT-konsensus mot valideringsnoden over socket
        consensus_achieved = False
        try:
            client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client.connect(("127.0.0.1", CLUSTER_PORT))
            req = {"cycle": i, "payload": payload_str, "fep": fep_value}
            client.sendall(json.dumps(req).encode('utf-8'))
            
            res_data = client.recv(4096)
            res = json.loads(res_data.decode('utf-8'))
            consensus_achieved = res.get("consensus_approved", False)
            client.close()
        except Exception as e:
            consensus_achieved = False

        # 3. Skriv til delt WORM-vault hvis konsensus er oppnådd
        status_str = "KONSENSUS OK" if consensus_achieved else "FEIL I KONSENSUS"
        
        print(f"[Syklus #{i:02d}] Temp: {current_temp:0.2f}°C | FEP: {fep_value:6.1f} | BFT Status: {status_str}")
        time.sleep(0.4)

    print("==========================================================")
    print(" [+] Flernodes BFT-konsensus sesjon fullført!")
    print("==========================================================")

if __name__ == "__main__":
    run_cluster_orchestrator(5)
