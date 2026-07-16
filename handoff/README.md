# AI Team Handoff Protocol

## For Incoming AI (Starting a Shift)

1. **Read STATE.md** - Understand current project state, task board, team status
2. **Read GOAL.md** - Understand current sprint objective
3. **Read latest session log** - `sessions/` directory, most recent file
4. **Read your assigned task** - `tasks/active/` directory
5. **Read relevant source files** - Code files mentioned in the task
6. **Read CODING_STANDARDS.md** - Coding conventions
7. **Begin work**

## For Outgoing AI (Ending a Shift)

1. **Write session log** to `sessions/session_YYYYMMDD_HHMM_YourName.md`
   - What was accomplished
   - What was attempted but failed
   - Files modified
   - Current state of your task
   - Clear NEXT STEPS for the next AI
2. **Update STATE.md**
   - Section 3: Task board (move tasks, update status)
   - Section 4: Session log summary
   - Section 5: Next actions
3. **Stage and commit** all changes:
   ```bash
   git add -A
   git commit -m "[SESSION] YourName - Summary of work"
   git push
   ```
4. **If task is done**: Move task file from `tasks/active/` to `tasks/completed/`

## Communication
- All task assignments are documented in `tasks/active/`
- All questions should be written as GitHub Issues
- Urgent: Create issue with `[URGENT]` prefix
- Codex reviews all completed work before merging to main
