
# Legg til øverst sammen med andre importer
import subprocess

# ... inne i while True-loopen, etter at record er skrevet:
            # Automatisk Felt-Heartbeat: Trigger sync hver 100. syklus
            if cycle % 100 == 0:
                print(f"[*] Felt-Heartbeat: Triggerer automatisk WORM-Sync ved syklus {cycle}...")
                subprocess.Popen(["python3", "g13_sync_vault.py"])
import ctypes
import os
import time
import hashlib
import json
import sys

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

lib_thermo.evaluate_thermodynamic_state.argtypes = [ctypes.c_double, ctypes.c_double]
lib_thermo.evaluate_thermodynamic_state.restype = ctypes.c_double

lib_bft.evaluate_best_route.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
lib_bft.evaluate_best_route.restype = ctypes.c_int

lib_zk.generate_stark_proof_ffi.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint8)]
lib_zk.generate_stark_proof_ffi.restype = ctypes.c_int

DAEMON_LOG = "g13_daemon_vault.log"
PID_FILE = "openteis_daemon.pid"

def get_last_hash():
    if not os.path.exists(DAEMON_LOG):
        return "0" * 64
    last_line = ""
    try:
        with open(DAEMON_LOG, "r") as f:
            for line in f:
                if line.strip():
                    last_line = line.strip()
        if last_line:
            data = json.loads(last_line)
            return data.get("current_hash", "0" * 64)
    except Exception:
        pass
    return "0" * 64

def run_daemon():
    print("[*] OpenTeis Daemon starter opp...")
    cycle = 0
    
    # Finn siste kjede-hash for WORM-kontinuitet
    prev_hash = get_last_hash()
    if prev_hash != "0" * 64:
        print(f"[*] Fant eksisterer WORM-kjede. Fortsetter fra forrige hash...")

    try:
        while True:
            cycle += 1
            timestamp = time.time()
            
            # Simulér/hent fysiske parametere
            temp = 42.0 + (cycle % 10) * 0.42
            fep_val = lib_thermo.evaluate_thermodynamic_state(temp, 101.3)
            bft_hop = lib_bft.evaluate_best_route(cycle, 3, 2)
            
            # Formell sjekk via formal_guard
            code_snippet = f"sys_check_cycle_{cycle}".encode('utf-8')
            res_ptr = lib_guard.verify_code_safety(code_snippet)
            res = res_ptr.contents
            satisfiable = res.is_satisfiable
            unsat_core = res.unsat_core_id
            lib_guard.free_guard_result(res_ptr)

            # Generer ekte ZK-STARK bevis-buffer via FFI
            stark_buf = (ctypes.c_uint8 * 32)()
            payload_dict = {
                "cycle": cycle,
                "temperature": temp,
                "fep": fep_val,
                "unsat_core": unsat_core
            }
            payload_bytes = json.dumps(payload_dict, sort_keys=True).encode('utf-8')
            
            stark_status = lib_zk.generate_stark_proof_ffi(payload_bytes, stark_buf)
            zk_proof_hash = "".join(f"{b:02x}" for b in stark_buf)
            stark_ok = (stark_status == 0)

            # Bygg WORM-blokken uten current_hash først for å beregne gyldig SHA-256
            record = {
                "cycle": cycle,
                "timestamp": timestamp,
                "temperature_c": temp,
                "unsat_core_id": hex(unsat_core),
                "fep_metric": fep_val,
                "bft_next_hop": bft_hop,
                "stark_ok": stark_ok,
                "zk_proof_hash": zk_proof_hash,
                "prev_hash": prev_hash
            }

            # Beregn kryptografisk hash av blokken
            block_bytes = json.dumps(record, sort_keys=True).encode('utf-8')
            current_hash = hashlib.sha256(block_bytes).hexdigest()
            record["current_hash"] = current_hash

            # Skriv til WORM-Vault (Append-Only)
            with open(DAEMON_LOG, "a") as f:
                f.write(json.dumps(record) + "\n")

            prev_hash = current_hash
            time.sleep(1.0)

    except KeyboardInterrupt:
        print("\n[*] Daemon stanset.")

if __name__ == "__main__":
    run_daemon()
