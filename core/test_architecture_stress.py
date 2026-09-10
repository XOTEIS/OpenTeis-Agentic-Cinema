import ctypes
import os
import time

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

# Sett signaturer
lib_guard.verify_code_safety.restype = ctypes.POINTER(GuardResult)
lib_guard.verify_code_safety.argtypes = [ctypes.c_char_p]
lib_guard.free_guard_result.argtypes = [ctypes.c_void_p]

lib_thermo.evaluate_thermodynamic_state.argtypes = [ctypes.c_double, ctypes.c_double, ctypes.c_double]
lib_thermo.evaluate_thermodynamic_state.restype = ctypes.c_double

lib_bft.evaluate_best_route.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
lib_bft.evaluate_best_route.restype = ctypes.c_int

lib_zk.generate_stark_proof_ffi.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_uint8)]
lib_zk.generate_stark_proof_ffi.restype = ctypes.c_int

def run_stress_test(iterations=100):
    print(f"[*] Starter arkitektur-stressinvalidisering ({iterations} sykluser)...")
    start_time = time.time()

    for i in range(iterations):
        # Steg 1: Guard + Minneallokering/-frigjøring
        guard_ptr = lib_guard.verify_code_safety(b"src/formal_guard_core.c")
        if not guard_ptr:
            raise RuntimeError(fion=f"Feil på syklus {i}: NULL pointer fra guard")

        # Les ut data for å verifisere integritet
        _ = guard_ptr.contents.unsat_core_id
        lib_guard.free_guard_result(guard_ptr)

        # Steg 2: Termodynamikk
        _ = lib_thermo.evaluate_thermodynamic_state(100.0 + i, 1.0, 298.15)

        # Steg 3: BFT Ruting
        _ = lib_bft.evaluate_best_route(10, i % 10, (i + 3) % 10)

        # Steg 4: STARK Proof
        buf = (ctypes.c_uint8 * 32)()
        status = lib_zk.generate_stark_proof_ffi(b"STRESS_TEST_PAYLOAD", buf)
        if status != 1:
            raise RuntimeError(f"STARK feil på syklus {i}")

    elapsed = time.time() - start_time
    print(f"[+] Fullført {iterations} sykluser uten avvik!")
    print(f"[+] Total tid: {elapsed:.4f} sekunder ({elapsed/iterations*1000:.2f} ms per syklus).")

if __name__ == "__main__":
    run_stress_test(100)
