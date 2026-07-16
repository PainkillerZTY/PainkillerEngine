#!/usr/bin/env python3
import http.server, json, os, glob

ROOT = os.path.dirname(os.path.abspath(__file__))
PORT = 8080

def read_file(path):
    try:
        with open(os.path.join(ROOT, path), "r", encoding="utf-8") as f:
            return f.read()
    except:
        return ""

def list_tasks(folder):
    p = os.path.join(ROOT, "..", "tasks", folder)
    if os.path.isdir(p):
        return [f for f in sorted(os.listdir(p)) if f.endswith(".md")]
    return []

FRONTEND = read_file("dashboard.html")

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(FRONTEND.encode())
        elif self.path == "/api/data":
            # Tasks
            tasks = {
                "active": [{"n": f} for f in list_tasks("active")],
                "completed": [{"n": f} for f in list_tasks("completed")],
                "backlog": [{"n": f} for f in list_tasks("backlog")]
            }
            # Sessions
            sessions = []
            spath = os.path.join(ROOT, "..", "sessions")
            if os.path.isdir(spath):
                for fn in sorted(os.listdir(spath), reverse=True)[:10]:
                    if fn.endswith(".md"):
                        c = read_file(os.path.join("..", "sessions", fn))
                        d = fn.split("_")[0] if "_" in fn else ""
                        a = "AI"
                        s = ""
                        for line in c.split("\n"):
                            if "**AI**" in line:
                                a = line.split(":")[-1].strip()
                            if line.strip().startswith("- ") and not line.strip().startswith("- **") and not s:
                                s = line.strip()[2:80]
                        sessions.append({"d": d, "a": a, "s": s or fn.replace(".md", "")})
            # Team
            team = [
                {"n": "You", "t": "Product Director", "r": "you", "s": "active"},
                {"n": "Codex", "t": "Tech Lead", "r": "codex", "s": "active"},
                {"n": "Employee #2", "t": "Render Engineer", "r": "render", "s": "idle"},
                {"n": "Employee #3", "t": "Gameplay Engineer", "r": "gameplay", "s": "idle"},
                {"n": "Employee #4", "t": "World Engineer", "r": "world", "s": "off"},
                {"n": "Employee #5", "t": "Network Engineer", "r": "network", "s": "off"},
            ]
            # Check employee OUTBOX for active status
            for i, emp in enumerate(["employee_2_render", "employee_3_gameplay"]):
                outbox = os.path.join(ROOT, "..", "ai_team", emp, "OUTBOX.md")
                if os.path.isfile(outbox):
                    c = open(outbox, "r", encoding="utf-8").read()
                    if "IN_PROGRESS" in c:
                        team[i + 2]["s"] = "active"
            
            def sanitize(obj):
                if isinstance(obj, str):
                    return "".join(ch for ch in obj if 32 <= ord(ch) < 128)
                elif isinstance(obj, list):
                    return [sanitize(item) for item in obj]
                elif isinstance(obj, dict):
                    return {k: sanitize(v) for k, v in obj.items()}
                return obj
        
            data = sanitize({"tasks": tasks, "sessions": sessions, "team": team})
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps(data, ensure_ascii=False).encode())
        else:
            super().do_GET()
    
    def log_message(self, fmt, *args):
        print(f"[Dashboard] {args[0]}")

if __name__ == "__main__":
    server = http.server.HTTPServer(("0.0.0.0", PORT), Handler)
    print(f"Dashboard: http://localhost:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        server.server_close()
