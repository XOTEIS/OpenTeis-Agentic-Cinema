import os
import json
import hashlib

VAULT_FILE = "g13_worm_vault.log"

def verify_worm_vault():
    print("==================================================")
    print("[*] OPENTEIS VAULT VERIFIER: Integritetskontroll")
    print("[*] Verifiserer SHA-256 WORM-kjeden...")
    print("==================================================\n")

    if not os.path.exists(VAULT_FILE):
        print(f"[!] Feil: Fant ikke vault-filen '{VAULT_FILE}'.")
        return False

    with open(VAULT_FILE, "r") as f:
        lines = f.readlines()

    if not lines:
        print("[*] Vault-en er tom.")
        return True

    expected_prev_hash = "0" * 64
    corrupted = False

    for idx, line in enumerate(lines):
        line = line.strip()
        if not line:
            continue

        try:
            record = json.loads(line)
        except json.JSONDecodeError:
            print(f"[!] KRITISK: Ugyldig JSON på linje {idx + 1}")
            corrupted = True
            break

        # Hent ut lagret hash og forrige hash
        stored_current_hash = record.get("current_hash")
        stored_prev_hash = record.get("prev_hash")

        # 1. Sjekk at forrige hash matcher forventet kjede
        if stored_prev_hash != expected_prev_hash:
            print(f"[!] INTEGRITETSBRUDD på syklus {record.get('cycle')} (Linje {idx + 1}):")
            print(f"    Forventet prev_hash: {expected_prev_hash}")
            print(f"    Lagret prev_hash:   {stored_prev_hash}")
            corrupted = True
            break

        # 2. Rekalkuler hashen fra posten (fjern 'current_hash' midlertidig under beregning)
        record_copy = record.copy()
        record_copy.pop("current_hash", None)
        recalculated_hash = hashlib.sha256(json.dumps(record_copy, sort_keys=True).encode('utf-8')).hexdigest()

        if recalculated_hash != stored_current_hash:
            print(f"[!] DATATUKLING OPPDAGET på syklus {record.get('cycle')} (Linje {idx + 1}):")
            print(f"    Rekalkulert hash: {recalculated_hash}")
            print(f"    Lagret hash:      {stored_current_hash}")
            corrupted = True
            break

        print(f"[OK] Syklus {record.get('cycle')} [Temp: {record.get('temperature_c')}°C] -> Hash OK")
        expected_prev_hash = stored_current_hash

    print("\n==================================================")
    if corrupted:
        print("[X] FEIL: WORM-vaulten er skadet eller tuklet med!")
        print("==================================================")
        return False
    else:
        print(f"[+] SUKSESS: Alle {len(lines)} poster er verifisert. Kjeden er 100% intakt.")
        print("==================================================")
        return True

if __name__ == "__main__":
    verify_worm_vault()
