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
    [ValidateSet('Both','Server','Client')][string]$ProbeRole = 'Both',
    [switch]$EnableServerFactoryProbe,
    [switch]$EnableServerSoundProbe,
    [switch]$EnableServerEventProbe,
    [switch]$EnableServerCvarProbe,
    [switch]$EnableServerCvarBatchProbe,
    [switch]$EnableClientSoundProbe,
    [switch]$EnableClientMaterialProbe,
    [switch]$EnableClientScreenProbe,
    [switch]$EnableClientGaussianProbe,
    [switch]$EnableClientCvarProbe,
    [switch]$EnableClientOwnedStartupProbe,
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
# Walk the input paths lexically rather than following Resolve-Path alone.
# A junction or symbolic link in *any ancestor* can hide the live Steam game
# behind an apparently disposable directory name. Resolve-Path on the leaf
# and leaf-only attribute checks are not sufficient protection.
function Require-NoReparseAncestors([string]$Path) {
    $current = [System.IO.Path]::GetFullPath($Path)
    while ($true) {
        $item = Get-Item -LiteralPath $current -Force -ErrorAction Stop
        if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Refusing reparse point ancestor: $current"
        }
        $parent = [System.IO.Directory]::GetParent($current)
        if ($null -eq $parent) { break }
        $current = $parent.FullName
    }
}
# All user assertions and *all* binary preflight validations must pass
# BEFORE the first Move-Item, to keep expected failures non-destructive.
if (-not $DisposableTestCopyConfirmed) {
    throw "Refusing without -DisposableTestCopyConfirmed (NEVER test your live Steam install)."
}
if ($EnableClientOwnedStartupProbe -and -not $EnableClientCvarProbe) {
    throw 'EnableClientOwnedStartupProbe requires EnableClientCvarProbe and preceding Client-only gates'
}
if ($EnableClientCvarProbe -and -not $EnableClientGaussianProbe) {
    throw 'EnableClientCvarProbe requires EnableClientGaussianProbe and its Client-only prerequisites'
}
if ($EnableClientGaussianProbe -and -not $EnableClientScreenProbe) {
    throw 'EnableClientGaussianProbe requires EnableClientScreenProbe and its Client-only prerequisites'
}
if ($EnableClientScreenProbe -and -not $EnableClientMaterialProbe) {
    throw 'EnableClientScreenProbe requires EnableClientMaterialProbe and Client-only prerequisites'
}
if ($EnableClientMaterialProbe -and -not $EnableClientSoundProbe) {
    throw 'EnableClientMaterialProbe requires EnableClientSoundProbe and Client-only mode'
}
if ($EnableClientSoundProbe -and $ProbeRole -ne 'Client') {
    throw 'EnableClientSoundProbe requires explicit Client-only mode'
}
if ($EnableServerCvarBatchProbe -and -not $EnableServerCvarProbe) {
    throw 'EnableServerCvarBatchProbe requires EnableServerCvarProbe and preceding server gates'
}
if ($EnableServerCvarProbe -and -not $EnableServerEventProbe) {
    throw 'EnableServerCvarProbe requires EnableServerEventProbe (and its server-only prerequisites)'
}
if ($EnableServerEventProbe -and -not $EnableServerSoundProbe) {
    throw 'EnableServerEventProbe requires EnableServerSoundProbe (and its server-only prerequisites)'
}
if ($EnableServerSoundProbe -and ($ProbeRole -ne 'Server' -or -not $EnableServerFactoryProbe)) {
    throw 'EnableServerSoundProbe requires Server-only mode and EnableServerFactoryProbe'
}
if ($EnableServerFactoryProbe -and $ProbeRole -ne 'Server') {
    throw "EnableServerFactoryProbe requires the explicit Server-only diagnostic mode"
}
Require-ActualDirectory $GameBin
Require-ActualDirectory $IndependentBin
Require-ActualFile $GameExecutable
# Reject linked parents in all three locations *before* any staging.
Require-NoReparseAncestors $GameBin
Require-NoReparseAncestors $IndependentBin
Require-NoReparseAncestors $GameExecutable
$bin = (Resolve-Path -LiteralPath $GameBin).ProviderPath.TrimEnd('\')
$build = (Resolve-Path -LiteralPath $IndependentBin).ProviderPath.TrimEnd('\')
$exe = (Resolve-Path -LiteralPath $GameExecutable).ProviderPath
# Retail SiN splits the root engine bin/ from the game-specific
# SE1/bin/ containing the original client.dll and server.dll.
# Resolve the marker root from the actual game-content path, not always
# from the DLL directory's immediate parent.
$gameBinParent = [System.IO.Path]::GetFullPath((Join-Path $bin '..')).TrimEnd('\')
$isSe1Layout = [string]::Equals([System.IO.Path]::GetFileName($gameBinParent),
    'SE1',[System.StringComparison]::OrdinalIgnoreCase)
$root = $gameBinParent
if ($isSe1Layout) {
    $root = [System.IO.Path]::GetFullPath((Join-Path $gameBinParent '..')).TrimEnd('\')
}
$prefix = $root + [System.IO.Path]::DirectorySeparatorChar
if (-not [string]::Equals([System.IO.Path]::GetFileName($bin),'bin',
        [System.StringComparison]::OrdinalIgnoreCase) -or
    $bin -eq $build -or
    -not $exe.EndsWith('.exe',[System.StringComparison]::OrdinalIgnoreCase) -or
    -not $exe.StartsWith($prefix,[System.StringComparison]::OrdinalIgnoreCase) -or
    $root -match '(?i)[\\/]steamapps[\\/]common[\\/]') {
    throw "Expected a standalone disposable owner game-copy root outside steamapps/common"
}
if ($isSe1Layout) {
    if (-not [string]::Equals([System.IO.Path]::GetFileName($exe),
            'SinEpisodes.exe',[System.StringComparison]::OrdinalIgnoreCase) -or
        -not [string]::Equals([System.IO.Path]::GetDirectoryName($exe),$root,
            [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "SE1 game DLL layout requires SinEpisodes.exe in the disposable game root"
    }
    Require-ActualFile (Join-Path $gameBinParent 'GameInfo.txt')
}
$marker = Join-Path $root $MarkerFile
Require-ActualFile $marker
if ((Get-Content -LiteralPath $marker -Raw).Trim() -cne $MarkerValue) {
    throw "Explicit disposable-copy marker content mismatch: $MarkerFile"
}
# Authenticate both originals, even when staging only one. The retained
# original is not copied, moved or opened for writing.
$selectedNames = switch ($ProbeRole) {
    'Server' { 'server.dll' }
    'Client' { 'client.dll' }
    'Both' { 'server.dll'; 'client.dll' }
}
$plan = @()
$retained = @()
foreach ($spec in $HashPins) {
    $live = Join-Path $bin $spec.Name
    $candidate = Join-Path $build $spec.Name
    $saved = Join-Path $bin ("sin_independent_saved_" + $spec.Name)
    Require-ActualFile $live
    if (Test-Path -LiteralPath $saved) { throw "Pre-existing independent backup, refuse: $saved" }
    if (Test-Path -LiteralPath (Join-Path $bin ("sin_original_" + $spec.Name))) {
        throw "Original-forwarding proxy staging detected: $($spec.Name)"
    }
    if ((Sha256 $live) -cne $spec.Hash) { throw "Owner-original DLL SHA256 mismatch: $live" }
    if ($selectedNames -notcontains $spec.Name) {
        $retained += [pscustomobject]@{ Live=$live; OriginalHash=$spec.Hash }
        continue
    }
    Require-ActualFile $candidate
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
Write-Output ("OWNER_INDEPENDENT_PROBE_ROLE: {0}" -f $ProbeRole.ToUpperInvariant())
Write-Output ("OWNER_INDEPENDENT_PLANNED_DLLS: {0}" -f ($selectedNames -join ','))
foreach ($step in $retained) {
    Write-Output ("RETAINED_ORIGINAL_VERIFIED: {0}" -f $step.Live)
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
    Write-Output ("STAGED_INDEPENDENT: {0} standalone PE32 test DLL(s); selected retail originals preserved as sibling backups" -f $staged.Count)
    $previous = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_TRACE_FILE','Process')
    $previousSession = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_TRACE_SESSION','Process')
    $previousFactoryProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_SERVER_FACTORY_PROBE','Process')
    $previousSoundProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_SERVER_SOUND_PROBE','Process')
    $previousEventProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_SERVER_EVENT_PROBE','Process')
    $previousCvarProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_SERVER_CVAR_PROBE','Process')
    $previousBatchProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_SERVER_CVAR_BATCH_PROBE','Process')
    $previousClientSoundProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_SOUND_PROBE','Process')
    $previousClientMaterialProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_MATERIAL_PROBE','Process')
    $previousClientScreenProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_SCREEN_PROBE','Process')
    $previousClientGaussianProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_GAUSSIAN_PROBE','Process')
    $previousClientCvarProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_CVAR_PROBE','Process')
    $previousOwnedStartupProbe = [Environment]::GetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_OWNED_STARTUP_PROBE','Process')
    try {
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_TRACE_FILE','1','Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_TRACE_SESSION',$session,'Process')
        # Do not inherit an accidentally enabled negotiation probe.
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_FACTORY_PROBE',
            $(if ($EnableServerFactoryProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_SERVER_FACTORY_PROBE: {0}" -f $(if ($EnableServerFactoryProbe) { 'ENABLED_DIAGNOSTIC_ONLY' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_SOUND_PROBE',
            $(if ($EnableServerSoundProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_SERVER_SOUND_PROBE: {0}" -f $(if ($EnableServerSoundProbe) { 'ENABLED_TERMINAL_DIAGNOSTIC' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_EVENT_PROBE',
            $(if ($EnableServerEventProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_SERVER_EVENT_PROBE: {0}" -f $(if ($EnableServerEventProbe) { 'ENABLED_TERMINAL_DIAGNOSTIC' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_CVAR_PROBE',
            $(if ($EnableServerCvarProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_SERVER_CVAR_PROBE: {0}" -f $(if ($EnableServerCvarProbe) { 'ENABLED_TERMINAL_DIAGNOSTIC' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_CVAR_BATCH_PROBE',
            $(if ($EnableServerCvarBatchProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_SERVER_CVAR_BATCH_PROBE: {0}" -f $(if ($EnableServerCvarBatchProbe) { 'ENABLED_TERMINAL_DIAGNOSTIC' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_SOUND_PROBE',
            $(if ($EnableClientSoundProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_CLIENT_SOUND_PROBE: {0}" -f $(if ($EnableClientSoundProbe) { 'ENABLED_TERMINAL_DIAGNOSTIC' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_MATERIAL_PROBE',
            $(if ($EnableClientMaterialProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_CLIENT_MATERIAL_PROBE: {0}" -f $(if ($EnableClientMaterialProbe) { 'ENABLED_TERMINAL_DIAGNOSTIC' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_SCREEN_PROBE',
            $(if ($EnableClientScreenProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_CLIENT_SCREEN_PROBE: {0}" -f $(if ($EnableClientScreenProbe) { 'ENABLED_TERMINAL_DIAGNOSTIC' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_GAUSSIAN_PROBE',
            $(if ($EnableClientGaussianProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_CLIENT_GAUSSIAN_PROBE: {0}" -f $(if ($EnableClientGaussianProbe) { 'ENABLED_TERMINAL_DIAGNOSTIC' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_CVAR_PROBE',
            $(if ($EnableClientCvarProbe) { '1' } else { '0' }),'Process')
        Write-Output ("OWNER_CLIENT_CVAR_PROBE: {0}" -f $(if ($EnableClientCvarProbe) { 'ENABLED_TERMINAL_DIAGNOSTIC' } else { 'DISABLED' }))
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_OWNED_STARTUP_PROBE',$(if ($EnableClientOwnedStartupProbe) { '1' } else { '0' }),'Process')
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
        # Poll the actual owner-launched process for its loaded modules.
        # This is direct retail-engine boundary evidence, independent of
        # CreateInterface/Init logging. Module enumeration is best effort:
        # an early exit or access-denied enumeration is NOT evidence that
        # the engine did not load either DLL.
        $observedModules = @{}
        $moduleInspectionSucceeded = $false
        $moduleInspectionFailures = 0
        $moduleInspectionSamples = 0
        $moduleInspectionFirstError = $null
        $watch = [System.Diagnostics.Stopwatch]::StartNew()
        # Inspect immediately, including a process that has already exited.
        # The old HasExited loop guard skipped the first sample entirely for
        # fast-failing retail processes. A failed snapshot remains inconclusive.
        do {
            try {
                $child.Refresh()
                $moduleInspectionSamples++
                foreach ($module in $child.Modules) {
                    foreach ($step in $plan) {
                        if ([string]::Equals($module.FileName,$step.Live,
                                [System.StringComparison]::OrdinalIgnoreCase)) {
                            $observedModules[[System.IO.Path]::GetFileName($step.Live)] = $true
                        }
                    }
                }
                $moduleInspectionSucceeded = $true
            } catch {
                $moduleInspectionFailures++
                if ($null -eq $moduleInspectionFirstError) {
                    $moduleInspectionFirstError = $_.Exception.GetType().Name
                }
            }
            $child.Refresh()
            if ($child.HasExited) { break }
            if ($watch.Elapsed.TotalSeconds -ge $TimeoutSeconds) { break }
            # A short sampling interval reduces, but cannot eliminate, the
            # chance of missing transient DLL loads during early startup.
            if ($child.WaitForExit(50)) { break }
        } while ($true)
        $watch.Stop()
        if (-not $child.HasExited) {
            Write-Warning "Timeout: terminating only the launched disposable game process tree"
            & taskkill.exe /PID $child.Id /T /F | Out-Null
            [void]$child.WaitForExit(15000)
            throw "Disposable game launch timed out"
        }
        foreach ($step in $plan) {
            $name = [System.IO.Path]::GetFileName($step.Live)
            if ($observedModules.ContainsKey($name)) {
                Write-Output ("OWNER_ENGINE_MODULE_OBSERVED: {0}" -f $name)
            } else {
                Write-Output ("OWNER_ENGINE_MODULE_NOT_OBSERVED: {0}" -f $name)
            }
        }
        Write-Output ("OWNER_ENGINE_MODULE_INSPECTION: success={0} failures={1} samples={2} first_error={3}" -f
            $moduleInspectionSucceeded,$moduleInspectionFailures,$moduleInspectionSamples,
            $(if ($null -eq $moduleInspectionFirstError) { 'none' } else { $moduleInspectionFirstError }))
        Write-Output ("OWNER_ENGINE_EXIT_CODE: {0}" -f $child.ExitCode)
        # A process can terminate before a single module snapshot or trace.
        # Preserve the actual signed Win32 exit status, then decode common
        # NT loader failures without attributing them to our DLLs.
        $exitDiagnosis = Join-Path $PSScriptRoot 'Get-OwnerEngineExitDiagnosis.ps1'
        & $exitDiagnosis -ExitCode ([long]$child.ExitCode)
    } finally {
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_TRACE_FILE',$previous,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_TRACE_SESSION',$previousSession,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_FACTORY_PROBE',$previousFactoryProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_SOUND_PROBE',$previousSoundProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_EVENT_PROBE',$previousEventProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_CVAR_PROBE',$previousCvarProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_SERVER_CVAR_BATCH_PROBE',$previousBatchProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_SOUND_PROBE',$previousClientSoundProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_MATERIAL_PROBE',$previousClientMaterialProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_SCREEN_PROBE',$previousClientScreenProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_GAUSSIAN_PROBE',$previousClientGaussianProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_CVAR_PROBE',$previousClientCvarProbe,'Process')
        [Environment]::SetEnvironmentVariable('SIN_INDEPENDENT_CLIENT_OWNED_STARTUP_PROBE',$previousOwnedStartupProbe,'Process')
    }
} finally {
    # Do not replace DLLs from under a still-running game process.
    if ($null -ne $child) {
        try { $childStillAlive = -not $child.HasExited } catch { $childStillAlive = $true }
    }
    $restoreIncomplete = $childStillAlive
    if ($childStillAlive) {
        Write-Warning "Game still running: originals deliberately remain backed up. Stop game and restore manually."
        foreach ($step in $staged) {
            Write-Warning ("Backup: {0}" -f $step.Saved)
        }
    } else {
        foreach ($step in $staged) {
            # A retail launcher can exit before a separate engine process
            # unloads client.dll. Do not let its Windows sharing violation
            # abort evidence collection or prevent other DLL restorations.
            try {
                if (-not (Test-Path -LiteralPath $step.Saved -PathType Leaf)) {
                    throw "Saved original backup no longer exists"
                }
                if ((Sha256 $step.Saved) -cne $step.OriginalHash) {
                    throw "Saved original backup SHA256 mismatch"
                }
                if (Test-Path -LiteralPath $step.Live) {
                    Remove-Item -LiteralPath $step.Live -Force -ErrorAction Stop
                }
                Move-Item -LiteralPath $step.Saved -Destination $step.Live -ErrorAction Stop
                if ((Sha256 $step.Live) -cne $step.OriginalHash) {
                    throw "Restored original SHA256 verification failed"
                }
                Write-Output ("RESTORED_VERIFIED: {0}" -f $step.Live)
            } catch {
                $restoreIncomplete = $true
                # The SHA-verified backup must never be deleted in response
                # to a locked game DLL. Surface the error and continue to
                # the other role and session-specific diagnostic traces.
                Write-Warning ("RESTORE_PENDING: {0} backup={1} reason={2}" -f
                    $step.Live,$step.Saved,$_.Exception.GetType().Name)
            }
        }
    }
    # Also check the retained role has not changed. Never overwrite it:
    # a discrepancy must be surfaced, including failures to read the file.
    foreach ($step in $retained) {
        try {
            if ((Sha256 $step.Live) -cne $step.OriginalHash) {
                throw "Retained original SHA256 mismatch"
            }
            Write-Output ("RETAINED_ORIGINAL_UNCHANGED: {0}" -f $step.Live)
        } catch {
            $restoreIncomplete = $true
            Write-Warning ("RETAINED_ORIGINAL_CHECK_FAILED: {0} reason={1}" -f $step.Live,$_.Exception.GetType().Name)
        }
    }
    Write-Output ("OWNER_INDEPENDENT_RESTORATION: {0}" -f $(if ($restoreIncomplete) { 'INCOMPLETE_MANUAL_RECOVERY_REQUIRED' } else { 'VERIFIED_COMPLETE' }))
    # Never attribute a previous or concurrent independent engine attempt
    # to this launch, even if it logged inside the same time window.
    $logs = @(Get-ChildItem -LiteralPath ([System.IO.Path]::GetTempPath()) -Filter ("sin-independent-{0}-*.log" -f $session) -File |
        Sort-Object LastWriteTimeUtc -Descending)
    foreach ($log in $logs) {
        Write-Output ("OWNER_INDEPENDENT_TRACE: {0}" -f $log.FullName)
        # The full 16-query server preflight is >24 lines. Do not
        # silently lose its initial factory/Init evidence. Bound console
        # output while the classifier below consumes the complete log.
        $lines = @(Get-Content -LiteralPath $log.FullName -TotalCount 1001)
        if ($lines.Count -gt 1000) {
            $lines | Select-Object -First 1000
            Write-Warning "Trace console output capped at 1000 lines; classifier reads entire session."
        } else {
            $lines
        }
    }
    # Produce a deterministic first-blocker summary from this launch's own
    # session logs only. No inference from synthetic tests or old log files.
    $verdictScript = Join-Path $PSScriptRoot 'Get-OwnerIndependentStartupVerdict.ps1'
    & $verdictScript -Session $session -LogDirectory ([System.IO.Path]::GetTempPath())
    if ($logs.Count -eq 0) {
        Write-Warning "No independent startup trace observed. The retail executable may not have loaded the game DLLs."
    }
}
