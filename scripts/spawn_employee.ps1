param(
    [string]$TaskFile,
    [string]$EmployeeName = "AI Employee",
    [switch]$Interactive
)

if (-not (Test-Path $TaskFile)) {
    Write-Error "Task file not found: $TaskFile"
    exit 1
}

$taskContent = Get-Content $TaskFile -Raw
$prompt = "You are $EmployeeName working on the PainkillerEngine project.
Your task is below. Read the files mentioned, make the changes, and verify.
When done, create a git commit with your changes.

IMPORTANT RULES:
1. Only modify files listed in the task
2. Follow CODING_STANDARDS.md (4-space indent, m_ prefix for members)
3. Test: cmake --build build --target painkiller
4. Commit: git add -A && git commit -m ""[TASK] Description of changes""

TASK:
$taskContent"

if ($Interactive) {
    # Interactive mode
    $prompt | claude --model claude-sonnet-4-20250514
} else {
    # Non-interactive mode
    $tempFile = [System.IO.Path]::GetTempFileName() + ".md"
    $prompt | Out-File -FilePath $tempFile -Encoding UTF8
    Write-Host "Running task: $TaskFile"
    claude -p "$(Get-Content $tempFile -Raw)" --print --allow-dangerously-skip-permissions 2>&1
    Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
}
