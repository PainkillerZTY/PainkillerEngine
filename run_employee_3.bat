@echo off
echo ========================================
echo  Launching AI Employee #3 - gameplay
echo  Task: ai_team\employee_3_gameplay\INBOX.md
echo ========================================
echo.
echo  Options:
echo  1. Interactive mode (recommended)
echo  2. Non-interactive mode (automated)
echo  3. View task only
echo.
set /p opt="Choose (1-3): "
if "%opt%"=="1" (
    claude ai_team\employee_3_gameplay\INBOX.md
) else if "%opt%"=="2" (
    type ai_team\employee_3_gameplay\INBOX.md | claude --print --allow-dangerously-skip-permissions
) else if "%opt%"=="3" (
    type ai_team\employee_3_gameplay\INBOX.md | more
) else (
    echo Invalid option
)
echo.
echo Done. Check ai_team\employee_3_gameplay\OUTBOX.md for results.
pause
