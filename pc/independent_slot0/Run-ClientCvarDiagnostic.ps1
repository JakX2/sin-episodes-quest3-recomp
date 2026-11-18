# Phase 348 convenience wrapper. Existing launcher owns all staging safeguards.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$GameRoot,
    [string]$GameArguments = '-console',
    [ValidateRange(10,300)][int]$TimeoutSeconds=90,
    [switch]$EnableClientOwnedStartupProbe,
    [switch]$EnableClientVGuiProbe,
    [switch]$EnableClientVGuiLayoutProbe,
    [switch]$EnableClientEngineVGuiSlot1Probe,
    [switch]$DisposableTestCopyConfirmed
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if (-not $DisposableTestCopyConfirmed) {
    throw 'Use only an existing disposable test copy; explicit confirmation required.'
}
$manifest=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'manifest.json') -Raw | ConvertFrom-Json
if ($manifest.schema -ne 1 -or $manifest.commit -cnotmatch '^[0-9a-f]{40}$') {
    throw 'Invalid bundle manifest.'
}
$required=@('bin/client.dll','bin/server.dll','Invoke-OwnerIndependentEngineProbe.ps1',
    'Get-OwnerEngineExitDiagnosis.ps1','Get-OwnerIndependentStartupVerdict.ps1',
    'Run-ClientCvarDiagnostic.ps1','OWNER_CLIENT_CVAR_BUNDLE.md')
$entries=@($manifest.files.PSObject.Properties)
if ($entries.Count -ne $required.Count) { throw 'Unexpected manifest file count.' }
foreach ($name in $required) {
    $entry=$manifest.files.PSObject.Properties[$name]
    if ($null -eq $entry -or $entry.Value -cnotmatch '^[0-9a-f]{64}$') {
        throw "Invalid manifest entry: $name"
    }
    $file=Join-Path $PSScriptRoot $name
    $item=Get-Item -LiteralPath $file -ErrorAction Stop
    if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Unexpected bundle file type: $name"
    }
    if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant() -cne $entry.Value) {
        throw "Bundle integrity mismatch: $name"
    }
}
Write-Output ("OWNER_DIAGNOSTIC_BUNDLE_COMMIT: {0}" -f $manifest.commit)
& (Join-Path $PSScriptRoot 'Invoke-OwnerIndependentEngineProbe.ps1') `
    -GameBin (Join-Path $GameRoot 'SE1/bin') `
    -GameExecutable (Join-Path $GameRoot 'SinEpisodes.exe') `
    -IndependentBin (Join-Path $PSScriptRoot 'bin') `
    -GameArguments $GameArguments -TimeoutSeconds $TimeoutSeconds `
    -ProbeRole Client -EnableClientSoundProbe -EnableClientMaterialProbe `
    -EnableClientScreenProbe -EnableClientGaussianProbe -EnableClientCvarProbe `
    -EnableClientOwnedStartupProbe:$EnableClientOwnedStartupProbe -EnableClientVGuiProbe:$EnableClientVGuiProbe -EnableClientVGuiLayoutProbe:$EnableClientVGuiLayoutProbe -EnableClientEngineVGuiSlot1Probe:$EnableClientEngineVGuiSlot1Probe -DisposableTestCopyConfirmed
