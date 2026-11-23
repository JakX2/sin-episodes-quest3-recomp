#requires -Version 5.1
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Launcher)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$tokens=$null
$parseErrors=$null
$ast=[System.Management.Automation.Language.Parser]::ParseFile(
    $Launcher,[ref]$tokens,[ref]$parseErrors)
if ($null -eq $ast -or @($parseErrors).Count -ne 0) {
    throw 'Owner-only Elite callback tracing script contains PowerShell syntax errors.'
}
$source=[IO.File]::ReadAllText($Launcher)
$required=@(
    'DisposableTestCopyConfirmed',
    'Get-FileHash',
    'SHA256',
    'Get-Pe32',
    'sin_original_server.dll',
    '--disposable-confirmed',
    'Get-Content',
    'ConvertFrom-Json',
    'elite_hardware_watch_armed',
    'retail_server_sha256_verified',
    'WatchProfile',
    'watch_profile_weapon',
    'watch_profile_elite',
    'watch_profile_event',
    'event_id_readable',
    'ORIGINAL_SECONDARY_EVENT43_CLASSIFIED',
    'ORIGINAL_SECONDARY_EVENT47_CLASSIFIED',
    'ORIGINAL_SECONDARY_EVENT_ID_UNREADABLE',
    'ORIGINAL_FIRST_EVENT43_CLASSIFIED',
    'ORIGINAL_WEAPON_GATE_ENTRY_NOT_OBSERVED',
    'ORIGINAL_ENGINE_ELITE_TICK_NOT_OBSERVED',
    'ORIGINAL_WEAPON_GATE_ENTRY_NOT_OBSERVED',
    'ORIGINAL_WEAPON_TIMED_UPDATE_NOT_OBSERVED',
    'ORIGINAL_ENGINE_SPAWN_ENTRY_NOT_OBSERVED',
    'ORIGINAL_ENGINE_CALLBACK_EVIDENCE_NOT_PROVEN'
)
foreach($needle in $required) {
    if(-not $source.Contains($needle)) {
        throw "Phase627 owner runner omitted mandatory validation: $needle"
    }
}
if($source -match 'Start-Process') {
    throw 'Owner runner must use the synchronous, evidence-returning tracer directly.'
}
Write-Output 'PASS Phase627 owner-only PowerShell syntax, original SHA gate and no-bogus-proof witness'
