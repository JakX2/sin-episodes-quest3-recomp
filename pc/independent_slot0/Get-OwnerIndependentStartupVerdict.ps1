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
$blockersByLog = New-Object System.Collections.Generic.List[string]
foreach ($log in $logs) {
    $logFirstBlocker = $null
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
Write-Output ("OWNER_INDEPENDENT_FACTORY_REQUESTS: {0}" -f ($requests -join ','))
Write-Output ("OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: {0}" -f ($factoryResults -join ','))
if ($issues.Count -gt 0) {
    Write-Output ("OWNER_INDEPENDENT_BLOCKERS_BY_LOG: {0}" -f ($blockersByLog -join ','))
    if ($blockersByLog.Count -eq 1) {
        Write-Output ("OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: {0}" -f $issues[0])
    } else {
        Write-Output "OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: AMBIGUOUS_ACROSS_PROCESSES"
    }
    Write-Output "OWNER_INDEPENDENT_VERDICT: ENGINE_ENTERED_INDEPENDENT_INIT_BUT_REJECTED"
} elseif ($callbackEvents -eq 0 -and $factoryResults.Count -gt 0) {
    # The engine reached CreateInterface, but may have probed optional
    # names. No claim that a rejection is the fatal retail startup cause.
    Write-Output "OWNER_INDEPENDENT_VERDICT: GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE"
} elseif ($logs.Count -eq 0 -or $seen.Count -eq 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: NO_INDEPENDENT_INIT_EVIDENCE"
} else {
    Write-Output "OWNER_INDEPENDENT_VERDICT: DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF"
}
# Factory-only observations cannot establish retail Init or boot success.
# PID-sorted files cannot establish cross-process event chronology.
# Intentionally no SUCCESS verdict: the current independent DLLs always
# return failure and do not implement a playable retail engine contract.
