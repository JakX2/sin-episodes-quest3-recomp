#requires -Version 5.1
<#
 Phase627: OWNER-ONLY, offline/disposable SiN Episodes original Elite callback
 evidence. No live engine interfaces, no original .text edits, no DLL patches.

 This script is NOT run in CI. It validates PE32 scanner and original DLL
 SHA256 before launching the owner's disposable SiN game with the debugger.
 Only the helper's --self-test (no game) is run by Windows CI.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameExecutable,
    [Parameter(Mandatory = $true)][string]$GameBin,
    [Parameter(Mandatory = $true)][string]$TraceExecutable,
    [Parameter(Mandatory = $true)][string]$OutputTrace,
    [Parameter(Mandatory = $true)][switch]$DisposableTestCopyConfirmed,
    [ValidateRange(5,900)][int]$TimeoutSeconds = 120,
    [ValidateSet('Elite','Weapon','Event')][string]$WatchProfile = 'Elite',
    [string[]]$GameArguments = @()
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-RealFile([string]$Path, [string]$Role) {
    $item = Get-Item -LiteralPath $Path -ErrorAction Stop
    if ($item.PSIsContainer) { throw "$Role is a directory, not a file." }
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "$Role must be a real file, not a junction or symlink."
    }
    return $item
}
function Get-Pe32([string]$Path) {
    $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read,
                              [IO.FileShare]::ReadWrite)
    try {
        if ($stream.Length -lt 256) { return $false }
        $reader = New-Object IO.BinaryReader($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) { return $false }
        [void]$stream.Seek(0x3C, [IO.SeekOrigin]::Begin)
        $offset = $reader.ReadInt32()
        if ($offset -lt 64 -or ($offset + 26) -gt $stream.Length) {
            return $false
        }
        [void]$stream.Seek($offset, [IO.SeekOrigin]::Begin)
        if ($reader.ReadUInt32() -ne 0x00004550) { return $false }
        # COFF Machine=0x14c (IMAGE_FILE_MACHINE_I386).
        if ($reader.ReadUInt16() -ne 0x014C) { return $false }
        [void]$stream.Seek($offset + 24, [IO.SeekOrigin]::Begin)
        return $reader.ReadUInt16() -eq 0x10B
    }
    finally { $stream.Dispose() }
}
if (-not $DisposableTestCopyConfirmed.IsPresent) {
    throw 'Owner must explicitly confirm this is a disposable, offline game copy.'
}
$game = Get-RealFile $GameExecutable 'Game executable'
$tracer = Get-RealFile $TraceExecutable 'Win32 x86 tracing executable'
$bin = Get-Item -LiteralPath $GameBin -ErrorAction Stop
if (-not $bin.PSIsContainer -or
    ($bin.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw 'GameBin must be a physical directory, not a reparse point.'
}
$originalPath = Join-Path $bin.FullName 'sin_original_server.dll'
if (-not (Test-Path -LiteralPath $originalPath -PathType Leaf)) {
    $originalPath = Join-Path $bin.FullName 'server.dll'
}
$original = Get-RealFile $originalPath 'Original SiN server DLL'
$originalHash = (Get-FileHash -LiteralPath $original.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
$expected = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
if ($originalHash -ne $expected -or $original.Length -ne 6049792) {
    throw 'GameBin does not contain the exact original 2006 SiN Episodes server.dll.'
}
if (-not (Get-Pe32 $game.FullName)) { throw 'Game executable is not Windows PE32 x86.' }
if (-not (Get-Pe32 $tracer.FullName)) { throw 'Debugger is not Windows PE32 x86.' }
if (-not (Get-Pe32 $original.FullName)) { throw 'Original server DLL is not PE32 x86.' }
$gameDir = $game.Directory.FullName
$binExpected = Join-Path $gameDir 'SE1\bin'
# Do not require identical path spelling or case; Windows Resolve-Path
# normalizes the expected location where the Source engine loads it.
if (-not (Test-Path -LiteralPath $binExpected -PathType Container)) {
    throw 'The disposable game executable does not have its SE1\bin directory.'
}
if ((Resolve-Path -LiteralPath $binExpected).Path -ine $bin.FullName) {
    throw 'GameBin is not this disposable game executable’s SE1\bin.'
}
$out = [IO.Path]::GetFullPath($OutputTrace)
if ([IO.File]::Exists($out)) {
    throw 'OutputTrace already exists. Use a fresh name to prevent stale evidence.'
}
$outDir = [IO.Path]::GetDirectoryName($out)
if (-not [IO.Directory]::Exists($outDir)) {
    throw "OutputTrace directory does not exist: $outDir"
}
Write-Host 'OWNER_ELITE_TRACE: ORIGINAL_DLL_SHA256_AUTHENTICATED'
Write-Host "OWNER_ELITE_TRACE: REQUESTED_PROFILE=$WatchProfile"
Write-Host 'OWNER_ELITE_TRACE: No original code or runtime NPC data will be modified.'
$command = @('--disposable-confirmed',
             '--game', $game.FullName,
             '--cwd', $gameDir,
             '--log', $out,
             '--timeout-ms', [string]($TimeoutSeconds * 1000),
             '--watch-profile', $WatchProfile.ToLowerInvariant())
if ($GameArguments.Count -gt 0) { $command += '--'; $command += $GameArguments }
& $tracer.FullName @command
$exitCode = $LASTEXITCODE
if ($exitCode -ne 0) {
    Write-Host "OWNER_ELITE_TRACE: TRACE_TOOL_NONZERO_EXIT=$exitCode"
}
if (-not [IO.File]::Exists($out)) {
    throw 'The trace executable did not create a trace file.'
}
$events = @(Get-Content -LiteralPath $out | Where-Object { $_.Trim() } |
  ForEach-Object { $_ | ConvertFrom-Json -ErrorAction Stop })
$starts = @($events | Where-Object { $_.phase -eq 'trace_process_started' })
$finished = @($events | Where-Object { $_.phase -eq 'trace_finished' })
$singlePid = $starts.Count -eq 1 -and $finished.Count -eq 1
if ($singlePid) {
    $expectedPid = [int64]$starts[0].pid
    if ($expectedPid -le 0 -or
        @($events | Where-Object { [int64]$_.pid -ne $expectedPid }).Count -gt 0) {
        $singlePid = $false
    }
}
$authenticated = @($events | Where-Object {
  $_.phase -eq 'retail_server_sha256_verified'
}).Count -eq 1
$armed = @($events | Where-Object {
  $_.phase -eq 'elite_hardware_watch_armed'
}).Count -eq 1
$profileTag = switch ($WatchProfile) {
  'Elite' { 'watch_profile_elite' }
  'Weapon' { 'watch_profile_weapon' }
  'Event' { 'watch_profile_event' }
}
$allProfileTags = @('watch_profile_elite','watch_profile_weapon',
                    'watch_profile_event')
$profileVerified = @($events | Where-Object {
  $_.phase -eq $profileTag
}).Count -eq 1
if (@($events | Where-Object {
  $_.phase -in $allProfileTags -and $_.phase -ne $profileTag
}).Count -gt 0) {
    $profileVerified = $false
}
$incomplete = @($events | Where-Object {
  $_.phase -in @('elite_thread_watch_failed','watch_completeness_unproven',
                'continue_debug_failed','watch_disarm_failed_detach_unsafe',
                'elite_watch_partial_rollback_failed',
                'elite_watch_arm_rejected_clean',
                'elite_original_unload_disarm_failed',
                'unsafe_watch_disposable_process_terminated',
                'unsafe_watch_process_termination_failed',
                'debugger_detach_failed',
                'original_live_code_prefix_mismatch')
}).Count -gt 0
$spawns = @($events | Where-Object {
  $_.phase -eq 'elite_spawn' -and [int]$_.detail -gt 0 -and
  [int]$_.tid -gt 0
}).Count
$animation = @($events | Where-Object {
  $_.phase -eq 'elite_animation_event' -and [int]$_.detail -gt 0 -and
  [int]$_.tid -gt 0
}).Count
$ticks = @($events | Where-Object {
  $_.phase -eq 'elite_virtual_tick' -and [int]$_.detail -gt 0 -and
  [int]$_.tid -gt 0
}).Count
$gates = @($events | Where-Object {
  $_.phase -eq 'weapon_gate43' -and [int]$_.detail -gt 0 -and
  [int]$_.tid -gt 0
}).Count
$updates = @($events | Where-Object {
  $_.phase -eq 'weapon_state_update' -and [int]$_.detail -gt 0 -and
  [int]$_.tid -gt 0
}).Count
$secondary = @($events | Where-Object {
  $_.phase -eq 'elite_secondary_event' -and [int]$_.detail -gt 0 -and
  [int]$_.tid -gt 0
})
$secondary43 = @($secondary | Where-Object {
  [int]$_.event_id_readable -eq 1 -and [int64]$_.event_id -eq 0x43
}).Count
$secondary47 = @($secondary | Where-Object {
  [int]$_.event_id_readable -eq 1 -and [int64]$_.event_id -eq 0x47
}).Count
$secondaryUnknown = @($secondary | Where-Object {
  [int]$_.event_id_readable -ne 1
}).Count
$firstEvent43 = @($events | Where-Object {
  $_.phase -eq 'elite_animation_event' -and
  [int]$_.event_id_readable -eq 1 -and [int64]$_.event_id -eq 0x43
}).Count
$firstEvent47 = @($events | Where-Object {
  $_.phase -eq 'elite_animation_event' -and
  [int]$_.event_id_readable -eq 1 -and [int64]$_.event_id -eq 0x47
}).Count
if ($exitCode -eq 0 -and $singlePid -and $authenticated -and $armed -and
    $profileVerified -and -not $incomplete) {
    if ($spawns -gt 0) {
        Write-Host "OWNER_ELITE_TRACE: ORIGINAL_ENGINE_SPAWN_ENTRY_OBSERVED=$spawns"
    } else {
        Write-Host 'OWNER_ELITE_TRACE: ORIGINAL_ENGINE_SPAWN_ENTRY_NOT_OBSERVED'
    }
    if ($WatchProfile -eq 'Weapon') {
        if ($ticks -gt 0) {
            Write-Host "OWNER_ELITE_TRACE: ORIGINAL_ENGINE_ELITE_TICK_OBSERVED=$ticks"
        } else {
            Write-Host 'OWNER_ELITE_TRACE: ORIGINAL_ENGINE_ELITE_TICK_NOT_OBSERVED'
        }
        if ($gates -gt 0) {
            Write-Host "OWNER_ELITE_TRACE: ORIGINAL_WEAPON_GATE_ENTRY_OBSERVED=$gates"
        } else {
            Write-Host 'OWNER_ELITE_TRACE: ORIGINAL_WEAPON_GATE_ENTRY_NOT_OBSERVED'
        }
        if ($updates -gt 0) {
            Write-Host "OWNER_ELITE_TRACE: ORIGINAL_WEAPON_TIMED_UPDATE_OBSERVED=$updates"
        } else {
            Write-Host 'OWNER_ELITE_TRACE: ORIGINAL_WEAPON_TIMED_UPDATE_NOT_OBSERVED'
        }
    } elseif ($WatchProfile -eq 'Event') {
        if ($secondary.Count -gt 0) {
            Write-Host "OWNER_ELITE_TRACE: ORIGINAL_SECONDARY_EVENT_ENTRY_OBSERVED=$($secondary.Count)"
        } else {
            Write-Host 'OWNER_ELITE_TRACE: ORIGINAL_SECONDARY_EVENT_ENTRY_NOT_OBSERVED'
        }
        Write-Host "OWNER_ELITE_TRACE: ORIGINAL_SECONDARY_EVENT43_CLASSIFIED=$secondary43"
        Write-Host "OWNER_ELITE_TRACE: ORIGINAL_SECONDARY_EVENT47_CLASSIFIED=$secondary47"
        Write-Host "OWNER_ELITE_TRACE: ORIGINAL_SECONDARY_EVENT_ID_UNREADABLE=$secondaryUnknown"
        Write-Host "OWNER_ELITE_TRACE: ORIGINAL_FIRST_EVENT43_CLASSIFIED=$firstEvent43"
        Write-Host "OWNER_ELITE_TRACE: ORIGINAL_FIRST_EVENT47_CLASSIFIED=$firstEvent47"
        if ($gates -gt 0) {
            Write-Host "OWNER_ELITE_TRACE: ORIGINAL_WEAPON_GATE_ENTRY_OBSERVED=$gates"
        } else {
            Write-Host 'OWNER_ELITE_TRACE: ORIGINAL_WEAPON_GATE_ENTRY_NOT_OBSERVED'
        }
    } else {
        if ($animation -gt 0) {
            Write-Host "OWNER_ELITE_TRACE: ORIGINAL_ENGINE_EVENT_ENTRY_OBSERVED=$animation"
        } else {
            Write-Host 'OWNER_ELITE_TRACE: ORIGINAL_ENGINE_EVENT_ENTRY_NOT_OBSERVED'
        }
    }
} else {
    Write-Host 'OWNER_ELITE_TRACE: ORIGINAL_ENGINE_CALLBACK_EVIDENCE_NOT_PROVEN'
}
Write-Host "OWNER_ELITE_TRACE: TRACE_FILE=$out"
