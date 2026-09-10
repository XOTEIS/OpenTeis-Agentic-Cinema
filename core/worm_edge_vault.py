import ctypes
import os
import time
import hashlib
import json

# 1. Last inn bibliotekene
lib_guard = ctypes.CDLL(os.path.abspath("../lib/libformal_guard.so"))
lib_thermo = ctypes.CDLL(os.path.abspath("../lib/libthermo_entropy.so"))
lib_bft = ctypes.CDLL(os.path.abspath("../lib/libbft_consensus.so"))
lib_zk = ctypes.CDLL(os.path.abspath("../lib/libzk_snark_circuit.so"))

# 2. Datastrukturer
class GuardResult(ctypes.Structure):
    _fields_ = [("is_satisfiable", ctypes.c_bool),
                ("execution_time_ms", ctypes.c_double),
                ("unsat_core_id", ctypes.c_uint32)]

# 3. Sett FFI-signaturer
lib_guard.verify_code_safety.restype = ctypes.POINTER(GuardResult)
lib_guard.verify_code_safety.argtypes = [ctypes.c_char_p]
lib_guard.free_guard_result.argtypes = [ctypes.c_void_p]

lib_thermo.evaluate_thermodynamic_state.argtypes = [ctypes.c_double, ctypes.c_double, ctypes.c_double]
lib_thermo.evaluate_thermodynamic_state.restype = ctypes.c_double

lib_bft.evaluate_best_route.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
lib_bft.evaluate_best_route.restype = ctypes.c_int

lib_zk.generate_stark_proof_ffi.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint8)]
lib_zk.generate_stark_proof_ffi.restype = ctypes.c_int

VAULT_FILE = "g13_worm_vault.log"

def get_device_telemetry():
    thermal_path = "/sys/class/thermal/thermal_zone0/temp"
    temp_c = 37.5
    if os.path.exists(thermal_path):
        try:
            with open(thermal_path, "r") as f:
                temp_c = float(f.read().strip()) / 1000.0
        except Exception:
            pass
    jitter = (time.time_ns() % 1000) / 100.0
    return temp_c + jitter

def get_last_worm_hash():
    """Henter hashen av den forrige oppføringen for å opprettholde kjeden (blockchain-lite WORM)"""
    if not os.path.exists(VAULT_FILE):
        return "0" * 64
    try:
        with open(VAULT_FILE, "r") as f:
            lines = f.readlines()
            if not lines:
                return "0" * 64
            last_entry = json.loads(lines[-1].strip())
            return last_entry.get("current_hash", "0" * 64)
    except Exception:
        return "0" * 64

def append_to_worm_vault(record):
    """Skriver uforanderlig til WORM-loggen (append-only med SHA-256 kobling)"""
    prev_hash = get_last_worm_hash()
    record["prev_hash"] = prev_hash
    
    # Beregn kryptografisk hash av innholdet inkludert forrige hash
    record_string = json.dumps(record, sort_keys=True)
    current_hash = hashlib.sha256(record_string.encode('utf-8')).hexdigest()
    record["current_hash"] = current_hash

    # Skriv til fil (Write-Once append)
    with open(VAULT_FILE, "a") as f:
        f.write(json.dumps(record) + "\n")
    
    return current_hash

def run_worm_vault_loop(cycles=5):
    print("==================================================")
    print("[*] OPENTEIS WORM VAULT: Uforanderlig Telemetri-Logg")
    print("[*] HIL-data sikres med SHA-256 WORM-kjeding")
    print("==================================================\n")

    for i in range(cycles):
        current_temp = get_device_telemetry()
        timestamp = int(time.time() * 1000)
        telemetry_payload = f"WORM_NODE_AS_T{timestamp}_TEMP_{current_temp:.2f}".encode('utf-8')

        # Steg 1: Formal Guard
        guard_ptr = lib_guard.verify_code_safety(telemetry_payload)
        unsat_id = guard_ptr.contents.unsat_core_id
        lib_guard.free_guard_result(guard_ptr)

        # Steg 2: Termodynamikk
        temp_kelvin = current_temp + 273.15
        fep_value = lib_thermo.evaluate_thermodynamic_state(150.0, 1.2, temp_kelvin)

        # Steg 3: BFT Ruting
        next_hop = lib_bft.evaluate_best_route(10, 1, i % 5)

        # Steg 4: STARK Bevis
        buf = (ctypes.c_uint8 * 32)()
        status = lib_zk.generate_stark_proof_ffi(telemetry_payload, buf)
        stark_hex = ''.join([f"{b:02x}" for b in buf])

        # Bygg WORM-post
        record = {
            "cycle": i + 1,
            "timestamp": timestamp,
            "temperature_c": round(current_temp, 2),
            "unsat_core_id": f"0x{unsat_id:08X}",
            "fep_metric": round(fep_value, 4),
            "bft_next_hop": next_hop,
            "stark_proof_prefix": stark_hex[:16] + "..."
        }

        # Forsegl i WORM-vault
        vault_hash = append_to_worm_vault(record)

        print(f"--- Syklus {i+1}/{cycles} [Temp: {current_temp:.2f}°C] ---")
        print(f" -> WORM Hash Forseglet: {vault_hash[:16]}...")
        print(f" -> FEP: {fep_value:.2f} | Unsat: 0x{unsat_id:08X}\n")

        time.sleep(0.3)

    print("==================================================")
    print(f"[+] WORM Vault-lagring fullført! Logg lagret i: {VAULT_FILE}")
    print("==================================================")

if __name__ == "__main__":
    run_worm_vault_loop(5)
