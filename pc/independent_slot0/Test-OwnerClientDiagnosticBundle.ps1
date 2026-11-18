# Synthetic bundle-only exercise. Never starts the actual game or actual stager.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Wrapper)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Join-Path ([IO.Path]::GetTempPath()) ('sin-bundle-test-'+[Guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $root
try {
    $bundle=Join-Path $root 'bundle with spaces'
    $null=New-Item -ItemType Directory -Path (Join-Path $bundle 'bin') -Force
    $name='Run-ClientCvarDiagnostic.ps1'
    Copy-Item -LiteralPath $Wrapper -Destination (Join-Path $bundle $name)
    $stub=Join-Path $bundle 'Invoke-OwnerIndependentEngineProbe.ps1'
    @'
param(
    [string]$GameBin,[string]$GameExecutable,[string]$IndependentBin,
    [string]$GameArguments,[int]$TimeoutSeconds,[string]$ProbeRole,
    [switch]$EnableClientSoundProbe,[switch]$EnableClientMaterialProbe,
    [switch]$EnableClientScreenProbe,[switch]$EnableClientGaussianProbe,
    [switch]$EnableClientCvarProbe,[switch]$EnableClientOwnedStartupProbe,[switch]$EnableClientVGuiProbe,[switch]$EnableClientVGuiLayoutProbe,[switch]$DisposableTestCopyConfirmed
)
$ErrorActionPreference='Stop'
if ($ProbeRole -cne 'Client' -or -not $EnableClientSoundProbe -or
    -not $EnableClientMaterialProbe -or -not $EnableClientScreenProbe -or
    -not $EnableClientGaussianProbe -or -not $EnableClientCvarProbe -or
    -not $DisposableTestCopyConfirmed -or $TimeoutSeconds -ne 45 -or
    $GameArguments -cne '-console -windowed' -or
    $GameBin -cne (Join-Path $env:SIN_TEST_BUNDLE_GAME 'SE1/bin') -or
    $GameExecutable -cne (Join-Path $env:SIN_TEST_BUNDLE_GAME 'SinEpisodes.exe') -or
    $IndependentBin -cne (Join-Path $PSScriptRoot 'bin')) { throw 'Incorrect delegated parameters' }
Set-Content -LiteralPath (Join-Path $PSScriptRoot 'called.txt') -Value $(if($EnableClientVGuiLayoutProbe -and $EnableClientVGuiProbe -and $EnableClientOwnedStartupProbe){'layout'}elseif($EnableClientVGuiProbe -and $EnableClientOwnedStartupProbe){'vgui'}elseif($EnableClientOwnedStartupProbe){'owned'}else{'called'})
'@ | Set-Content -LiteralPath $stub
    $files=@('bin/client.dll','bin/server.dll','Invoke-OwnerIndependentEngineProbe.ps1',
        'Get-OwnerEngineExitDiagnosis.ps1','Get-OwnerIndependentStartupVerdict.ps1',
        $name,'OWNER_CLIENT_CVAR_BUNDLE.md')
    foreach($file in $files){
        $p=Join-Path $bundle $file
        if(-not (Test-Path -LiteralPath $p)){Set-Content -LiteralPath $p -Value "synthetic $file"}
    }
    $hashes=@{}
    foreach($file in $files){
        $hashes[$file]=(Get-FileHash -LiteralPath (Join-Path $bundle $file) -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    $manifest=Join-Path $bundle 'manifest.json'
    $commit='a'*40
    @{schema=1;commit=$commit;files=$hashes}|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $manifest
    $run=Join-Path $bundle $name
    $called=Join-Path $bundle 'called.txt'
    $game=Join-Path $root 'game with spaces'
    $previous=$env:SIN_TEST_BUNDLE_GAME
    $env:SIN_TEST_BUNDLE_GAME=$game
    function Expect-Refusal([string]$pattern,[bool]$confirmed){
        $failure=$null
        try {
            & $run -GameRoot $game -TimeoutSeconds 45 -GameArguments '-console -windowed' -DisposableTestCopyConfirmed:$confirmed
        } catch {$failure=$_.Exception.Message}
        if($null -eq $failure -or $failure -notlike $pattern -or (Test-Path -LiteralPath $called)){
            throw "Expected refusal: $pattern; observed: $failure"
        }
    }
    try {
        Expect-Refusal '*explicit confirmation*' $false
        & $run -GameRoot $game -TimeoutSeconds 45 -GameArguments '-console -windowed' -DisposableTestCopyConfirmed
        if((Get-Content -LiteralPath $called -Raw).Trim() -cne 'called'){throw 'Default wrapper did not preserve Phase 348 mode'}
        Remove-Item -LiteralPath $called
        & $run -GameRoot $game -TimeoutSeconds 45 -GameArguments '-console -windowed' -EnableClientOwnedStartupProbe -DisposableTestCopyConfirmed
        if((Get-Content -LiteralPath $called -Raw).Trim() -cne 'owned'){throw 'Owned startup switch was not delegated'}
        Remove-Item -LiteralPath $called
        & $run -GameRoot $game -TimeoutSeconds 45 -GameArguments '-console -windowed' -EnableClientOwnedStartupProbe -EnableClientVGuiProbe -DisposableTestCopyConfirmed
        if((Get-Content -LiteralPath $called -Raw).Trim() -cne 'vgui'){throw 'VGUI switch was not delegated'}
        Remove-Item -LiteralPath $called
        & $run -GameRoot $game -TimeoutSeconds 45 -GameArguments '-console -windowed' -EnableClientOwnedStartupProbe -EnableClientVGuiProbe -EnableClientVGuiLayoutProbe -DisposableTestCopyConfirmed
        if((Get-Content -LiteralPath $called -Raw).Trim() -cne 'layout'){throw 'VGUI layout switch was not delegated'}
        Remove-Item -LiteralPath $called
        $client=Join-Path $bundle 'bin/client.dll'
        Add-Content -LiteralPath $client -Value 'modified'
        Expect-Refusal '*integrity mismatch*' $true
        Set-Content -LiteralPath $client -Value 'synthetic bin/client.dll'
        $hashes['bin/client.dll']=(Get-FileHash -LiteralPath $client -Algorithm SHA256).Hash.ToLowerInvariant()
        $hashes['unexpected.txt']='b'*64
        @{schema=1;commit=$commit;files=$hashes}|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $manifest
        Expect-Refusal '*file count*' $true
        $hashes.Remove('unexpected.txt')
        $hashes.Remove('bin/server.dll')
        $hashes['unexpected.txt']='b'*64
        @{schema=1;commit=$commit;files=$hashes}|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $manifest
        Expect-Refusal '*Invalid manifest entry*' $true
        Write-Output 'PASS: wrapper delegates exact client options; all refusal cases prevent delegation.'
    } finally {$env:SIN_TEST_BUNDLE_GAME=$previous}
} finally {Remove-Item -LiteralPath $root -Recurse -Force}
