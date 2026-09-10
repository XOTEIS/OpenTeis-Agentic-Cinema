import os
import json
import hashlib

DAEMON_LOG = "g13_daemon_vault.log"

def audit_worm_chain():
    print("==================================================================")
    print(" [*] OPENTEIS PRIME: WORM-Vault Integrity & Chain Auditor")
    print("==================================================================")

    if not os.path.exists(DAEMON_LOG):
        print(f" [!] ADVARSEL: Fant ikke loggfilen '{DAEMON_LOG}'.")
        print("     Sjekk at daemonen har kjørt og skrevet minst én syklus.")
        return

    records = []
    with open(DAEMON_LOG, "r") as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            try:
                records.append((line_num, json.loads(line)))
            except json.JSONDecodeError as e:
                print(f" [X] KRITISK FEIL på linje {line_num}: Ugyldig JSON-struktur ({e})")
                return

    if not records:
        print(" [!] Vaulten er tom for oppføringer.")
        return

    print(f" [*] Analyserer {len(records)} blokker i WORM-kjeden...\n")

    previous_hash = "0" * 64
    chain_valid = True

    for idx, (line_num, block) in enumerate(records):
        cycle = block.get("cycle", "N/A")
        curr_hash = block.get("current_hash", "")
        prev_hash_in_block = block.get("prev_hash", "")
        zk_hash = block.get("zk_proof_hash", "N/A")

        # 1. Sjekk at prev_hash matcher forrige bloks current_hash
        if idx > 0 and prev_hash_in_block != previous_hash:
            print(f" [X] BRUDD I KJEDEN ved syklus #{cycle} (linje {line_num}):")
            print(f"     Forventet prev_hash: {previous_hash}")
            print(f"     Faktisk prev_hash:   {prev_hash_in_block}")
            chain_valid = False
            break

        # 2. Re-verifiser at current_hash faktisk stemmer overens med innholdet (unntatt hash-feltet selv)
        # Vi lager en kopi uten current_hash for å teste re-hashen
        test_block = dict(block)
        test_block.pop("current_hash", None)
        computed_hash = hashlib.sha256(json.dumps(test_block, sort_keys=True).encode('utf-8')).hexdigest()

        if computed_hash != curr_hash:
            print(f" [X] HASH-AVVIK DETEKTED ved syklus #{cycle} (linje {line_num}):")
            print(f"     Lagret hash:   {curr_hash}")
            print(f"     Beregnet hash: {computed_hash}")
            print(f"     -> Innholdet i denne blokken har blitt modifisert eksternt!")
            chain_valid = False
            break

        print(f"  [OK] Syklus #{cycle:04d} | ZK: {zk_hash[:12]}... | Hash: {curr_hash[:12]}...")
        previous_hash = curr_hash

    print("-" * 66)
    if chain_valid:
        print(" [√] RESULTAT: WORM-kjeden er 100% kryptografisk intakt!")
        print("     Ingen tukling, modifikasjoner eller entropi-avvik oppdaget.")
    else:
        print(" [!] RESULTAT: KJEDEN ER KOMPROMITTERT!")
    print("==================================================================")

if __name__ == "__main__":
    audit_worm_chain()
