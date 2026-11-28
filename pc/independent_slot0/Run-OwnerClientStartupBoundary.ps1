# Owner-only independent client startup boundary runner.
# This delegates all game-file changes, SHA pinning and restoration to
# Invoke-OwnerIndependentEngineProbe.ps1. It is not a playable game build.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$GameRoot,
    [string]$GameArguments='-console -game SE1',
    [ValidateRange(10,300)][int]$TimeoutSeconds=75,
    [switch]$DisposableTestCopyConfirmed
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if (-not $DisposableTestCopyConfirmed) {
    throw 'Use an existing disposable test copy only; explicit confirmation required.'
}
$manifestPath=Join-Path $PSScriptRoot 'manifest.json'
$manifest=Get-Content -LiteralPath $manifestPath -Raw -ErrorAction Stop | ConvertFrom-Json
if ($manifest.schema -ne 1 -or $manifest.commit -cnotmatch '^[0-9a-f]{40}$') {
    throw 'Invalid client startup bundle manifest.'
}
$required=@(
    'bin/client.dll','bin/server.dll',
    'Invoke-OwnerIndependentEngineProbe.ps1',
    'Get-OwnerEngineExitDiagnosis.ps1',
    'Get-OwnerIndependentStartupVerdict.ps1',
    'Run-OwnerClientStartupBoundary.ps1',
    'OWNER_CLIENT_STARTUP_BOUNDARY.md'
)
$entries=@($manifest.files.PSObject.Properties)
if ($entries.Count -ne $required.Count) {
    throw 'Unexpected client startup bundle manifest file count.'
}
foreach ($name in $required) {
    $entry=$manifest.files.PSObject.Properties[$name]
    if ($null -eq $entry -or $entry.Value -cnotmatch '^[0-9a-f]{64}$') {
        throw "Invalid manifest entry: $name"
    }
    $path=Join-Path $PSScriptRoot $name
    $item=Get-Item -LiteralPath $path -ErrorAction Stop
    if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Invalid bundle file type: $name"
    }
    $hash=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($hash -cne $entry.Value) { throw "Bundle integrity mismatch: $name" }
}
Write-Output ("OWNER_CLIENT_STARTUP_BUNDLE_COMMIT: {0}" -f $manifest.commit)
Write-Output 'OWNER_CLIENT_STARTUP_MODE: CLIENT_ONLY_SOUND_CONNECT_PREDICTION_ENTITYLIST_CENTERPRINT_DIAGNOSTIC'
Write-Output 'OWNER_CLIENT_STARTUP_BOOT_CLAIM: NONE'
# Original server.dll stays in place. The independent client is swapped
# only by the SHA-pinned lower-level launcher, which restores on exit.
$argsToOwner=@{
    GameBin=(Join-Path $GameRoot 'SE1/bin')
    GameExecutable=(Join-Path $GameRoot 'SinEpisodes.exe')
    IndependentBin=(Join-Path $PSScriptRoot 'bin')
    GameArguments=$GameArguments
    TimeoutSeconds=$TimeoutSeconds
    ProbeRole='Client'
    EnableClientPredictionFactoryProbe=$true
    EnableClientPredictionCvarRegisterProbe=$true
    EnableClientEntityListFactoryProbe=$true
    EnableClientCenterprintFactoryProbe=$true
    EnableClientSoundConnectContinueProbe=$true
    DisposableTestCopyConfirmed=$true
}
& (Join-Path $PSScriptRoot 'Invoke-OwnerIndependentEngineProbe.ps1') @argsToOwner
