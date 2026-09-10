import os
import json
from http.server import HTTPServer, BaseHTTPRequestHandler

DAEMON_LOG = "g13_daemon_vault.log"
PORT = 8000

class OpenTeisAPIHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            response = {
                "system": "OpenTeis Prime",
                "status": "Operational",
                "mode": "HIL Edge-Sense + WORM Vault (Native API)",
                "region": "Follo, Norway"
            }
            self.wfile.write(json.dumps(response).encode('utf-8'))

        elif self.path == "/status":
            if not os.path.exists(DAEMON_LOG):
                self.send_error(404, "Daemon vault finnes ikke ennå.")
                return
            try:
                with open(DAEMON_LOG, "r") as f:
                    lines = f.readlines()
                    if not lines:
                        self.send_error(404, "Vaulten er tom.")
                        return
                    latest_record = json.loads(lines[-1].strip())
                    
                    self.send_response(200)
                    self.send_header("Content-Type", "application/json")
                    self.end_headers()
                    self.wfile.write(json.dumps(latest_record).encode('utf-8'))
            except Exception as e:
                self.send_error(500, str(e))

        elif self.path.startswith("/vault/history"):
            if not os.path.exists(DAEMON_LOG):
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps([]).encode('utf-8'))
                return
            try:
                with open(DAEMON_LOG, "r") as f:
                    lines = f.readlines()
                    records = [json.loads(line.strip()) for line in lines if line.strip()]
                    
                    self.send_response(200)
                    self.send_header("Content-Type", "application/json")
                    self.end_headers()
                    self.wfile.write(json.dumps(records[-10:]).encode('utf-8'))
            except Exception as e:
                self.send_error(500, str(e))
        else:
            self.send_error(404, "Endepunkt ikke funnet.")

    def log_message(self, format, *args):
        # Stillegåendelogging for å holde terminalen ren
        return

def run_server():
    server_address = ("127.0.0.1", PORT)
    httpd = HTTPServer(server_address, OpenTeisAPIHandler)
    print(f"[*] OpenTeis Native API kjører på http://127.0.0.1:{PORT}")
    print("[*] Endepunkter tilgjengelige: GET /, GET /status, GET /vault/history")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[*] Stopper API-serveren...")
        httpd.server_close()

if __name__ == "__main__":
    run_server()
