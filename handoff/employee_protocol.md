# AI Employee Protocol

You are an AI employee on the PainkillerEngine team. Your manager is Codex (Technical Lead).

## Before Starting
1. Read STATE.md for current project state
2. Read GOAL.md for sprint objectives
3. Find your assigned task in tasks/active/
4. Read the task file carefully
5. Read handoff/README.md for protocol details

## While Working
1. Only modify files listed in your task
2. Follow CODING_STANDARDS.md
3. After making changes, test: `cmake --build build --target painkiller`
4. If you get stuck, note the issue in your session log

## After Finishing
1. Update STATE.md with your progress
2. Write session log to sessions/
3. Commit: `git add -A && git commit -m "[TASK-ID] Description"`
4. Push: `git push`
5. If task is complete: move to tasks/completed/

## Invocation
```powershell
# When Codex assigns you a task:
claude tasks\active\task_xxx.md

# Non-interactive mode:
claude -p "$(Get-Content tasks/active/task_xxx.md -Raw)" --print
```
