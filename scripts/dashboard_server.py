
#!/usr/bin/env python3
import http.server, json, os, re, glob

ROOT = os.path.dirname(os.path.abspath(__file__))
PORT = 8080

HTML = '''<!DOCTYPE html><html lang="zh-CN"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>PainkillerEngine Dashboard</title>
<style>
:root{--bg:#0f1117;--bg2:#1a1d27;--bg3:#242838;--text:#e4e6f0;--text2:#8b90a0;--accent:#6c8cff;--green:#4ade80;--yellow:#facc15;--red:#ef4444;--radius:12px}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--text);line-height:1.5}
.header{background:linear-gradient(135deg,#1a1d27 0%,#242838 100%);border-bottom:1px solid rgba(108,140,255,0.15);padding:20px 32px}
.header h1{font-size:24px;font-weight:700}.header h1 span{color:var(--accent)}.header .sub{color:var(--text2);font-size:14px;margin-top:4px}
.container{max-width:1400px;margin:0 auto;padding:24px 32px}
.grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:20px;margin-bottom:24px}
.card{background:var(--bg2);border:1px solid rgba(255,255,255,0.06);border-radius:var(--radius);padding:20px}
.card h3{font-size:13px;text-transform:uppercase;letter-spacing:0.5px;color:var(--text2);margin-bottom:12px}
.card .big{font-size:32px;font-weight:700;margin-bottom:4px}
.team-grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-top:16px}
.member{display:flex;align-items:center;gap:10px;padding:8px 12px;background:var(--bg3);border-radius:8px}
.member .avatar{width:32px;height:32px;border-radius:8px;display:flex;align-items:center;justify-content:center;font-size:16px}
.member .name{font-size:13px;font-weight:600}.member .role{font-size:11px;color:var(--text2)}
.member .status{width:8px;height:8px;border-radius:50%}
.s-active{background:var(--green);box-shadow:0 0 8px var(--green)}.s-idle{background:var(--yellow)}.s-off{background:var(--text2)}
.task-item{display:flex;align-items:center;gap:10px;padding:8px 12px;background:var(--bg3);border-radius:8px;margin-bottom:6px}
.task-item .title{font-size:13px;font-weight:500}.task-item .assignee{font-size:11px;color:var(--text2);margin-left:auto}
.session-item{padding:10px 12px;background:var(--bg3);border-radius:8px;margin-bottom:6px}
.session-item .date{font-size:11px;color:var(--text2)}.session-item .summary{font-size:13px;margin-top:4px}
.two-col{display:grid;grid-template-columns:2fr 1fr;gap:20px;margin-bottom:24px}
.loading{text-align:center;padding:40px;color:var(--text2)}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}.loading span{animation:pulse 1.5s ease-in-out infinite}
@media(max-width:1024px){.grid{grid-template-columns:1fr 1fr}}@media(max-width:768px){.grid,.two-col{grid-template-columns:1fr}}
</style></head><body>
<div class="header"><h1>??? <span>PainkillerEngine</span> AI Team Dashboard</h1>
<div class="sub"><span id="updateTime">Loading...</span></div></div>
<div class="container" id="app"><div class="loading"><span>Loading dashboard data...</span></div></div>
<script>
async function loadData(){try{
const[s,t,ss,tm]=await Promise.all([fetch("/api/state").then(r=>r.json()),fetch("/api/tasks").then(r=>r.json()),fetch("/api/sessions").then(r=>r.json()),fetch("/api/team").then(r=>r.json())]);
document.getElementById("updateTime").textContent="Updated: "+new Date().toLocaleString();
let h=`<div class="grid"><div class="card"><h3>?? Active</h3><div class="big" style="color:var(--yellow)">${t.active.length}</div><div style="color:var(--text2);font-size:13px">${t.backlog.length} backlog</div></div><div class="card"><h3>? Completed</h3><div class="big" style="color:var(--green)">${t.completed.length}</div></div><div class="card"><h3>?? Team</h3><div class="big" style="color:var(--accent)">${tm.members.filter(m=>m.s!=="off").length}/5</div><div style="color:var(--text2);font-size:13px">Employees online</div></div></div>`;
h+=`<div class="two-col"><div class="card"><h3>?? Active Tasks</h3>`;
if(t.active.length===0)h+=`<div style="color:var(--text2);padding:12px;font-size:13px">No active tasks</div>`;
t.active.forEach(i=>{h+=`<div class="task-item"><span>??</span><span class="title">${i.n.replace(".md","")}</span><span class="assignee">${i.a||"Unassigned"}</span></div>`});
h+=`</div><div class="card"><h3>?? Team</h3><div class="team-grid">`;
const em={you:"??",codex:"??",render:"??",gameplay:"??",world:"??",network:"??"};
tm.members.forEach(m=>{h+=`<div class="member"><div class="avatar" style="background:rgba(108,140,255,0.15)">${em[m.r]||"??"}</div><div class="info"><div class="name">${m.n}</div><div class="role">${m.t}</div></div><div class="status s-${m.s}"></div></div>`});
h+=`</div></div></div>`;
h+=`<div class="grid"><div class="card"><h3>? Completed</h3>`;
t.completed.slice(-5).reverse().forEach(i=>{h+=`<div class="task-item"><span>?</span><span class="title">${i.n.replace(".md","")}</span></div>`});
if(t.completed.length===0)h+=`<div style="color:var(--text2);padding:12px;font-size:13px">None</div>`;
h+=`</div><div class="card"><h3>?? Sessions</h3>`;
ss.slice(0,5).forEach(s=>{h+=`<div class="session-item"><div class="date">${s.d} ? ${s.a}</div><div class="summary">${s.s||""}</div></div>`});
if(ss.length===0)h+=`<div style="color:var(--text2);padding:12px;font-size:13px">None</div>`;
h+=`</div><div class="card"><h3>?? Backlog</h3>`;
t.backlog.forEach(i=>{h+=`<div class="task-item"><span>?</span><span class="title">${i.n.replace(".md","")}</span></div>`});
if(t.backlog.length===0)h+=`<div style="color:var(--text2);padding:12px;font-size:13px">Empty</div>`;
h+=`</div></div>`;document.getElementById("app").innerHTML=h;}
catch(e){document.getElementById("app").innerHTML='<div class="loading" style="color:var(--red)">Failed to load</div>'}
}
loadData();setInterval(loadData,30000);
</script></body></html>'''

def read_file(path):
    try:
        p = os.path.join(ROOT, "..", path)
        with open(p, "r", encoding="utf-8") as f:
            return f.read()
    except: return ""

def list_md(d):
    p = os.path.join(ROOT, "..", d)
    if os.path.isdir(p):
        return sorted(os.listdir(p))
    return []

class H(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200); self.send_header("Content-Type","text/html; charset=utf-8"); self.end_headers()
            self.wfile.write(HTML.encode())
        elif self.path == "/api/tasks":
            r={"active":[],"completed":[],"backlog":[]}
            for k in r:
                for f in list_md(f"tasks/{k}"):
                    if f.endswith(".md"): r[k].append({"n":f})
            self.json(r)
        elif self.path == "/api/sessions":
            ss=[]
            for fn in sorted(glob.glob(os.path.join(ROOT,"..","sessions","*.md")),reverse=True)[:10]:
                c=read_file("sessions/"+os.path.basename(fn))
                d=os.path.basename(fn).split("_")[0] if "_" in os.path.basename(fn) else ""
                a="AI"; s=""
                for line in c.split("
"):
                    if "**AI**" in line: a=line.split(":")[-1].strip()
                    if line.strip().startswith("- "): s=line.strip()[2:80]
                ss.append({"d":d,"a":a,"s":s or os.path.basename(fn).replace(".md","")})
            self.json(ss)
        elif self.path == "/api/team":
            m=[{"n":"You","t":"Product Director","r":"you","s":"active"},
               {"n":"Codex","t":"Tech Lead","r":"codex","s":"active"},
               {"n":"Employee #2","t":"Render Engineer","r":"render","s":"idle"},
               {"n":"Employee #3","t":"Gameplay Engineer","r":"gameplay","s":"idle"},
               {"n":"Employee #4","t":"World Engineer","r":"world","s":"off"},
               {"n":"Employee #5","t":"Network Engineer","r":"network","s":"off"}]
            self.json({"members":m})
        else: super().do_GET()
    def json(self,d):
        self.send_response(200); self.send_header("Content-Type","application/json; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin","*"); self.end_headers()
        self.wfile.write(json.dumps(d,ensure_ascii=False).encode())
    def log_message(self,f,*a): print(f"[Dashboard] {a[0]}")

if __name__=="__main__":
    s=http.server.HTTPServer(("0.0.0.0",PORT),H)
    print(f"Dashboard: http://localhost:{PORT}")
    try: s.serve_forever()
    except: s.server_close()
