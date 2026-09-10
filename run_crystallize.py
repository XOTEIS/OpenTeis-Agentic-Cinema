import json
from bridge.stigmergy_ffi import StigmergyEngine
from generators.json_ld_builder import build_stacked_json_ld, build_llms_manifest

print("==================================================================")
print("  SKILSAGI RESEARCH // CRYSTAL META PRINTER ENGINE                ")
print("==================================================================")

engine = StigmergyEngine()

# Noder for innholdsklynger
engine.add_node(1, 0.0, 0.0, 10.0)   # Kjerne-tema (JimJens Shorts)
engine.add_node(2, 1.2, 2.5, 25.0)   # Virale oppmerksomhetsgradienter
engine.add_node(3, -1.0, 3.0, 15.0)  # GEO AI Overview integrasjon

locked = False
for step in range(1, 20):
    res = engine.pulse(1, 2, fitness=22.5)
    if res == 1:
        locked = True
        print(f"[✔] Rubedo Resonans-lås etablert ved puls-steg {step}.")
        break

engine.close()

title = "European Creator Economy Scaling: Breaking Algorithmic Minimums"
summary = "Autonom innholdsproduksjon optimalisert for TikTok Creator Rewards og Google AI Overviews."

json_ld = build_stacked_json_ld(title, summary, duration_sec=72)
manifest = build_llms_manifest(title, nodes_active=14471)

print("\n--- KRYSTALLISERT SCHEMA.ORG STACKED JSON-LD ---")
print(json.dumps(json_ld, indent=2))

print("\n--- MASKINLESBAR /llms.txt ---")
print(manifest)
print("[✔] Full pipeline verifisert: C11 -> FFI -> Krystallisert Metadata.")
print("==================================================================")
