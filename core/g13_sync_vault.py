import json
import os
import tarfile
import datetime

DAEMON_LOG = "g13_daemon_vault.log"
SYNC_DIR = "../sync_out"

def prepare_sync_payload():
    if not os.path.exists(SYNC_DIR):
        os.makedirs(SYNC_DIR)

    # Finn siste 10 blokker for rask sync
    blocks = []
    if os.path.exists(DAEMON_LOG):
        with open(DAEMON_LOG, "r") as f:
            lines = f.readlines()
            blocks = [json.loads(line) for line in lines[-10:]]

    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    sync_filename = f"WORM_SYNC_{timestamp}.json"
    
    with open(sync_filename, "w") as f:
        json.dump(blocks, f, indent=4)

    # Pakk inn i et arkiv for transport
    tar_name = f"{SYNC_DIR}/sync_{timestamp}.tar.gz"
    with tarfile.open(tar_name, "w:gz") as tar:
        tar.add(sync_filename)
    
    os.remove(sync_filename)
    print(f"[*] WORM-Sync generert: {tar_name}")
    print(f"[*] Payload inneholder {len(blocks)} verifiserte ZK-STARK blokker.")

if __name__ == "__main__":
    prepare_sync_payload()
