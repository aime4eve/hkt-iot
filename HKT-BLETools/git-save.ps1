# git-save.ps1
# 脚本功能：自动暂存所有更改并提交到本地 Git
# Usage: .\git-save.ps1 ["Your commit message"]

param (
    [Parameter(Position=0)]
    [string]$UserMessage
)

# 1. 检查当前目录是否在 Git 仓库中
git rev-parse --is-inside-work-tree > $null 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Git check failed. Not inside a git repository or git error." -ForegroundColor Red
    exit 1
}

# 2. 检查是否有未提交的更改
$status = git status --porcelain
if (-not $status) {
    Write-Host "Notice: No changes detected to commit." -ForegroundColor Yellow
    exit 0
}

# 3. 显示当前修改概览
Write-Host "--- Detected changes ---" -ForegroundColor Cyan
git status -s

# 4. 确定提交消息
$message = ""
if (-not [string]::IsNullOrWhiteSpace($UserMessage)) {
    $message = $UserMessage
} else {
    $inputMessage = Read-Host "Enter commit message (Press Enter for default timestamp)"
    if ([string]::IsNullOrWhiteSpace($inputMessage)) {
        $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        $message = "Manual Save: $timestamp"
    } else {
        $message = $inputMessage
    }
}

Write-Host "Using commit message: $message" -ForegroundColor Gray

# 5. 执行暂存和提交
Write-Host "Staging files..." -ForegroundColor Cyan
git add -A

# 由于可能在子目录中运行，并且有子模块或特殊情况导致未被提交，使用 -a
Write-Host "Committing..." -ForegroundColor Cyan
git commit -a -m "$message"

if ($LASTEXITCODE -eq 0) {
    Write-Host "Success! Changes saved to local repository." -ForegroundColor Green
} else {
    Write-Host "Failure: Git commit failed." -ForegroundColor Red
}