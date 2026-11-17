<#
Owner-only reversible retail engine *probe* for independent, NON-FORWARDING
client.dll/server.dll. It does not produce a playable game build.

Requires an explicit disposable test-copy marker and SHA-pinned untouched
original retail DLLs, plus MSVC PE32 independently-built binaries.
Restores originals even if launch or test fails. This file never uploads
commercial game bytes or logs to GitHub.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$GameBin,
    [Parameter(Mandatory=$true)][string]$GameExecutable,
    [Parameter(Mandatory=$true)][string]$IndependentBin,
    [string]$GameArguments = '-console',
    [ValidateRange(10,300)][int]$TimeoutSeconds = 90,
    [switch]$DisposableTestCopyConfirmed
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$HashPins = @(
    @{ Name='server.dll'; Hash='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650' },
    @{ Name='client.dll'; Hash='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d' }
)
$MarkerFile = 'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY.txt'
$MarkerValue = 'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY_V1'

function Sha256([string]$Path) {
    # Windows PowerShell runner images may omit the Get-FileHash cmdlet.
    # This is the same .NET SHA-256 calculation as the existing owner stager.
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        return [System.BitConverter]::ToString($hasher.ComputeHash($stream)).Replace('-','').ToLowerInvariant()
    } finally {
        $stream.Dispose()
        $hasher.Dispose()
    }
}
function Require-Pe32([string]$Path) {
    $stream = [System.IO.File]::OpenRead($Path)
    $reader = New-Object System.IO.BinaryReader($stream)
    try {
        if ($reader.ReadUInt16() -ne 0x5A4D) { throw "Not an MZ image: $Path" }
        [void]$stream.Seek(0x3C,[System.IO.SeekOrigin]::Begin)
        $offset = $reader.ReadUInt32()
        if ($offset -lt 64 -or $offset -gt $stream.Length - 32) {
            throw "Invalid PE image offset: $Path"
        }
        [void]$stream.Seek($offset,[System.IO.SeekOrigin]::Begin)
        if ($reader.ReadUInt32() -ne 0x4550) { throw "Not PE: $Path" }
        if ($reader.ReadUInt16() -ne 0x014C) { throw "Not x86 PE32: $Path" }
        [void]$stream.Seek($offset+24,[System.IO.SeekOrigin]::Begin)
        if ($reader.ReadUInt16() -ne 0x010B) { throw "Not PE32 32-bit optional header: $Path" }
    } finally {
        $reader.Dispose()
        $stream.Dispose()
    }
}
function Require-ActualFile([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing file: $Path" }
    $item = Get-Item -LiteralPath $Path
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing symlink/reparse point: $Path"
    }
}
function Require-ActualDirectory([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "Missing directory: $Path"
    }
    $item = Get-Item -LiteralPath $Path
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing directory reparse point: $Path"
    }
}
# All user assertions and *all* binary preflight validations must pass
# BEFORE the first Move-Item, to keep expected failures non-destructive.
if (-not $DisposableTestCopyConfirmed) {
    throw "Refusing without -DisposableTestCopyConfirmed (NEVER test your live Steam install)."
}
Require-ActualDirectory $GameBin
Require-ActualDirectory $IndependentBin
Require-ActualFile $GameExecutable
$bin = (Resolve-Path -LiteralPath $GameBin).ProviderPath.TrimEnd('\')
$build = (Resolve-Path -LiteralPath $IndependentBin).ProviderPath.TrimEnd('\')
$exe = (Resolve-Path -LiteralPath $GameExecutable).ProviderPath
$root = [System.IO.Path]::GetFullPath((Join-Path $bin '..')).TrimEnd('\')
$prefix = $root + [System.IO.Path]::DirectorySeparatorChar
if ($bin -eq $build -or
    -not $exe.EndsWith('.exe',[System.StringComparison]::OrdinalIgnoreCase) -or
    -not $exe.StartsWith($prefix,[System.StringComparison]::OrdinalIgnoreCase) -or
    $root -match '(?i)[\\/]steamapps[\\/]common[\\/]') {
    throw "Expected a standalone disposable owner game-copy root outside steamapps/common"
}
$marker = Join-Path $root $MarkerFile
Require-ActualFile $marker
if ((Get-Content -LiteralPath $marker -Raw).Trim() -cne $MarkerValue) {
    throw "Explicit disposable-copy marker content mismatch: $MarkerFile"
}
$plan = @()
foreach ($spec in $HashPins) {
    $live = Join-Path $bin $spec.Name
    $candidate = Join-Path $build $spec.Name
    $saved = Join-Path $bin ("sin_independent_saved_" + $spec.Name)
    Require-ActualFile $live
    Require-ActualFile $candidate
    if (Test-Path -LiteralPath $saved) { throw "Pre-existing independent backup, refuse: $saved" }
    if (Test-Path -LiteralPath (Join-Path $bin ("sin_original_" + $spec.Name))) {
        throw "Original-forwarding proxy staging detected: $($spec.Name)"
    }
    if ((Sha256 $live) -cne $spec.Hash) { throw "Owner-original DLL SHA256 mismatch: $live" }
    Require-Pe32 $candidate
    $hash = Sha256 $candidate
    if ($hash -ceq $spec.Hash) { throw "Expected independent binary, got original: $candidate" }
    $plan += [pscustomobject]@{
        Live = $live
        Candidate = $candidate
        Saved = $saved
        OriginalHash = $spec.Hash
        CandidateHash = $hash
    }
}
$session = [Guid]::NewGuid().ToString('N')
$staged = New-Object System.Collections.Generic.List[object]
$child = $null
$childStillAlive = $false
try {
    foreach ($step in $plan) {
        Move-Item -LiteralPath $step.Live -Destination $step.Saved -ErrorAction Stop
        $staged.Add($step)
        Copy-Item -LiteralPath $step.Candidate -Destination $step.Live -ErrorAction Stop
        if ((Sha256 $step.Live) -cne $step.CandidateHash) {
            throw "Independent DLL copy integrity check failed"
        }
    }
    Write-Output "STAGED_INDEPENDENT: two standalone PE32 test DLLs; retail originals preserved as sibling backups"
    $previous = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_TRACE_FILE','Process')
    $previousSession = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_TRACE_SESSION','Process')
    try {
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_TRACE_FILE','1','Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_TRACE_SESSION',$session,'Process')
        Write-Output ("OWNER_INDEPENDENT_SESSION: {0}" -f $session)
        $options = @{
            FilePath = $exe
            WorkingDirectory = [System.IO.Path]::GetDirectoryName($exe)
            PassThru = $true
        }
        if (-not [string]::IsNullOrWhiteSpace($GameArguments)) {
            $options['ArgumentList'] = $GameArguments
        }
        $child = Start-Process @options
        Write-Output ("OWNER_ENGINE_PROCESS_ID: {0}" -f $child.Id)
        if (-not $child.WaitForExit($TimeoutSeconds*1000)) {
            Write-Warning "Timeout: terminating only the launched disposable game process tree"
            & taskkill.exe /PID $child.Id /T /F | Out-Null
            [void]$child.WaitForExit(15000)
            throw "Disposable game launch timed out"
        }
        Write-Output ("OWNER_ENGINE_EXIT_CODE: {0}" -f $child.ExitCode)
    } finally {
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_TRACE_FILE',$previous,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_TRACE_SESSION',$previousSession,'Process')
    }
} finally {
    # Do not replace DLLs from under a still-running game process.
    if ($null -ne $child) {
        try { $childStillAlive = -not $child.HasExited } catch { $childStillAlive = $true }
    }
    if ($childStillAlive) {
        Write-Warning "Game still running: originals deliberately remain backed up. Stop game and restore manually."
        Write-Warning "Backups: sin_independent_saved_client.dll and sin_independent_saved_server.dll"
    } else {
        foreach ($step in $staged) {
            if (Test-Path -LiteralPath $step.Saved -PathType Leaf) {
                if ((Sha256 $step.Saved) -cne $step.OriginalHash) {
                    Write-Warning "Original backup hash changed; retained for manual recovery: $($step.Saved)"
                    continue
                }
                if (Test-Path -LiteralPath $step.Live) {
                    Remove-Item -LiteralPath $step.Live -Force
                }
                Move-Item -LiteralPath $step.Saved -Destination $step.Live -ErrorAction Stop
                if ((Sha256 $step.Live) -cne $step.OriginalHash) {
                    throw "Original restoration verification failed: $($step.Live)"
                }
                Write-Output ("RESTORED_VERIFIED: {0}" -f $step.Live)
            }
        }
    }
    # Never attribute a previous or concurrent independent engine attempt
    # to this launch, even if it logged inside the same time window.
    $logs = @(Get-ChildItem -LiteralPath ([System.IO.Path]::GetTempPath()) -Filter ("sin-independent-{0}-*.log" -f $session) -File |
        Sort-Object LastWriteTimeUtc -Descending)
    foreach ($log in $logs) {
        Write-Output ("OWNER_INDEPENDENT_TRACE: {0}" -f $log.FullName)
        Get-Content -LiteralPath $log.FullName -Tail 24
    }
    # Produce a deterministic first-blocker summary from this launch's own
    # session logs only. No inference from synthetic tests or old log files.
    $verdictScript = Join-Path $PSScriptRoot 'Get-OwnerIndependentStartupVerdict.ps1'
    & $verdictScript -Session $session -LogDirectory ([System.IO.Path]::GetTempPath())
    if ($logs.Count -eq 0) {
        Write-Warning "No independent startup trace observed. The retail executable may not have loaded the game DLLs."
    }
}
