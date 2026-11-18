# Offline classifier for ONE opt-in independent Windows game-launch session.
# Consumes only owner-local diagnostic logs; never executes or reads game code.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{32}$')][string]$Session,
    [Parameter(Mandatory=$true)][string]$LogDirectory
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not (Test-Path -LiteralPath $LogDirectory -PathType Container)) {
    throw "Missing startup trace directory"
}
$logs = @(Get-ChildItem -LiteralPath $LogDirectory -File -Filter ("sin-independent-{0}-*.log" -f $Session) |
    Where-Object { $_.Name -cmatch ("^sin-independent-{0}-[0-9]+\.log$" -f $Session) } |
    Sort-Object Name)
$seen = @{}
$issues = New-Object System.Collections.Generic.List[string]
$requests = New-Object System.Collections.Generic.List[string]
$factoryResults = New-Object System.Collections.Generic.List[string]
$callbackEvents = 0
$unsupportedCallbacks = New-Object System.Collections.Generic.List[string]
$soundCallbacks = New-Object System.Collections.Generic.List[string]
$soundTerminalLogs = New-Object System.Collections.Generic.List[string]
$clientSoundTerminalLogs = New-Object System.Collections.Generic.List[string]
$eventTerminalLogs = New-Object System.Collections.Generic.List[string]
$cvarTerminalLogs = New-Object System.Collections.Generic.List[string]
$explicitInitRoles = @{}
$legacyInitEvents = 0
$blockersByLog = New-Object System.Collections.Generic.List[string]
foreach ($log in $logs) {
    $logFirstBlocker = $null
    $soundEntered = $false
    $clientSoundEntered = $false
    $clientSoundResult = $null
    $soundResult = $null
    $eventStage = 0
    $eventResult = $null
    $cvarStage = 0
    $cvarResult = $null
    foreach ($line in (Get-Content -LiteralPath $log.FullName)) {
        if ($line -notmatch '^\[SiN independent\] role=(client|server) phase=([a-z0-9-]+) version=([^\s]+)$') {
            continue
        }
        $role = $Matches[1]
        $phase = $Matches[2]
        $version = $Matches[3]
        $seen[$role] = $true
        if ($phase -eq 'factory-accepted' -or $phase -eq 'factory-rejected') {
            $factoryResults.Add(("{0}:{1}:{2}" -f $role,$phase,$version))
        } else {
            $callbackEvents++
        }
        if ($phase -eq 'unsupported-callback') {
            $unsupportedCallbacks.Add(("{0}:{1}:{2}" -f $log.Name,$role,$version))
        }
        if ($role -eq 'client' -and $version -eq 'VSoundEmitter001') {
            if ($phase -eq 'sound-connect-entered') {
                $clientSoundEntered = $true
                $clientSoundResult = $null
            } elseif ($clientSoundEntered -and $phase -cin @('sound-connect-succeeded','sound-connect-rejected')) {
                $clientSoundResult = $phase
            } elseif ($clientSoundEntered -and $null -ne $clientSoundResult -and $phase -eq 'sound-probe-terminal-stop') {
                $clientSoundTerminalLogs.Add(("{0}:client:{1}" -f $log.Name,$clientSoundResult))
                $clientSoundEntered = $false
                $clientSoundResult = $null
            }
        }
        if ($role -eq 'server' -and $version -eq 'VSoundEmitter001') {
            if ($phase -eq 'sound-connect-entered') { $soundEntered = $true; $soundResult = $null }
            if ($soundEntered -and ($phase -eq 'sound-connect-succeeded' -or $phase -eq 'sound-connect-rejected')) {
                $soundResult = $phase
                $soundCallbacks.Add(("{0}:{1}:{2}" -f $log.Name,$role,$phase))
            }
            if ($soundEntered -and $null -ne $soundResult -and $phase -eq 'sound-probe-terminal-stop') {
                $soundTerminalLogs.Add(("{0}:{1}" -f $log.Name,$soundResult))
            }
        }
        if ($role -eq 'server' -and $soundResult -eq 'sound-connect-succeeded') {
            if ($phase -eq 'math-tables-initialized' -and $version -eq 'server-scalar') {
                $eventStage = 1
                $eventResult = $null
            } elseif ($eventStage -eq 1 -and $phase -eq 'factory-state-cached' -and $version -eq 'server') {
                $eventStage = 2
            } elseif ($eventStage -eq 2 -and $phase -eq 'event-load-entered' -and $version -eq 'resource/gameevents.res') {
                $eventStage = 3
            } elseif ($eventStage -eq 3 -and $phase -eq 'event-load-returned' -and
                $version -cmatch '^resource/gameevents\.res:(-?[0-9]+)$') {
                $count = 0
                if ([int]::TryParse($Matches[1],[ref]$count)) {
                    $eventResult = $count
                    $eventStage = 4
                }
            } elseif ($eventStage -eq 4 -and $phase -eq 'event-probe-terminal-stop' -and $version -eq 'resource/gameevents.res') {
                $eventTerminalLogs.Add(("{0}:{1}" -f $log.Name,$eventResult))
                $eventStage = 5
            }
        }
        if ($role -eq 'server' -and $eventStage -eq 4 -and $eventResult -gt 0) {
            if ($cvarStage -eq 0 -and $phase -eq 'cvar-name-available' -and $version -eq 'dd_log_events') {
                $cvarStage = 1
            } elseif ($cvarStage -eq 1 -and $phase -eq 'cvar-register-entered' -and $version -eq 'dd_log_events') {
                $cvarStage = 2
            } elseif ($cvarStage -eq 2 -and $phase -eq 'cvar-register-returned' -and $version -eq 'dd_log_events') {
                $cvarStage = 3
            } elseif ($cvarStage -eq 3 -and $phase -eq 'cvar-registration-verified' -and $version -eq 'dd_log_events') {
                $cvarStage = 4
            } elseif ($phase -eq 'cvar-probe-result') {
                if (($version -eq 'verified' -and $cvarStage -eq 4) -or
                    ($version -cin @('invalid-interface','existing-name','invalid-list') -and $cvarStage -eq 0) -or
                    ($version -eq 'bad-owned-object' -and $cvarStage -in @(1,3)) -or
                    ($version -cin @('lookup-mismatch','unexpected-relink','override-present') -and $cvarStage -eq 3)) {
                    $cvarResult = $version
                    $cvarStage = 5
                }
            } elseif ($cvarStage -eq 5 -and $phase -eq 'cvar-probe-terminal-stop' -and $version -eq 'dd_log_events') {
                $cvarTerminalLogs.Add(("{0}:{1}" -f $log.Name,$cvarResult))
                $cvarStage = 6
            }
        }
        if ($phase -eq 'init-entered') { $explicitInitRoles[$role] = $true }
        # Backward-compatible classification of old traces. Only an actual
        # Init/DLLInit body can emit these, unlike Shutdown or factory probes.
        if ($phase -eq 'request' -or $phase -eq 'missing-interface' -or
            $phase -eq 'missing-factory' -or $phase -eq 'invalid-init-arguments' -or
            $phase -eq 'sdk2004-preflight-only') { $legacyInitEvents++ }
        if ($phase -eq 'request') { $requests.Add(("{0}:{1}" -f $role,$version)) }
        if ($phase -eq 'missing-interface' -or
            $phase -eq 'missing-factory' -or
            $phase -eq 'invalid-init-arguments') {
            $blocker = ("{0}:{1}:{2}" -f $role,$phase,$version)
            $issues.Add($blocker)
            if ($null -eq $logFirstBlocker) { $logFirstBlocker = $blocker }
        }
    }
    if ($null -ne $logFirstBlocker) {
        $blockersByLog.Add(("{0}={1}" -f $log.Name,$logFirstBlocker))
    }
}
Write-Output ("OWNER_INDEPENDENT_TRACE_FILES: {0}" -f $logs.Count)
Write-Output ("OWNER_INDEPENDENT_ROLES_OBSERVED: {0}" -f ((@('client','server') | Where-Object { $seen.ContainsKey($_) }) -join ','))
Write-Output ("OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: {0}" -f ((@('client','server') | Where-Object { $explicitInitRoles.ContainsKey($_) }) -join ','))
Write-Output ("OWNER_INDEPENDENT_FACTORY_REQUESTS: {0}" -f ($requests -join ','))
Write-Output ("OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: {0}" -f ($factoryResults -join ','))
if ($unsupportedCallbacks.Count -gt 0) {
    Write-Output ("OWNER_INDEPENDENT_UNSUPPORTED_CALLBACKS: {0}" -f ($unsupportedCallbacks -join ','))
}
if ($soundCallbacks.Count -gt 0) {
    Write-Output ("OWNER_SERVER_SOUND_CONNECT_RESULTS: {0}" -f ($soundCallbacks -join ','))
}
if ($clientSoundTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SOUND_CONNECT_RESULTS: {0}" -f ($clientSoundTerminalLogs -join ','))
}
if ($eventTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_SERVER_EVENT_LOAD_RESULTS: {0}" -f ($eventTerminalLogs -join ','))
}
if ($cvarTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_SERVER_CVAR_PROBE_RESULTS: {0}" -f ($cvarTerminalLogs -join ','))
}
if ($issues.Count -gt 0) {
    Write-Output ("OWNER_INDEPENDENT_BLOCKERS_BY_LOG: {0}" -f ($blockersByLog -join ','))
    if ($blockersByLog.Count -eq 1) {
        Write-Output ("OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: {0}" -f $issues[0])
    } else {
        Write-Output "OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: AMBIGUOUS_ACROSS_PROCESSES"
    }
    Write-Output "OWNER_INDEPENDENT_VERDICT: ENGINE_ENTERED_INDEPENDENT_INIT_BUT_REJECTED"
} elseif ($unsupportedCallbacks.Count -gt 0) {
    # A terminal callback can occur before the first Init call. It proves
    # virtual entry and diagnostic termination, not that Init was entered.
    Write-Output "OWNER_INDEPENDENT_VERDICT: UNSUPPORTED_CALLBACK_DIAGNOSTIC_STOP_NO_BOOT_PROOF"
} elseif ($cvarTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CVAR_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($eventTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: EVENT_LOAD_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($clientSoundTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_SOUND_CONNECT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($soundTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: SOUND_CONNECT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($explicitInitRoles.Count -eq 0 -and $legacyInitEvents -eq 0 -and $factoryResults.Count -gt 0) {
    # The engine reached CreateInterface, but may have probed optional
    # names. No claim that a rejection is the fatal retail startup cause.
    Write-Output "OWNER_INDEPENDENT_VERDICT: GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE"
} elseif ($explicitInitRoles.Count -eq 0 -and $legacyInitEvents -eq 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: NO_INDEPENDENT_INIT_EVIDENCE"
} else {
    Write-Output "OWNER_INDEPENDENT_VERDICT: DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF"
}
# Factory-only observations cannot establish retail Init or boot success.
# PID-sorted files cannot establish cross-process event chronology.
# Intentionally no SUCCESS verdict: the current independent DLLs always
# return failure and do not implement a playable retail engine contract.
