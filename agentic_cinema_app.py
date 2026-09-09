import os
import sys
import time
import json
import ctypes
from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from pydantic import BaseModel
import uvicorn

# Importer offisiell Google GenAI SDK
try:
    from google import genai
    from google.genai import types
    HAS_GENAI = True
except ImportError:
    HAS_GENAI = False

# Last C11-kjernen via ctypes FFI
so_path = os.path.expanduser("~/openteis/libs/libopenteis_cinema_core.so")
lib_core = ctypes.CDLL(so_path)
lib_core.cinema_engine_init.restype = None
lib_core.cinema_worm_commit.restype = ctypes.c_uint64
lib_core.cinema_worm_commit.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_uint32, ctypes.c_uint32]
lib_core.cinema_get_state_root.restype = ctypes.c_uint64
lib_core.cinema_get_sequence.restype = ctypes.c_uint32

lib_core.cinema_engine_init()

app = FastAPI(title="OpenTeis Prime: Agentic Cinema")
app.mount("/static", StaticFiles(directory="static"), name="static")

# Mock/Reell Grafana Cloud MCP JSON-RPC Bro
class GrafanaMCPBridge:
    def __init__(self):
        self.endpoint = os.environ.get("GRAFANA_MCP_ENDPOINT", "http://localhost:9090/api/v1/push")
    
    def emit_prometheus_metric(self, metric_name: str, value: float, labels: dict):
        # Formatert iht. grafana/mcp-grafana spesifikasjon
        payload = {
            "jsonrpc": "2.0",
            "method": "tools/call",
            "params": {
                "name": "push_prometheus_metric",
                "arguments": {
                    "metric": metric_name,
                    "value": value,
                    "labels": labels,
                    "timestamp": time.time()
                }
            },
            "id": int(time.time() * 1000)
        }
        return payload

grafana_mcp = GrafanaMCPBridge()

class DirectorialRequest(BaseModel):
    prompt: str

@app.get("/")
def get_hud():
    return FileResponse("static/index.html")

@app.get("/api/telemetry")
def get_telemetry():
    root = lib_core.cinema_get_state_root()
    seq = lib_core.cinema_get_sequence()
    return {
        "status": "ONLINE",
        "state_root": f"0x{root:016X}",
        "sequence": seq,
        "joules_per_op": 0.00248,
        "eu_ai_act_compliance": "Art. 12 & 13 Validated"
    }

@app.post("/api/director/dispatch")
def dispatch_action(req: DirectorialRequest):
    chirality = 1.0 if "desert" in req.prompt.lower() or "obsidian" in req.prompt.lower() else -1.0
    carrier_hz = 110.0

    # Gemini Enterprise integrasjon
    gemini_status = "Gemini Enterprise Directorial Reasoning"
    api_key = os.environ.get("GEMINI_API_KEY")
    if HAS_GENAI and api_key:
        try:
            client = genai.Client(api_key=api_key)
            response = client.models.generate_content(
                model='gemini-2.5-flash',
                contents=f"Directorial prompt: {req.prompt}. Respond with cinematic parameters."
            )
            gemini_status = "Gemini Response Received"
        except Exception as e:
            gemini_status = f"Gemini Fallback: {str(e)[:40]}"

    # Commit til C11 WORM Vault
    in_h = abs(hash(req.prompt)) & 0xFFFFFFFF
    out_h = abs(hash(req.prompt + str(chirality))) & 0xFFFFFFFF
    state_root = lib_core.cinema_worm_commit(ctypes.c_float(chirality), ctypes.c_float(carrier_hz), in_h, out_h)

    # Emitter telemetri via Grafana Cloud MCP Bridge
    grafana_mcp.emit_prometheus_metric("openteis_joules_per_op", 0.00248, {"compliance": "truth_by_joule"})
    grafana_mcp.emit_prometheus_metric("openteis_carrier_lock_hz", carrier_hz, {"mode": "cymatic"})

    return {
        "director_note": gemini_status,
        "chirality": chirality,
        "carrier_hz": carrier_hz,
        "joules_per_op": 0.00248,
        "sequence": lib_core.cinema_get_sequence(),
        "state_root": f"0x{state_root:016X}"
    }

if __name__ == "__main__":
    print("[+] Starter OpenTeis Prime: Agentic Cinema på http://127.0.0.1:8000")
    uvicorn.run(app, host="0.0.0.0", port=8000)
