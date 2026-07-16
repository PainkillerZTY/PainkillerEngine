# AI Team Workspace

## Structure
Each employee has a sandbox directory:
```
employee_X_role/
??? INBOX.md      ? Task assigned (read this!)
??? OUTBOX.md     ? Work results (write this!)
??? CHAT_LOG.md   ? Full conversation log
??? patches/      ? Code changes as diff files
??? workspace/    ? Copy of project files to modify
```

## Workflow
1. Employee reads INBOX.md for task details
2. Employee works in workspace/ (copy of relevant source files)
3. Employee writes results to OUTBOX.md
4. Employee creates patch files in patches/
5. Employee writes CHAT_LOG.md
6. Leadership AI (Codex) reviews and integrates

## Rules
- Only modify files listed in INBOX.md
- Follow CODING_STANDARDS.md
- Test before submitting: cmake --build build --target painkiller
- All changes go through Codex review
