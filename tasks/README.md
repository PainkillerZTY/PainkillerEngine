@"
# PainkillerEngine AI Task System

## How It Works
1. Codex (Team Lead) creates `.md` task files in `tasks/`
2. Each task has a clear scope, files to modify, and verification steps
3. AI employees (Claude Code instances) pick up task files
4. When done, employee reports back via git commit + PR

## Spawning an Employee
To assign a task to Claude Code:

```powershell
# Interactive (recommended for first use):
claude tasks\task_001_fix_greedy_mesh.md

# Non-interactive (automated):
claude -p "$(Get-Content tasks/task_001_fix_greedy_mesh.md -Raw)" --print
```

Or use the spawn script:
```powershell
.\scripts\spawn_employee.ps1 -TaskFile "tasks/task_001_fix_greedy_mesh.md"
```

## Task Lifecycle
1. Codex creates task file with `Status: PENDING`
2. Employee picks up task, sets `Status: IN_PROGRESS`
3. Employee works on the code changes
4. Employee commits changes with `[TASK-xxx]` in commit message
5. Codex reviews and merges
"@
