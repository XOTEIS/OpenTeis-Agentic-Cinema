import ctypes
import os
import time
import json
import statistics

# Last inn bibliotekene for benchmarking
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

def run_benchmark(iterations=100):
    print("==================================================================")
    print(f" [*] OPENTEIS PRIME: Offisiell FFI & ZK Benchmark ({iterations} sykluser)")
    print("==================================================================")

    latencies_guard = []
    latencies_thermo = []
    latencies_bft = []
    latencies_zk = []

    start_total = time.time()

    for i in range(iterations):
        # 1. Benchmark Formal Guard
        t0 = time.perf_counter()
        snippet = f"bench_node_{i}".encode('utf-8')
        res_ptr = lib_guard.verify_code_safety(snippet)
        lib_guard.free_guard_result(res_ptr)
        latencies_guard.append((time.perf_counter() - t0) * 1000.0)

        # 2. Benchmark Thermo Entropy
        t0 = time.perf_counter()
        lib_thermo.evaluate_thermodynamic_state(42.0 + (i * 0.1), 101.3)
        latencies_thermo.append((time.perf_counter() - t0) * 1000.0)

        # 3. Benchmark BFT Consensus
        t0 = time.perf_counter()
        lib_bft.evaluate_best_route(i, 3, 2)
        latencies_bft.append((time.perf_counter() - t0) * 1000.0)

        # 4. Benchmark ZK-STARK Proof Generation
        t0 = time.perf_counter()
        buf = (ctypes.c_uint8 * 32)()
        payload = json.dumps({"iteration": i, "val": 42.0}).encode('utf-8')
        lib_zk.generate_stark_proof_ffi(payload, buf)
        latencies_zk.append((time.perf_counter() - t0) * 1000.0)

    total_time = time.time() - start_total
    ops_per_sec = iterations / total_time

    print(f"[*] Total kjøretid  : {total_time:.4f} sekunder")
    print(f"[*] Gjennomstrømning: {ops_per_sec:.2f} fulle sykluser/sekund\n")
    print("------------------------------------------------------------------")
    print(" Gjennomsnittlig latens per FFI-komponent:")
    print(f"  - Formal Guard (SAT)  : {statistics.mean(latencies_guard):.4f} ms")
    print(f"  - Thermo Entropy (FEP): {statistics.mean(latencies_thermo):.4f} ms")
    print(f"  - BFT Consensus Route : {statistics.mean(latencies_bft):.4f} ms")
    print(f"  - ZK-STARK Proof Gen  : {statistics.mean(latencies_zk):.4f} ms")
    print("==================================================================")

if __name__ == "__main__":
    run_benchmark(100)
