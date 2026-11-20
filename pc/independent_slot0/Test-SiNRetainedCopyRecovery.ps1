# Windows CI uses only fake synthetic client/server bytes in a TEMP
# directory. No retail engine DLL, no actual game or owner data is touched.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$RecoveryScript)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
function Sha([string]$Path) {
    $hasher=[Security.Cryptography.SHA256]::Create()
    $stream=[IO.File]::OpenRead($Path)
    try {
        return [BitConverter]::ToString($hasher.ComputeHash($stream)).Replace('-','').ToLowerInvariant()
    } finally {
        $stream.Dispose()
        $hasher.Dispose()
    }
}
$original=Get-Content -LiteralPath $RecoveryScript -Raw
$syntheticClient='fake original client payload'
$syntheticServer='fake original server payload'
$seed=Join-Path ([IO.Path]::GetTempPath()) ('sin-recovery-test-'+[Guid]::NewGuid().ToString('N').Substring(0,10))
[void](New-Item -ItemType Directory -Path $seed)
$children=New-Object 'System.Collections.Generic.List[string]'
try {
    $fixtureClient=Join-Path $seed 'client.original'
    $fixtureServer=Join-Path $seed 'server.original'
    [IO.File]::WriteAllText($fixtureClient,$syntheticClient)
    [IO.File]::WriteAllText($fixtureServer,$syntheticServer)
    $clientSha=Sha $fixtureClient
    $serverSha=Sha $fixtureServer
    $retailClient='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
    $retailServer='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
    if (-not $original.Contains($retailClient) -or -not $original.Contains($retailServer)) {
        throw 'Owner recovery SHA pins were removed'
    }
    $synthetic=$original.Replace($retailClient,$clientSha).Replace($retailServer,$serverSha)
    $fixture=Join-Path $seed 'Synthetic-Recover.ps1'
    [IO.File]::WriteAllText($fixture,$synthetic)
    $tokens=$null;$errors=$null
    [void][System.Management.Automation.Language.Parser]::ParseFile(
        $fixture,[ref]$tokens,[ref]$errors)
    if ($errors.Count -ne 0) {throw ('Invalid PowerShell parser output '+($errors | Out-String))}
    $plan=@(& $fixture -PlanOnly)
    if (($plan -join [Environment]::NewLine) -notmatch 'RECOVERY_PLAN_ONLY_NO_GAME_FILES_TOUCHED') {
        throw 'PlanOnly attempted game restore'
    }
    function Make-Copy {
        $tag=[Guid]::NewGuid().ToString('N').Substring(0,10)
        $root=Join-Path ([IO.Path]::GetTempPath()) ('sin-independent-fs-'+$tag)
        $children.Add($root)
        $game=Join-Path $root 'game'
        $bin=Join-Path $game 'SE1\bin'
        [void](New-Item -ItemType Directory -Path $bin -Force)
        [IO.File]::WriteAllText((Join-Path $game 'SinEpisodes.exe'),'synthetic nonexecuted EXE')
        [IO.File]::WriteAllText((Join-Path $game 'SE1\GameInfo.txt'),'synthetic game info')
        [IO.File]::WriteAllText((Join-Path $game 'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY.txt'),
           'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY_V1')
        [IO.File]::WriteAllText((Join-Path $bin 'client.dll'),'synthetic independent client')
        [IO.File]::WriteAllText((Join-Path $bin 'server.dll'),$syntheticServer)
        [IO.File]::WriteAllText((Join-Path $bin 'sin_independent_saved_client.dll'),$syntheticClient)
        return $root
    }
    function Reject([string]$Root) {
        $blocked=$false
        try {
            & $fixture -CopyRoot $Root | Out-Null
        } catch { $blocked=$true }
        if (-not $blocked) {throw ('Recovery should refuse invalid fixture: '+$Root)}
    }
    $good=Make-Copy
    $before=(Sha (Join-Path $good 'game\SE1\bin\client.dll'))
    $pre=@(& $fixture -CopyRoot $good -PlanOnly)
    if (($pre -join [Environment]::NewLine) -notmatch 'RECOVERY_ORIGINAL_BACKUP: SHA_VERIFIED' -or
        (Sha (Join-Path $good 'game\SE1\bin\client.dll')) -cne $before) {
        throw 'PlanOnly mutated the synthetic clone'
    }
    $result=@(& $fixture -CopyRoot $good)
    if (($result -join [Environment]::NewLine) -notmatch 'RECOVERY_VERIFIED_ORIGINAL_DLLS:' -or
        (Sha (Join-Path $good 'game\SE1\bin\client.dll')) -cne $clientSha -or
        (Test-Path -LiteralPath (Join-Path $good 'game\SE1\bin\sin_independent_saved_client.dll'))) {
        throw 'Synthetic original backup recovery did not restore verified client'
    }
    $result=@(& $fixture -CopyRoot $good -CleanupRecoveredCopy)
    if (Test-Path -LiteralPath $good) {
        throw 'CleanupVerifiedCopy did not remove already-restored test clone'
    }
    $broken=Make-Copy
    [IO.File]::WriteAllText((Join-Path $broken 'game\SIN_INDEPENDENT_DISPOSABLE_TEST_COPY.txt'),'invalid')
    Reject $broken
    if (-not (Test-Path -LiteralPath (Join-Path $broken 'game\SE1\bin\sin_independent_saved_client.dll'))) {
        throw 'Invalid marker caused unsafe backup deletion'
    }
    $badBackup=Make-Copy
    [IO.File]::WriteAllText((Join-Path $badBackup 'game\SE1\bin\sin_independent_saved_client.dll'),'corrupt backup')
    Reject $badBackup
    if (-not (Test-Path -LiteralPath (Join-Path $badBackup 'game\SE1\bin\sin_independent_saved_client.dll'))) {
        throw 'Invalid saved SHA caused unsafe backup deletion'
    }
    $badServer=Make-Copy
    [IO.File]::WriteAllText((Join-Path $badServer 'game\SE1\bin\server.dll'),'not retail original')
    Reject $badServer
    $duplicate=Make-Copy
    [IO.File]::WriteAllText((Join-Path $duplicate 'game\SE1\bin\client.dll'),$syntheticClient)
    Reject $duplicate
    $missing=Make-Copy
    Remove-Item -LiteralPath (Join-Path $missing 'game\SE1\bin\sin_independent_saved_client.dll')
    Reject $missing
    # The fixture never starts or loads a game executable. These paths
    # are all synthetic original-free disk trees under this test's TEMP.
    Write-Output 'PASS retained owner test clone: authentic SHA guard, rollback retry, restoration, cleanup and fail-closed mutations'
} finally {
    foreach($child in $children) {
        if (Test-Path -LiteralPath $child) {
            Remove-Item -LiteralPath $child -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    Remove-Item -LiteralPath $seed -Recurse -Force -ErrorAction SilentlyContinue
}
