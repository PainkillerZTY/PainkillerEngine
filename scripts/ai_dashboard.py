#!/usr/bin/env python3
"""PainkillerEngine AI Team Dashboard"""
import os, sys, datetime, json, subprocess, glob

ROOT = os.path.dirname(os.path.abspath(__file__))

def read_file(path):
    try:
        with open(os.path.join(ROOT, path), "r", encoding="utf-8") as f:
            return f.read()
    except: return ""

def write_file(path, content):
    with open(os.path.join(ROOT, path), "w", encoding="utf-8") as f:
        f.write(content)

def get_tasks(dir_name):
    tasks = []
    path = os.path.join(ROOT, "tasks", dir_name)
    if os.path.isdir(path):
        for fn in sorted(os.listdir(path)):
            if fn.endswith(".md"):
                tasks.append(fn)
    return tasks

def get_sessions():
    sessions = []
    path = os.path.join(ROOT, "sessions")
    if os.path.isdir(path):
        for fn in sorted(os.listdir(path), reverse=True)[:5]:
            sessions.append(fn)
    return sessions

def clear():
    os.system("cls" if os.name == "nt" else "clear")

def show_header():
    print("=" * 60)
    print("  PAINKILLER ENGINE - AI TEAM DASHBOARD")
    print("  Repository: PainkillerZTY/PainkillerEngine")
    print("=" * 60)
    print()

def show_state():
    state = read_file("STATE.md")
    lines = state.split("\n")
    in_section = ""
    for line in lines:
        if line.startswith("## SECTION"):
            in_section = line
        if "**Sprint Goal**" in line:
            print(f"  GOAL: {line.split(':')[-1].strip()}")
        if "Last Updated" in line:
            print(f"  State: {line.split('*')[1].strip() if '*' in line else line}")
    print()

def show_task_board():
    print("  ?? TASK BOARD ??????????????????????????????????")
    active = get_tasks("active")
    completed = get_tasks("completed")
    backlog = get_tasks("backlog")
    print(f"  ? Active:   {len(active)} task(s)")
    for t in active: print(f"  ?   ?? {t}")
    print(f"  ? Completed: {len(completed)} task(s)")
    for t in completed[-3:]: print(f"  ?   ? {t}")
    print(f"  ? Backlog:  {len(backlog)} task(s)")
    print("  ????????????????????????????????????????????????")
    print()

def show_team():
    print("  ?? TEAM STATUS ?????????????????????????????????")
    print("  ?  ?? You (Product Director)")
    print("  ?  ?? Codex (Tech Lead / Leadership AI)")
    print("  ?  ?? Employee #2 (Render) - IDLE")
    print("  ?  ?? Employee #3 (Gameplay) - UNASSIGNED")
    print("  ?  ?? Employee #4 (World) - IDLE")
    print("  ?  ?? Employee #5 (Network) - UNASSIGNED")
    print("  ????????????????????????????????????????????????")
    print()

def show_menu():
    print("  Commands:")
    print("  1. Start task (invoke Claude Code employee)")
    print("  2. Mark task complete")
    print("  3. Write session log")
    print("  4. Show last session")
    print("  5. Git status")
    print("  6. Build & test")
    print("  7. Refresh dashboard")
    print("  8. Exit")
    print()

def start_task():
    active = get_tasks("active")
    backlog = get_tasks("backlog")
    all_tasks = active + backlog
    print("\n  Available tasks:")
    for i, t in enumerate(all_tasks):
        status = "??" if t in active else "?"
        print(f"  {i+1}. {status} {t}")
    print("  0. Cancel")
    try:
        choice = int(input("\n  Select task: ")) - 1
        if choice >= 0 and choice < len(all_tasks):
            task_file = all_tasks[choice]
            src = "active" if task_file in active else "backlog"
            task_path = os.path.join(ROOT, "tasks", src, task_file)
            print(f"\n  Starting: {task_file}")
            print("  Options:")
            print("  1. Invoke Claude Code (interactive)")
            print("  2. Assign to me (I will do it now)")
            print("  0. Cancel")
            opt = input("\n  Choose: ")
            if opt == "1":
                subprocess.run(["claude", task_path], cwd=ROOT)
            elif opt == "2":
                if src == "backlog":
                    import shutil
                    shutil.move(task_path, os.path.join(ROOT, "tasks", "active", task_file))
                print(f"  Task {task_file} assigned to you. Begin work!")
    except: pass
    input("\n  Press Enter to continue...")

def mark_complete():
    active = get_tasks("active")
    print("\n  Active tasks:")
    for i, t in enumerate(active):
        print(f"  {i+1}. ?? {t}")
    print("  0. Cancel")
    try:
        choice = int(input("\n  Select completed task: ")) - 1
        if choice >= 0 and choice < len(active):
            task_file = active[choice]
            src = os.path.join(ROOT, "tasks", "active", task_file)
            dst = os.path.join(ROOT, "tasks", "completed", task_file)
            os.rename(src, dst)
            print(f"  ? {task_file} marked complete!")
    except: pass
    input("\n  Press Enter to continue...")

def write_session():
    print("\n  Write Session Log")
    name = input("  Your name: ") or "AI"
    summary = input("  Summary: ") or "Work completed"
    files = input("  Files changed (comma sep): ") or "N/A"
    next_steps = input("  Next steps: ") or "Review and continue"
    
    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M")
    content = f"""# Session Log: {ts}
- **AI**: {name}
- **Date**: {datetime.datetime.now().strftime("%Y-%m-%d %H:%M")}

## Summary
{summary}

## Files Changed
{files}

## Next Steps
{next_steps}
"""
    fn = os.path.join(ROOT, "sessions", f"session_{ts}_{name.lower().replace(' ','_')}.md")
    write_file(fn, content)
    print(f"  ? Session log written: {fn}")
    input("\n  Press Enter to continue...")

def show_last_session():
    sessions = get_sessions()
    if sessions:
        print(f"\n  Last session: {sessions[0]}")
        content = read_file(os.path.join("sessions", sessions[0]))
        print(f"  {content[:500]}...")
    else:
        print("  No sessions found.")
    input("\n  Press Enter to continue...")

def git_status():
    result = subprocess.run(["git", "status", "--short"], cwd=ROOT, capture_output=True, text=True)
    print(f"\n  Git Status:")
    print(f"  {result.stdout or '  (clean)'}")
    result2 = subprocess.run(["git", "log", "--oneline", "-3"], cwd=ROOT, capture_output=True, text=True)
    print(f"  Recent commits:")
    for line in result2.stdout.strip().split("\n") if result2.stdout else []:
        print(f"  {line}")
    input("\n  Press Enter to continue...")

def build_test():
    print("\n  Building...")
    result = subprocess.run(["cmake", "--build", "build", "--target", "painkiller"], 
                          cwd=ROOT, capture_output=True, text=True)
    lines = result.stdout.strip().split("\n") if result.stdout else []
    errors = [l for l in lines if "error" in l.lower()]
    if errors:
        print(f"  ? Build FAILED - {len(errors)} errors")
        for e in errors[:3]:
            print(f"  {e[:100]}")
    else:
        print(f"  ? Build SUCCESS")
    input("\n  Press Enter to continue...")

def main():
    while True:
        clear()
        show_header()
        show_state()
        show_task_board()
        show_team()
        show_menu()
        cmd = input("  Enter command (1-8): ").strip()
        if cmd == "1": start_task()
        elif cmd == "2": mark_complete()
        elif cmd == "3": write_session()
        elif cmd == "4": show_last_session()
        elif cmd == "5": git_status()
        elif cmd == "6": build_test()
        elif cmd == "7": continue
        elif cmd == "8":
            print("\n  Goodbye! Don't forget to:\n  1. git add -A && git commit -m \"[SESSION] summary\"\n  2. git push\n")
            break

if __name__ == "__main__":
    main()
