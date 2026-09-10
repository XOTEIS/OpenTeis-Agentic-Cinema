import ctypes
import os

lib_path = os.path.join(os.path.dirname(__file__), '../core/libopenteis_stigmergy.so')
_lib = ctypes.CDLL(lib_path)

_lib.openteis_stigmergy_init.restype = ctypes.c_void_p
_lib.openteis_stigmergy_add_node.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_double, ctypes.c_double, ctypes.c_double]
_lib.openteis_stigmergy_add_node.restype = ctypes.c_int

_lib.openteis_stigmergy_pulse.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_double]
_lib.openteis_stigmergy_pulse.restype = ctypes.c_int

_lib.openteis_stigmergy_free.argtypes = [ctypes.c_void_p]

class StigmergyEngine:
    def __init__(self):
        self.state_ptr = _lib.openteis_stigmergy_init()
        if not self.state_ptr:
            raise MemoryError("Kunne ikke allokere OpenTeisStigmergyState_t")

    def add_node(self, node_id: int, x: float, y: float, nutrient: float) -> int:
        return _lib.openteis_stigmergy_add_node(self.state_ptr, node_id, x, y, nutrient)

    def pulse(self, src_id: int, tgt_id: int, fitness: float) -> int:
        return _lib.openteis_stigmergy_pulse(self.state_ptr, src_id, tgt_id, fitness)

    def close(self):
        if self.state_ptr:
            _lib.openteis_stigmergy_free(self.state_ptr)
            self.state_ptr = None

    def __del__(self):
        self.close()
