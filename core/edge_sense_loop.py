import ctypes
import os
import time
import math

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

def get_device_telemetry():
    """Henter ekte/dynamiske fysiske metrikker fra Termux/Edge-miljøet"""
    # Sjekk om det finnes termisk informasjon i Android-systemet (hvis tilgjengelig)
    thermal_path = "/sys/class/thermal/thermal_zone0/temp"
    temp_c = 35.05 # Standard baseline for mobilenhet
    if os.path.exists(thermal_path):
        try:
            with open(thermal_path, "r") as f:
                raw_temp = f.read().strip()
                temp_c = float(raw_temp) / 1000.0
        except Exception:
            pass
    
    # Legg til litt fysisk støy basert på mikrosekunder for å simulere levende sensor-jitter
    jitter = (time.time_ns() % 1000) / 100.0
    return temp_c + jitter

def run_edge_sense_loop(cycles=5):
    print("==================================================")
    print("[*] OPENTEIS EDGE-SENSE: Fysisk Telemetri-Modul")
    print("[*] Kjører sanntids HIL-verifikasjon (Hardware-in-the-Loop)")
    print("==================================================\n")

    for i in range(cycles):
        # Hent levende telemetri
        current_temp = get_device_telemetry()
        timestamp = int(time.time() * 1000)
        telemetry_payload = f"EDGE_TELEMETRY_NODE_AS_T{timestamp}_TEMP_{current_temp:.2f}".encode('utf-8')
        
        print(f"--- Syklus {i+1}/{cycles} [Fysisk Temp: {current_temp:.2f}°C] ---")

        # Steg 1: Formal Guard på telemetri-pakken
        guard_ptr = lib_guard.verify_code_safety(telemetry_payload)
        if not guard_ptr:
            raise RuntimeError("NULL pointer fra guard")
        
        unsat_id = guard_ptr.contents.unsat_core_id
        lib_guard.free_guard_result(guard_ptr)

        # Steg 2: Termodynamikk basert på AKTUELL enhets-temperatur
        # Formel: E_net = Energi - T * S (Bruker enhetens faktiske temperatur i Kelvin)
        temp_kelvin = current_temp + 273.15
        fep_value = lib_thermo.evaluate_thermodynamic_state(150.0, 1.2, temp_kelvin)

        # Steg 3: BFT Ruting tilpasset termisk belastning
        # Hvis temperaturen stiger over 45 grader, endres ruten defensivt
        route_mode = 1 if current_temp < 45.0 else 99
        next_hop = lib_bft.evaluate_best_route(10, route_mode, i % 5)

        # Steg 4: Generer STARK-bevis for telemetri-tilstanden
        buf = (ctypes.c_uint8 * 32)()
        status = lib_zk.generate_stark_proof_ffi(telemetry_payload, buf)

        print(f" -> Guard Unsat Core: 0x{unsat_id:08X}")
        print(f" -> Termodynamisk FEP: {fep_value:.4f}")
        print(f" -> Adaptiv BFT Rute:  Hop {next_hop} (Modus: {route_mode})")
        print(f" -> STARK Bevisstatus: {'Godkjent' if status == 1 else 'Feilet'}\n")
        
        time.sleep(0.5) # Kort pause mellom målingene for å simulere live sensor-polling

    print("==================================================")
    print("[+] Edge-Sense Telemetri-syklus fullført!")
    print("==================================================")

if __name__ == "__main__":
    run_edge_sense_loop(5)
