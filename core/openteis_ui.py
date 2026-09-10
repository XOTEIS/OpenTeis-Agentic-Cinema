import time
import urllib.request
import json

API_URL = "http://127.0.0.1:8000/status"
HISTORY_URL = "http://127.0.0.1:8000/vault/history?limit=5"

def fetch_json(url):
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'OpenTeis-UI'})
        with urllib.request.urlopen(req, timeout=1.5) as response:
            return json.loads(response.read().decode('utf-8'))
    except Exception:
        return None

def run_control_room():
    try:
        while True:
            status_data = fetch_json(API_URL)
            history_data = fetch_json(HISTORY_URL)

            # Rens skjermen og tegn opp kontrollrommet på nytt
            print("\033[H\033[J", end="")
            print("==================================================================")
            print(" [*] OPENTEIS PRIME: ZK-STARK Metrology & Control Room")
            print("==================================================================")

            if not status_data:
                print(" [!] ADVARSEL: Kunne ikke koble til API-serveren på port 8000.")
                print("     Sjekk at './start_openteis.sh' kjører.")
            else:
                cycle = status_data.get("cycle", 0)
                temp = status_data.get("temperature_c", 0.0)
                fep = status_data.get("fep_metric", 0.0)
                unsat = status_data.get("unsat_core_id", "N/A")
                hop = status_data.get("bft_next_hop", 0)
                stark_ok = status_data.get("stark_ok", False)
                zk_hash = status_data.get("zk_proof_hash", "0" * 32)
                curr_hash = status_data.get("current_hash", "0" * 64)

                # Termometer-bar
                bar_len = int(max(0, min(25, (temp - 35.0) * 2)))
                therm_bar = "[" + "#" * bar_len + "-" * (25 - bar_len) + "]"

                print(f" Siste Syklus   : #{cycle:04d}")
                print(f" Temperatur     : {temp:0.2f}°C  {therm_bar}")
                print(f" FEP-verdi      : {fep:6.1f}")
                print(f" Unsat Core     : {unsat}")
                print(f" BFT Next Hop   : {hop}")
                print(f" STARK Status   : [{'MATEMATISK VERIFISERT' if stark_ok else 'FEILEDE BEVIS'}]")
                print(f" ZK-Proof Hash  : {zk_hash}")
                print(f" WORM Block     : {curr_hash[:16]}...{curr_hash[-16:]}")
                print("-" * 66)

            if history_data and isinstance(history_data, list):
                print(" Siste ZK-Audit Trail (WORM-kjede):")
                for h in history_data[-5:]:
                    print(f"  -> [# {h.get('cycle', 0):04d}] ZK: {h.get('zk_proof_hash', 'N/A')[:12]}... | Hash: {h.get('current_hash', '')[:10]}...")

            print("==================================================================")
            print(" Trykk [Ctrl + C] for å avslutte kontrollrommet.")
            
            time.sleep(1.0)

    except KeyboardInterrupt:
        print("\n\n[*] Kontrollrom stanset av bruker.")

if __name__ == "__main__":
    run_control_room()
