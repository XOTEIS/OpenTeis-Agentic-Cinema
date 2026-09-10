import ctypes
import os
import time

# 1. Last inn alle fire Level 5 moduler
lib_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "../lib/libformal_guard.so"))
lib_guard = ctypes.CDLL(lib_path)
lib_thermo = ctypes.CDLL(os.path.abspath("../lib/libthermo_entropy.so"))
lib_bft = ctypes.CDLL(os.path.abspath("../lib/libbft_consensus.so"))
lib_zk = ctypes.CDLL(os.path.abspath("../lib/libzk_snark_circuit.so"))

# 2. Definer C-strukturer for FFI
class GuardResult(ctypes.Structure):
    _fields_ = [("is_satisfiable", ctypes.c_bool),
                ("execution_time_ms", ctypes.c_double),
                ("unsat_core_id", ctypes.c_uint32)]

class ThermoState(ctypes.Structure):
    _fields_ = [("current_temp_c", ctypes.c_double),
                ("free_energy_metric", ctypes.c_double),
                ("allowed_depth_psi", ctypes.c_uint32),
                ("is_silent", ctypes.c_bool)]

class RouteDecision(ctypes.Structure):
    _fields_ = [("target_node_id", ctypes.c_uint32),
                ("confidence_score", ctypes.c_double),
                ("network_entropy", ctypes.c_double)]

class StarkProofContext(ctypes.Structure):
    _fields_ = [("state_root_hash", ctypes.c_uint8 * 32),
                ("trace_length", ctypes.c_uint64),
                ("validation_delta", ctypes.c_double),
                ("is_quantum_secure", ctypes.c_bool)]

# 3. Konfigurer FFI signaturer og returtyper
lib_guard.verify_code_safety.restype = ctypes.POINTER(GuardResult)
lib_guard.verify_code_safety.argtypes = [ctypes.c_char_p]

lib_guard.free_guard_result.argtypes = [ctypes.c_void_p]

if hasattr(lib_thermo, "evaluate_thermodynamic_state"):
    lib_thermo.evaluate_thermodynamic_state.argtypes = [ctypes.c_double, ctypes.c_double, ctypes.c_double]
    lib_thermo.evaluate_thermodynamic_state.restype = ctypes.c_double

if hasattr(lib_bft, "evaluate_best_route"):
    lib_bft.evaluate_best_route.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
    lib_bft.evaluate_best_route.restype = ctypes.c_int

if hasattr(lib_zk, "generate_stark_proof_ffi"):
    lib_zk.generate_stark_proof_ffi.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint8)]
    lib_zk.generate_stark_proof_ffi.restype = ctypes.c_int

# 4. Master Orchestration Loop
def run_master_loop():
    print("==================================================")
    print("[*] OPENTEIS PRIME: Level 5 Master Orchestration")
    print("[*] Kjører helhetlig siber-fysisk verifikasjonssyklus")
    print("==================================================\n")

    # Steg 1: Formal Guard med Live Datastream
    print("[Steg 1/4] Kjører libformal_guard på live datastream...")
    live_payload = f"STREAM_PACKET_T{int(time.time()*1000)}_G13_EDGE".encode('utf-8')
    guard_ptr = lib_guard.verify_code_safety(live_payload)
    if not guard_ptr:
        raise ValueError("Klarte ikke å hente GuardResult (NULL pointer)")
    
    guard_res = guard_ptr.contents
    print(f" -> Live Stream Validated: {guard_res.is_satisfiable}")
    print(f" -> Memory Execution Time: {guard_res.execution_time_ms} ms")
    print(f" -> Dynamic Unsat Core ID: 0x{guard_res.unsat_core_id:08X}")
    lib_guard.free_guard_result(guard_ptr)
    
    

    # Steg 2: Thermo Entropy
    print("\n[Steg 2/4] Kjører libthermo_entropy...")
    if hasattr(lib_thermo, "evaluate_thermodynamic_state"):
        fep = lib_thermo.evaluate_thermodynamic_state(100.0, 1.0, 298.15)
        print(f" -> FEP State Evaluated: {fep}")

    # Steg 3: BFT Consensus
    print("\n[Steg 3/4] Kjører libbft_consensus...")   
    if hasattr(lib_bft, "evaluate_best_route"):
        route = lib_bft.evaluate_best_route(5, 1, 4)
        print(f" -> BFT Best Route Next Hop: {route}")

    # Steg 4: ZK STARK Proof
    print("\n[Steg 4/4] Kjører libzk_snark_circuit...")
    if hasattr(lib_zk, "generate_stark_proof_ffi"):
        buffer = (ctypes.c_uint8 * 32)()
        status = lib_zk.generate_stark_proof_ffi(b"G-13_PAYLOAD", buffer)
        print(f" -> STARK Proof Generation Status: {status}")

    print("\n==================================================")
    print("[+] Level 5 Master Orchestration Fullført med Suksess!")
    print("==================================================")

if __name__ == "__main__":
    run_master_loop()
