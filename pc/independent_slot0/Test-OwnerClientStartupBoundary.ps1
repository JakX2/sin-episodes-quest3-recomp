# Synthetic only: verifies signed-bundle gate, explicit disposable confirmation
# and exact client-only source-gated handoff. No actual DLL or game is run.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Wrapper)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
function Get-Sha256([string]$Path) {
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        return [System.BitConverter]::ToString(
            $hasher.ComputeHash($stream)).Replace('-','').ToLowerInvariant()
    } finally {
        $stream.Dispose()
        $hasher.Dispose()
    }
}
$root=Join-Path ([IO.Path]::GetTempPath()) ('sin-client-startup-owner-'+[Guid]::NewGuid().ToString('N'))
$bundle=Join-Path $root 'bundle with spaces'
$null=New-Item -ItemType Directory -Path (Join-Path $bundle 'bin') -Force
try {
    Copy-Item -LiteralPath $Wrapper -Destination (Join-Path $bundle 'Run-OwnerClientStartupBoundary.ps1')
    $stub=Join-Path $bundle 'Invoke-OwnerIndependentEngineProbe.ps1'
    @'
[CmdletBinding()]
param(
    [string]$GameBin,[string]$GameExecutable,[string]$IndependentBin,
    [string]$GameArguments,[int]$TimeoutSeconds,[string]$ProbeRole,
    [switch]$EnableClientPredictionFactoryProbe,
    [switch]$EnableClientPredictionCvarRegisterProbe,
    [switch]$EnableClientEntityListFactoryProbe,
    [switch]$EnableClientCenterprintFactoryProbe,
    [switch]$EnableClientLeafFactoryProbe,
    [switch]$EnableClientSoundConnectContinueProbe,
    [switch]$DisposableTestCopyConfirmed
)
$ErrorActionPreference='Stop'
if ($ProbeRole -cne 'Client' -or
    -not $EnableClientPredictionFactoryProbe -or
    -not $EnableClientPredictionCvarRegisterProbe -or
    -not $EnableClientEntityListFactoryProbe -or
    -not $EnableClientCenterprintFactoryProbe -or
    -not $EnableClientSoundConnectContinueProbe -or
    -not $DisposableTestCopyConfirmed -or
    $TimeoutSeconds -ne 45 -or
    $GameArguments -cne '-console -game SE1' -or
    $GameBin -cne (Join-Path $env:SIN_CLIENT_STARTUP_SYNTH_GAME 'SE1/bin') -or
    $GameExecutable -cne (Join-Path $env:SIN_CLIENT_STARTUP_SYNTH_GAME 'SinEpisodes.exe') -or
    $IndependentBin -cne (Join-Path $PSScriptRoot 'bin')) {
    throw 'Incorrect client-only startup parameters'
}
Set-Content -LiteralPath (Join-Path $PSScriptRoot 'called.txt') -Value $(if ($EnableClientLeafFactoryProbe) {'client-plus-leaf-cold'} else {'client-only-factory-and-sound'})
'@ | Set-Content -LiteralPath $stub
    $required=@(
        'bin/client.dll','bin/server.dll',
        'Invoke-OwnerIndependentEngineProbe.ps1',
        'Get-OwnerEngineExitDiagnosis.ps1',
        'Get-OwnerIndependentStartupVerdict.ps1',
        'Run-OwnerClientStartupBoundary.ps1',
        'OWNER_CLIENT_STARTUP_BOUNDARY.md'
    )
    foreach($name in $required){
        $p=Join-Path $bundle $name
        if(-not (Test-Path -LiteralPath $p)) {
            Set-Content -LiteralPath $p -Value ('synthetic '+$name)
        }
    }
    $hashes=@{}
    foreach ($name in $required) {
        $hashes[$name]=(Get-Sha256 (Join-Path $bundle $name))
    }
    $manifestPath=Join-Path $bundle 'manifest.json'
    $commit='a'*40
    @{schema=1;commit=$commit;files=$hashes}|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $manifestPath
    $game=Join-Path $root 'disposable game with spaces'
    $run=Join-Path $bundle 'Run-OwnerClientStartupBoundary.ps1'
    $called=Join-Path $bundle 'called.txt'
    $old=$env:SIN_CLIENT_STARTUP_SYNTH_GAME
    $env:SIN_CLIENT_STARTUP_SYNTH_GAME=$game
    function Reject([string]$pattern,[bool]$confirmed) {
        Remove-Item -LiteralPath $called -ErrorAction SilentlyContinue
        $failure=$null
        try {
            & $run -GameRoot $game -GameArguments '-console -game SE1' -TimeoutSeconds 45 -DisposableTestCopyConfirmed:$confirmed | Out-Null
        } catch { $failure=$_.Exception.Message }
        if($null -eq $failure -or $failure -notlike $pattern -or
           (Test-Path -LiteralPath $called)) {
            throw ('Expected refusal '+$pattern+'; observed '+$failure)
        }
    }
    try {
        Reject '*explicit confirmation*' $false
        & $run -GameRoot $game -GameArguments '-console -game SE1' -TimeoutSeconds 45 -DisposableTestCopyConfirmed | Out-Null
        if (-not (Test-Path -LiteralPath $called)) {
            throw 'Correct bundle did not delegate to the client-only stub'
        }
        if ((Get-Content -LiteralPath $called -Raw).Trim() -cne 'client-only-factory-and-sound') {
            throw 'Incorrect client-only delegation'
        }
        Remove-Item -LiteralPath $called -ErrorAction SilentlyContinue
        & $run -GameRoot $game -GameArguments '-console -game SE1' -TimeoutSeconds 45 -DisposableTestCopyConfirmed -EnableClientLeafFactoryProbe | Out-Null
        if (-not (Test-Path -LiteralPath $called) -or
            (Get-Content -LiteralPath $called -Raw).Trim() -cne 'client-plus-leaf-cold') {
            throw 'Explicit client-leaf opt-in did not reach isolated guard stub'
        }
        $client=Join-Path $bundle 'bin/client.dll'
        Add-Content -LiteralPath $client -Value 'tampered'
        Reject '*Bundle integrity mismatch*' $true
        Set-Content -LiteralPath $client -Value 'synthetic bin/client.dll'
        $hashes['bin/client.dll']=(Get-Sha256 $client)
        $hashes['unexpected.txt']='b'*64
        @{schema=1;commit=$commit;files=$hashes}|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $manifestPath
        Reject '*manifest file count*' $true
        $hashes.Remove('unexpected.txt')
        $hashes.Remove('bin/server.dll')
        @{schema=1;commit=$commit;files=$hashes}|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $manifestPath
        Reject '*manifest file count*' $true
        Write-Output 'PASS synthetic client-only startup bundle leaf opt-in, exact flags, SHA pins, and fail-closed paths'
    } finally { $env:SIN_CLIENT_STARTUP_SYNTH_GAME=$old }
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
