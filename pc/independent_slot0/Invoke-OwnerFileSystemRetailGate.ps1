<#
Owner-only, two-step READ-ONLY 2006 SiN engine filesystem milestone.
Always delegates to the SHA-authenticated reversible owner launcher.
Presence is the mandatory first real retail test. The second read test
requires a saved, session-matched and restored presence transcript.
Public CI may use -PlanOnly; that never touches or launches the game.
THIS IS TERMINAL DIAGNOSTIC, NEVER GAME BOOT PROOF.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$DisposableGameRoot,
    [Parameter(Mandatory=$true)][string]$IndependentBin,
    [ValidateSet('Presence','Read','Staged','Hooks','Soundscape')][string]$Phase='Presence',
    [switch]$ReadOnlySoundscapeCvarLookup,
    [switch]$ReadOnlyPhysicsFactoryProbe,
    [switch]$ReadOnlySurfacePropsReadProbe,
    [string]$PriorPresenceTranscript,
    [string]$PriorReadTranscript,
    [string]$TranscriptPath,
    [ValidateRange(10,300)][int]$TimeoutSeconds=90,
    [switch]$DisposableTestCopyConfirmed,
    [switch]$PlanOnly
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if ($ReadOnlySoundscapeCvarLookup -and $Phase -ne 'Soundscape') {
    throw 'ReadOnlySoundscapeCvarLookup requires Soundscape phase and previous read/hook proof'
}
if ($ReadOnlyPhysicsFactoryProbe -and
    ($Phase -ne 'Soundscape' -or -not $ReadOnlySoundscapeCvarLookup)) {
    throw 'ReadOnlyPhysicsFactoryProbe requires Soundscape phase and ReadOnlySoundscapeCvarLookup'
}
if ($ReadOnlySurfacePropsReadProbe -and
    ($Phase -ne 'Soundscape' -or -not $ReadOnlyPhysicsFactoryProbe)) {
    throw 'ReadOnlySurfacePropsReadProbe requires Soundscape phase and ReadOnlyPhysicsFactoryProbe'
}

function Full-Absolute([string]$Path,[string]$Label) {
    if ([string]::IsNullOrWhiteSpace($Path) -or
        -not [System.IO.Path]::IsPathRooted($Path)) {
        throw "$Label must be an absolute path"
    }
    return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
}
function ExactLine([string[]]$Lines,[string]$Value) {
    return @($Lines | Where-Object { $_ -ceq $Value }).Count -eq 1
}
function Check-PresenceEvidence([string]$Path) {
    $full=Full-Absolute $Path 'PriorPresenceTranscript'
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        throw 'Read phase requires an existing prior presence transcript'
    }
    $item=Get-Item -LiteralPath $full
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Refusing symlink/reparse prior presence transcript'
    }
    $lines=[System.IO.File]::ReadAllLines($full)
    $sessions=@($lines | Where-Object {
        $_ -cmatch '^OWNER_INDEPENDENT_SESSION: [0-9a-f]{32}$'
    })
    if ($sessions.Count -ne 1) {
        throw 'Presence evidence missing a single exact session ID'
    }
    $sessionMatch=[regex]::Match($sessions[0],
        '^OWNER_INDEPENDENT_SESSION: ([0-9a-f]{32})$')
    if (-not $sessionMatch.Success -or $sessionMatch.Groups.Count -ne 2) {
        throw 'Presence evidence missing a single exact session ID'
    }
    $session=$sessionMatch.Groups[1].Value
    if (-not (ExactLine $lines 'OWNER_INDEPENDENT_PROBE_ROLE: CLIENT') -or
        -not (ExactLine $lines 'OWNER_CLIENT_ENGINE_FS_PROBE: ENABLED_TERMINAL_DIAGNOSTIC') -or
        -not (ExactLine $lines 'OWNER_CLIENT_ENGINE_FS_READ_PROBE: DISABLED') -or
        -not (ExactLine $lines 'OWNER_ENGINE_EXIT_HEX: 0x53494E68') -or
        -not (ExactLine $lines 'OWNER_ENGINE_EXIT_CODE: 1397313128') -or
        -not (ExactLine $lines 'OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE') -or
        -not (ExactLine $lines 'OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_PRESENCE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF')) {
        throw 'Presence evidence lacks exact exit, verdict or restoration proof'
    }
    $line=@($lines | Where-Object {
        $_ -cmatch '^OWNER_CLIENT_ENGINE_FS_PRESENCE_RESULTS: '
    })
    if ($line.Count -ne 1 -or
        $line[0] -cnotmatch ('^OWNER_CLIENT_ENGINE_FS_PRESENCE_RESULTS: sin-independent-' +
                $session + '-[0-9]+\.log:client:both-present$')) {
        throw 'Presence evidence lacks a same-session both-present trace'
    }
    return $session
}

function Check-ReadEvidence([string]$Path) {
    # Require a fully restored previous retail READ session before staged
    # original-mode resource probing. Transcript text remains owner-local
    # workflow gating rather than cryptographic proof.
    $full=Full-Absolute $Path 'PriorReadTranscript'
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) {
        throw 'Staged phase requires an existing previous read transcript'
    }
    $item=Get-Item -LiteralPath $full
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Refusing linked previous read transcript'
    }
    $lines=[IO.File]::ReadAllLines($full)
    $sessions=@($lines | Where-Object {
        $_ -cmatch '^OWNER_INDEPENDENT_SESSION: [0-9a-f]{32}$'
    })
    if ($sessions.Count -ne 1) {
        throw 'Read evidence requires one exact session ID'
    }
    $match=[regex]::Match($sessions[0],
        '^OWNER_INDEPENDENT_SESSION: ([0-9a-f]{32})$')
    if (-not $match.Success) {throw 'Read evidence requires one exact session ID'}
    $session=$match.Groups[1].Value
    foreach($required in @(
        'OWNER_INDEPENDENT_PROBE_ROLE: CLIENT'
        'OWNER_CLIENT_ENGINE_FS_PROBE: ENABLED_TERMINAL_DIAGNOSTIC'
        'OWNER_CLIENT_ENGINE_FS_READ_PROBE: ENABLED_TERMINAL_DIAGNOSTIC'
        'OWNER_CLIENT_ENGINE_FS_STAGED_PROBE: DISABLED'
        'OWNER_ENGINE_EXIT_HEX: 0x53494E6B'
        'OWNER_ENGINE_EXIT_CODE: 1397313131'
        'OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE'
        'OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_READ_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    )) {
        if (-not (ExactLine $lines $required)) {
            throw ('Read evidence lacks exact stage/exit/restoration: '+$required)
        }
    }
    $results=@($lines | Where-Object {
        $_ -cmatch '^OWNER_CLIENT_ENGINE_FS_READ_RESULTS: '
    })
    $exact=('^OWNER_CLIENT_ENGINE_FS_READ_RESULTS: sin-independent-'+
        $session+'-[0-9]+\.log:client:both-present:parsed-and-verified:62:8:156:66$')
    if ($results.Count -ne 1 -or $results[0] -cnotmatch $exact) {
        throw 'Read evidence lacks session-bound validated count trace'
    }
    return $session
}

$root=Full-Absolute $DisposableGameRoot 'DisposableGameRoot'
$independent=Full-Absolute $IndependentBin 'IndependentBin'
$sep=[System.IO.Path]::DirectorySeparatorChar
if ($root -eq [System.IO.Path]::GetPathRoot($root).TrimEnd('\') -or
    $root -match '(?i)[\\/]steamapps[\\/]common([\\/]|$)') {
    throw 'Refusing a drive root or live Steam installation'
}
if ($Phase -eq 'Read') {
    if ([string]::IsNullOrWhiteSpace($PriorPresenceTranscript)) {
        throw 'Read phase requires a previous verified owner presence transcript'
    }
    $previousSession=Check-PresenceEvidence $PriorPresenceTranscript
} elseif (-not [string]::IsNullOrWhiteSpace($PriorPresenceTranscript)) {
    throw 'PriorPresenceTranscript is valid only for Read phase'
}
if ($Phase -in @('Staged','Hooks','Soundscape')) {
    if ([string]::IsNullOrWhiteSpace($PriorReadTranscript)) {
        throw 'Staged phase requires a previous verified owner read transcript'
    }
    $previousReadSession=Check-ReadEvidence $PriorReadTranscript
} elseif (-not [string]::IsNullOrWhiteSpace($PriorReadTranscript)) {
    throw 'PriorReadTranscript is valid only for Staged, Hooks or Soundscape phase'
}
if (-not $PlanOnly -and -not $DisposableTestCopyConfirmed) {
    throw 'Real owner launch requires -DisposableTestCopyConfirmed'
}
if (-not $PlanOnly) {
    if ([string]::IsNullOrWhiteSpace($TranscriptPath)) {
        throw 'Real owner launch requires a new absolute -TranscriptPath'
    }
    $out=Full-Absolute $TranscriptPath 'TranscriptPath'
    if ($out.StartsWith(($root+$sep),
             [System.StringComparison]::OrdinalIgnoreCase) -or
        [string]::Equals($out,$root,[System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'Refusing transcript output inside disposable game files'
    }
    if (Test-Path -LiteralPath $out) {
        throw 'Refusing to overwrite an existing owner evidence transcript'
    }
    $parent=[System.IO.Path]::GetDirectoryName($out)
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        throw 'Transcript parent directory does not exist'
    }
    $item=Get-Item -LiteralPath $parent
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Refusing linked transcript parent directory'
    }
}
$launcher=Join-Path $PSScriptRoot 'Invoke-OwnerIndependentEngineProbe.ps1'
$settings=@{
    GameBin=(Join-Path $root 'SE1\bin')
    GameExecutable=(Join-Path $root 'SinEpisodes.exe')
    IndependentBin=$independent
    ProbeRole='Client'
    DisposableTestCopyConfirmed=$true
    TimeoutSeconds=$TimeoutSeconds
    EnableClientSoundProbe=$true
    EnableClientMaterialProbe=$true
    EnableClientScreenProbe=$true
    EnableClientGaussianProbe=$true
    EnableClientCvarProbe=$true
    EnableClientOwnedStartupProbe=$true
    EnableClientVGuiProbe=$true
    EnableClientVGuiLayoutProbe=$true
    EnableClientEngineVGuiSlot1Probe=$true
    EnableClientVGuiControlsBootstrapProbe=$true
    EnableClientMatSurfaceProbe=$true
    EnableClientSoundSystemInitProbe=$true
    EnableClientEngineFsProbe=$true
}
if ($Phase -in @('Read','Staged','Hooks','Soundscape')) {
    $settings.EnableClientEngineFsReadProbe=$true
}
if ($Phase -in @('Staged','Hooks','Soundscape')) {
    $settings.EnableClientEngineFsStagedProbe=$true
}
if ($Phase -in @('Hooks','Soundscape')) {
    $settings.EnableClientEngineFsHookProbe=$true
}
if ($Phase -eq 'Soundscape') {
    $settings.EnableClientSoundscapeCatalogProbe=$true
    if ($ReadOnlySoundscapeCvarLookup) {
        $settings.EnableClientSoundscapeCvarLookupProbe=$true
    }
    if ($ReadOnlyPhysicsFactoryProbe) {
        $settings.EnableClientPhysicsFactoryProbe=$true
    }
    if ($ReadOnlySurfacePropsReadProbe) {
        $settings.EnableClientSurfacePropsReadProbe=$true
    }
}
if ($PlanOnly) {
    Write-Output 'OWNER_ENGINE_FS_GATE: PLAN_ONLY_NO_GAME_LAUNCHED'
    Write-Output ("OWNER_ENGINE_FS_GATE_PHASE: {0}" -f $Phase)
    Write-Output ("OWNER_ENGINE_FS_GATE_ORIGINAL_LAUNCHER: {0}" -f $launcher)
    Write-Output ("OWNER_ENGINE_FS_GATE_ARGUMENT_NAMES: {0}" -f
        ((@($settings.Keys) | Sort-Object) -join ','))
    if ($Phase -eq 'Read') {
        Write-Output ("OWNER_ENGINE_FS_GATE_PREVIOUS_PRESENCE_SESSION: {0}" -f $previousSession)
    }
    if ($Phase -in @('Staged','Hooks','Soundscape')) {
        Write-Output ("OWNER_ENGINE_FS_GATE_PREVIOUS_READ_SESSION: {0}" -f $previousReadSession)
    }
    return
}
# The existing launcher alone validates the owner-original SHA256 hashes,
# both PE32 DLLs, content layout, explicit marker, linked ancestors and
# non-forwarding independent DLL, then restores the originals in finally.
Write-Output ("OWNER_ENGINE_FS_GATE_PHASE: {0}" -f $Phase)
Write-Output ("OWNER_ENGINE_FS_GATE_TRANSCRIPT: {0}" -f $out)
try {
    & $launcher @settings *>&1 | Tee-Object -FilePath $out | Out-Host
} catch {
    # Keep the live transcript and the underlying launcher's restore report.
    throw ("Owner filesystem diagnostic failed; inspect saved transcript: " +
           $out + " ; reason: " + $_.Exception.Message)
}
$report=[System.IO.File]::ReadAllLines($out)
if (-not (ExactLine $report 'OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE')) {
    throw 'Owner restoration not verified: do not repeat or re-stage until recovered'
}
$expected=if($Phase -eq 'Presence'){'0x53494E68'}elseif($Phase -eq 'Read'){'0x53494E6B'}elseif($Phase -eq 'Staged'){'0x53494E70'}elseif($Phase -eq 'Hooks'){'0x53494E74'}else{'0x53494E76'}
$verdict=if($Phase -eq 'Presence'){
    'OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_PRESENCE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
} elseif ($Phase -eq 'Read') {
    'OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_READ_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
} elseif ($Phase -eq 'Staged') {
    'OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_STAGED_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
} elseif ($Phase -eq 'Hooks') {
    'OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_HOOKS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
} else {
    'OWNER_INDEPENDENT_VERDICT: CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
}
if (-not (ExactLine $report ("OWNER_ENGINE_EXIT_HEX: "+$expected)) -or
    -not (ExactLine $report $verdict)) {
    Write-Output 'OWNER_ENGINE_FS_GATE_RESULT: EVIDENCE_INSUFFICIENT_NO_BOOT_PROOF'
    throw 'Expected same-session filesystem diagnostic result was not confirmed'
}
if ($Phase -eq 'Presence') {
    [void](Check-PresenceEvidence $out)
} elseif ($Phase -eq 'Read') {
    [void](Check-ReadEvidence $out)
} else {
    # Same-session terminal: original rb/GAME and rt/null resource inputs
    # were read into wholly independent C++ 66-slot and text owners.
    $current=@($report | Where-Object {
        $_ -cmatch '^OWNER_INDEPENDENT_SESSION: [0-9a-f]{32}$'
    })
    if ($current.Count -ne 1) {
        throw 'Staged diagnostic missing exactly one retail session'
    }
    $session=([regex]::Match($current[0],
        '^OWNER_INDEPENDENT_SESSION: ([0-9a-f]{32})$')).Groups[1].Value
    $strict=('^OWNER_CLIENT_ENGINE_FS_STAGED_RESULTS: sin-independent-'+
        $session+'-[0-9]+\.log:client:owned-staged-verified-not-engine-init:66:62:8:156:66:7:7$')
    if (-not (ExactLine $report 'OWNER_CLIENT_ENGINE_FS_STAGED_PROBE: ENABLED_TERMINAL_DIAGNOSTIC') -or
        -not (ExactLine $report 'OWNER_CLIENT_ENGINE_FS_READ_PROBE: ENABLED_TERMINAL_DIAGNOSTIC') -or
        @($report | Where-Object {
            $_ -cmatch '^OWNER_CLIENT_ENGINE_FS_STAGED_RESULTS: '
        }).Count -ne 1 -or
        @($report | Where-Object {$_ -cmatch $strict}).Count -ne 1) {
        throw 'Staged diagnostic exit lacks same-session original-mode 66:62:8:156:66 owned-binding proof'
    }
    if ($Phase -in @('Hooks','Soundscape')) {
        $hookPattern=('^OWNER_CLIENT_ENGINE_FS_HOOK_RESULTS: sin-independent-'+
            $session+'-[0-9]+\.log:client:three-owned-hooks-bound-not-dispatched:27:28:5:3$')
        if (-not (ExactLine $report 'OWNER_CLIENT_ENGINE_FS_HOOK_PROBE: ENABLED_TERMINAL_DIAGNOSTIC') -or
            @($report | Where-Object {
                $_ -cmatch '^OWNER_CLIENT_ENGINE_FS_HOOK_RESULTS: '
            }).Count -ne 1 -or
            @($report | Where-Object {$_ -cmatch $hookPattern}).Count -ne 1) {
            throw 'Hook proof lacks same-session owned 27:28:5 message-hook binding'
        }
    } else {
        if (-not (ExactLine $report 'OWNER_CLIENT_ENGINE_FS_HOOK_PROBE: DISABLED')) {
            throw 'Original staged gate unexpectedly enabled the new hook proof'
        }
    }
    if ($Phase -eq 'Soundscape') {
        # Independent fourth-stage proof: the SAME session that showed
        # all three Stats hooks must have read and parsed seven original
        # soundscape scripts through VFileSystem012 with rb/GAME.
        $pattern=('^OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS: sin-independent-'+
            $session+'-[0-9]+\.log:client:owned-soundscape-catalog-verified-original-files:7:301:242:3$')
        if (-not (ExactLine $report 'OWNER_CLIENT_SOUNDSCAPE_CATALOG_PROBE: ENABLED_TERMINAL_DIAGNOSTIC') -or
            @($report | Where-Object {
                $_ -cmatch '^OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS: '
            }).Count -ne 1 -or
            @($report | Where-Object {$_ -cmatch $pattern}).Count -ne 1) {
            throw 'Soundscape proof lacks original 7:301:242:3 engine-read catalog evidence'
        }
    }
}
Write-Output ("OWNER_ENGINE_FS_GATE_RESULT: {0}_DIAGNOSTIC_ONLY_NO_GAME_BOOT" -f
    $Phase.ToUpperInvariant())
