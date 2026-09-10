from fastapi import FastAPI, HTTPException
import os
import json

app = FastAPI(title="OpenTeis Prime API", version="1.0.0")
DAEMON_LOG = "g13_daemon_vault.log"

@app.get("/")
def read_root():
    return {
        "system": "OpenTeis Prime",
        "status": "Operational",
        "mode": "HIL Edge-Sense + WORM Vault",
        "region": "Follo, Norway"
    }

@app.get("/status")
def get_latest_status():
    """Henter den aller nyeste telemetri- og verifikasjonsposten fra WORM-vaulten"""
    if not os.path.exists(DAEMON_LOG):
        raise HTTPException(status_code=404, detail="Daemon vault finnes ikke ennå.")

    try:
        with open(DAEMON_LOG, "r") as f:
            lines = f.readlines()
            if not lines:
                raise HTTPException(status_code=404, detail="Vaulten er tom.")
            latest_record = json.loads(lines[-1].strip())
            return latest_record
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/vault/history")
def get_vault_history(limit: int = 10):
    """Henter de siste N postene fra WORM-kjeden"""
    if not os.path.exists(DAEMON_LOG):
        return []

    try:
        with open(DAEMON_LOG, "r") as f:
            lines = f.readlines()
            records = [json.loads(line.strip()) for line in lines if line.strip()]
            return records[-limit:]
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
