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
foreach ($log in $logs) {
    foreach ($line in (Get-Content -LiteralPath $log.FullName)) {
        if ($line -notmatch '^\[SiN independent\] role=(client|server) phase=([a-z0-9-]+) version=([^\s]+)$') {
            continue
        }
        $role = $Matches[1]
        $phase = $Matches[2]
        $version = $Matches[3]
        $seen[$role] = $true
        if ($phase -eq 'request') { $requests.Add(("{0}:{1}" -f $role,$version)) }
        if ($phase -eq 'missing-interface' -or
            $phase -eq 'missing-factory' -or
            $phase -eq 'invalid-init-arguments') {
            $issues.Add(("{0}:{1}:{2}" -f $role,$phase,$version))
        }
    }
}
Write-Output ("OWNER_INDEPENDENT_TRACE_FILES: {0}" -f $logs.Count)
Write-Output ("OWNER_INDEPENDENT_ROLES_OBSERVED: {0}" -f ((@('client','server') | Where-Object { $seen.ContainsKey($_) }) -join ','))
Write-Output ("OWNER_INDEPENDENT_FACTORY_REQUESTS: {0}" -f ($requests -join ','))
if ($issues.Count -gt 0) {
    Write-Output ("OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: {0}" -f $issues[0])
    Write-Output "OWNER_INDEPENDENT_VERDICT: ENGINE_ENTERED_INDEPENDENT_INIT_BUT_REJECTED"
} elseif ($logs.Count -eq 0 -or $seen.Count -eq 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: NO_INDEPENDENT_INIT_EVIDENCE"
} else {
    Write-Output "OWNER_INDEPENDENT_VERDICT: DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF"
}
# Intentionally no SUCCESS verdict: the current independent DLLs always
# return failure and do not implement a playable retail engine contract.
