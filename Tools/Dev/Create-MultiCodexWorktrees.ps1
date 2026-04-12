[CmdletBinding()]
param(
    [string]$BaseRef = "main",
    [ValidateSet("FirstWave", "RuntimeLayer", "FullW01W11")]
    [string]$Preset = "FirstWave"
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

$catalog = @(
    @{
        Id = "W01"
        Branch = "codex/w01/gameplay-core"
        Suffix = "w01-gameplay"
    }
    @{
        Id = "W02"
        Branch = "codex/w02/data-core"
        Suffix = "w02-data"
    }
    @{
        Id = "W03"
        Branch = "codex/w03/diagnostics-ai"
        Suffix = "w03-diagnostics"
    }
    @{
        Id = "W04"
        Branch = "codex/w04/scripting-host"
        Suffix = "w04-scripting"
    }
    @{
        Id = "W05"
        Branch = "codex/w05/scene-ecs"
        Suffix = "w05-scene"
    }
    @{
        Id = "W06"
        Branch = "codex/w06/asset-pipeline"
        Suffix = "w06-assets"
    }
    @{
        Id = "W07"
        Branch = "codex/w07/platform-input"
        Suffix = "w07-platform"
    }
    @{
        Id = "W08"
        Branch = "codex/w08/renderer2d"
        Suffix = "w08-renderer"
    }
    @{
        Id = "W09"
        Branch = "codex/w09/physics2d"
        Suffix = "w09-physics"
    }
    @{
        Id = "W10"
        Branch = "codex/w10/audio-runtime"
        Suffix = "w10-audio"
    }
    @{
        Id = "W11"
        Branch = "codex/w11/ui-debug"
        Suffix = "w11-ui-debug"
    }
)

$selectedIds = switch ($Preset) {
    "FirstWave" {
        @("W01", "W02", "W03")
    }
    "RuntimeLayer" {
        @("W05", "W06", "W07", "W08", "W09", "W10", "W11")
    }
    "FullW01W11" {
        @("W01", "W02", "W03", "W04", "W05", "W06", "W07", "W08", "W09", "W10", "W11")
    }
}

$workstreams = foreach ($item in $catalog) {
    if ($selectedIds -contains $item.Id) {
        @{
            Id = $item.Id
            Branch = $item.Branch
            Path = Join-Path $parentDir "$repoName-$($item.Suffix)"
        }
    }
}

Write-Host "Repository root: $repoRoot"
Write-Host "Base ref: $BaseRef"
Write-Host "Preset: $Preset"
Write-Host ""

foreach ($workstream in $workstreams) {
    $branch = $workstream.Branch
    $path = $workstream.Path
    $id = $workstream.Id

    if (Test-Path $path) {
        Write-Host "[$id] Skip existing path: $path"
        continue
    }

    $branchExists = (& git branch --list $branch | Out-String).Trim()

    if ($branchExists) {
        Write-Host "[$id] Creating worktree from existing branch $branch"
        & git worktree add $path $branch
    }
    else {
        Write-Host "[$id] Creating branch $branch from $BaseRef"
        & git worktree add -b $branch $path $BaseRef
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
