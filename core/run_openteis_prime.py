import ctypes
import os

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
lib_prime = ctypes.CDLL(os.path.join(BASE_DIR, "libopenteis_prime.so"))

class OpenTeisNodeState(ctypes.Structure):
    _fields_ = [
        ("node_id", ctypes.c_uint64),
        ("telemetry_signal", ctypes.c_uint32),
        ("consensus_state", ctypes.c_uint8),
        ("zk_proof_hash", ctypes.c_uint64)
    ]

lib_prime.execute_prime_cycle.argtypes = [ctypes.POINTER(OpenTeisNodeState), ctypes.c_int, ctypes.c_uint32]
lib_prime.execute_prime_cycle.restype = ctypes.c_uint32

def test_prime_execution():
    print("--- Starter OpenTeis Prime Full Potensial-test ---")
    
    node_count = 5
    nodes = (OpenTeisNodeState * node_count)()
    
    # Simulerer 5 noder med telemetrisignaler
    signals = [0xAA001122, 0xAA001122, 0xAA001122, 0xAA001122, 0xDEADBEEF]
    
    for i in range(node_count):
        nodes[i].node_id = 100 + i
        nodes[i].telemetry_signal = signals[i]
        nodes[i].consensus_state = 0
        nodes[i].zk_proof_hash = 999888777
        
    target_mask = 0xFF000000
    result_entropy = lib_prime.execute_prime_cycle(nodes, node_count, target_mask)
    
    print(f"[OpenTeis Prime] Kollektiv Entropi-hash: {hex(result_entropy)}")
    print(f"[OpenTeis Prime] Node 0 Tilstand (Kvorum Godkjent): {bool(nodes[0].consensus_state)}")
    print(f"[OpenTeis Prime] Node 4 Tilstand (Isolert av BFT): {bool(nodes[4].consensus_state)}")
    print("--- OpenTeis er nå oppgradert til sitt fulle potensial ---")

if __name__ == "__main__":
    test_prime_execution()
