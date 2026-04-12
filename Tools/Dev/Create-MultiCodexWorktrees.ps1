[CmdletBinding()]
param(
    [string]$BaseRef = "main"
)

$ErrorActionPreference = "Stop"

function Get-TrimmedGitOutput {
    param(
        [string[]]$GitArgs
    )

    $output = & git @GitArgs
    if ($LASTEXITCODE -ne 0) {
        throw "git $($GitArgs -join ' ') failed."
    }

    return ($output | Out-String).Trim()
}

$repoRoot = Get-TrimmedGitOutput -GitArgs @("rev-parse", "--show-toplevel")
$repoName = Split-Path $repoRoot -Leaf
$parentDir = Split-Path $repoRoot -Parent

Set-Location $repoRoot

$workstreams = @(
    @{
        Id = "W01"
        Branch = "codex/w01/gameplay-core"
        Path = Join-Path $parentDir "$repoName-w01-gameplay"
    }
    @{
        Id = "W02"
        Branch = "codex/w02/data-core"
        Path = Join-Path $parentDir "$repoName-w02-data"
    }
    @{
        Id = "W03"
        Branch = "codex/w03/diagnostics-ai"
        Path = Join-Path $parentDir "$repoName-w03-diagnostics"
    }
)

Write-Host "Repository root: $repoRoot"
Write-Host "Base ref: $BaseRef"
Write-Host ""

foreach ($workstream in $workstreams) {
    $branch = $workstream.Branch
    $path = $workstream.Path
    $id = $workstream.Id

    if (Test-Path $path) {
        Write-Host "[$id] Skip existing path: $path"
        continue
    }

    $branchExists = (git branch --list $branch | Out-String).Trim()

    if ($branchExists) {
        Write-Host "[$id] Creating worktree from existing branch $branch"
        git worktree add $path $branch
    }
    else {
        Write-Host "[$id] Creating branch $branch from $BaseRef"
        git worktree add -b $branch $path $BaseRef
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Failed to create worktree for $id."
    }
}

Write-Host ""
Write-Host "Done. Open Codex in these paths:"

foreach ($workstream in $workstreams) {
    Write-Host "- $($workstream.Id): $($workstream.Path)"
}
