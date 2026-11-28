# PowerShell native Windows fail-closed owner probe tests.
# No real game executable or original DLLs, no game process launched.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Launcher,
    [Parameter(Mandatory=$true)][string]$IndependentBin
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$tokens = $null
$issues = $null
[void][System.Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path -LiteralPath $Launcher).ProviderPath,
    [ref]$tokens, [ref]$issues
)
if ($issues.Count -gt 0) {
    throw ("Launcher script is not valid Windows PowerShell: " + $issues[0].Message)
}
# Contract checks for owner-only retail module-load evidence. These
# assertions do not claim a retail executable was run in public CI.
$launcherText = [System.IO.File]::ReadAllText(
    (Resolve-Path -LiteralPath $Launcher).ProviderPath)
foreach ($required in @(
    '$child.Modules',
    'OWNER_ENGINE_MODULE_OBSERVED:',
    'OWNER_ENGINE_MODULE_NOT_OBSERVED:',
    'OWNER_ENGINE_MODULE_INSPECTION:',
    'OrdinalIgnoreCase',
    '$moduleInspectionSucceeded',
    '$moduleInspectionSamples',
    '$moduleInspectionFirstError',
    '$child.WaitForExit(50)',
    'Require-NoReparseAncestors $GameBin',
    'Require-NoReparseAncestors $GameExecutable',
    'Require-NoReparseAncestors $IndependentBin'
)) {
    if (-not $launcherText.Contains($required)) {
        throw "Missing owner module-observation contract: $required"
    }
}
# Fast-failing game processes must get at least one attempted module
# snapshot; the original HasExited loop guard skipped them entirely.
if (-not $launcherText.Contains('do {') -or
    -not $launcherText.Contains('} while ($true)') -or
    $launcherText.Contains('while (-not $child.HasExited')) {
    throw 'Owner launcher may skip first module inspection on early game exit'
}
if (-not $launcherText.Contains('samples={2} first_error={3}')) {
    throw 'Owner launcher must report sample count and first inspection error'
}
if ($launcherText.Contains('OWNER_ENGINE_MODULE_MISSING:')) {
    throw 'Module polling must not infer absence from an unobserved sample'
}
# The actual owner's first retail test exited its tracked PID with zero
# while Windows still locked client.dll. The cleanup exception used to hide
# session traces and verdicts; the original backup was correctly retained.
# Guard the fail-closed recovery report and diagnostic tail of the launcher.
foreach ($required in @(
    'RESTORE_PENDING:',
    'OWNER_INDEPENDENT_RESTORATION:',
    'INCOMPLETE_MANUAL_RECOVERY_REQUIRED',
    'Saved original backup SHA256 mismatch'
)) {
    if (-not $launcherText.Contains($required)) {
        throw "Missing locked-DLL recovery contract: $required"
    }
}
$recoveryIndex = $launcherText.IndexOf('RESTORE_PENDING:')
$traceIndex = $launcherText.IndexOf('OWNER_INDEPENDENT_TRACE:')
$verdictIndex = $launcherText.IndexOf("Get-OwnerIndependentStartupVerdict.ps1")
if ($recoveryIndex -lt 0 -or $traceIndex -le $recoveryIndex -or
    $verdictIndex -le $traceIndex) {
    throw 'Locked-DLL recovery must retain later session traces and verdict'
}

function Sha256([string]$Path) {
    $hasher = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        return [System.BitConverter]::ToString($hasher.ComputeHash($stream)).Replace('-','').ToLowerInvariant()
    } finally {
        $stream.Dispose()
        $hasher.Dispose()
    }
}
$root = Join-Path ([System.IO.Path]::GetTempPath()) ("sin-independent-guard-" + [Guid]::NewGuid().ToString("N"))
$bin = Join-Path $root 'bin'
$exe = Join-Path $root 'fake-owned-test.exe'
$marker = Join-Path $root 'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY.txt'
New-Item -ItemType Directory -Path $bin -Force | Out-Null
try {
    [System.IO.File]::WriteAllText($exe,'This is NOT an actual game executable.')
    [System.IO.File]::WriteAllText((Join-Path $bin 'client.dll'),'synthetic invalid original CLIENT DLL')
    [System.IO.File]::WriteAllText((Join-Path $bin 'server.dll'),'synthetic invalid original SERVER DLL')
    $server = Join-Path $bin 'server.dll'
    $client = Join-Path $bin 'client.dll'
    $beforeServer = (Sha256 $server)
    $beforeClient = (Sha256 $client)

    function Reject([string]$Expected,[bool]$Confirm) {
        $args = @{
            GameBin = $bin
            GameExecutable = $exe
            IndependentBin = $IndependentBin
            GameArguments = '-console'
        }
        if ($Confirm) { $args.DisposableTestCopyConfirmed = $true }
        $rejected = $false
        try {
            & $Launcher @args | Out-Null
        } catch {
            if (-not $_.Exception.Message.Contains($Expected)) {
                throw "Unexpected fail-closed error: $($_.Exception.Message); expected: $Expected"
            }
            $rejected = $true
        }
        if (-not $rejected) {
            throw "Owner-only probe did not reject test case: $Expected"
        }
        if ((Sha256 $server) -ne $beforeServer -or
            (Sha256 $client) -ne $beforeClient) {
            throw "Fail-closed probe modified fake retail DLLs!"
        }
        foreach ($role in @('server.dll','client.dll')) {
            if (Test-Path -LiteralPath (Join-Path $bin ("sin_independent_saved_" + $role))) {
                throw "Fail-closed probe moved a fake original before verification"
            }
        }
    }

    Reject 'DisposableTestCopyConfirmed' $false
    Reject 'Missing file' $true
    [System.IO.File]::WriteAllText($marker,'INVALID_TEST_MARKER')
    Reject 'marker content mismatch' $true
    [System.IO.File]::WriteAllText($marker,'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY_V1')
    Reject 'SHA256 mismatch' $true

    # Phase1687: launching a real engine beyond the client Init
    # prefix is a separate, explicit Client-only diagnostic.
    # These invalid role/sound combinations must fail BEFORE
    # any SHA, replacement-file move or real game process launch.
    foreach ($case in @(
        @{Role='Both'; Sound=$false},
        @{Role='Server'; Sound=$false},
        @{Role='Client'; Sound=$true}
    )) {
        $inputs=@{
            GameBin=$bin
            GameExecutable=$exe
            IndependentBin=$IndependentBin
            ProbeRole=$case.Role
            EnableClientPredictionFactoryProbe=$true
            DisposableTestCopyConfirmed=$true
        }
        if ($case.Sound) { $inputs.EnableClientSoundProbe=$true }
        $rejected=$false
        try {
            & $Launcher @inputs | Out-Null
        } catch {
            if ($_.Exception.Message -notlike
                '*EnableClientPredictionFactoryProbe requires explicit Client-only mode without EnableClientSoundProbe*') {
                throw ('Unexpected prediction factory guard result: '+$_.Exception.Message)
            }
            $rejected=$true
        }
        if (-not $rejected) {
            throw 'Prediction owner factory diagnostic accepted unsafe role or simultaneous terminal sound probe'
        }
        if ((Sha256 $server) -cne $beforeServer -or
            (Sha256 $client) -cne $beforeClient) {
            throw 'Prediction factory role validation modified fake original DLLs'
        }
    }
    # Even when an opt-in is absent, the launcher must overwrite
    # inherited values with an explicit false and restore them
    # during the finally block. The test probes source text because
    # the fixture will not launch the owner's real game executable.
    foreach ($required in @(
        '$previousClientPredictionFactoryProbe',
        "'SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE'",
        'OWNER_CLIENT_PREDICTION_FACTORY_PROBE:',
        'ENABLED_OWNER_DIAGNOSTIC_NEXT_INTERFACE'
    )) {
        if (-not $launcherText.Contains($required)) {
            throw "Prediction owner probe environment isolation missing: $required"
        }
    }

    # Phase 517: only an explicitly selected Server-only diagnostic may
    # touch the second factory's 2006 physics names.
    foreach ($case in @(
        @{Role='Both'; Factory=$true},
        @{Role='Client'; Factory=$true},
        @{Role='Server'; Factory=$false}
    )) {
        $args=@{GameBin=$bin; GameExecutable=$exe;
            IndependentBin=$IndependentBin; ProbeRole=$case.Role;
            EnableServerPhysicsFactoryProbe=$true;
            DisposableTestCopyConfirmed=$true}
        if ($case.Factory) { $args.EnableServerFactoryProbe=$true }
        $blocked=$false
        try { & $Launcher @args | Out-Null } catch {
            if ($_.Exception.Message -notlike '*EnableServerPhysicsFactoryProbe requires Server-only role and EnableServerFactoryProbe*') {
                throw ('Unexpected server physics role guard failure: '+$_.Exception.Message)
            }
            $blocked=$true
        }
        if(-not $blocked) {throw 'Server physics stage accepted without explicit role and factory opt-in'}
        if((Sha256 $server) -ne $beforeServer -or
           (Sha256 $client) -ne $beforeClient) {
            throw 'Server physics invalid role test modified original fake DLLs'
        }
    }


    # Phase520: server-side resource stage must NEVER be enabled as
    # a client/both-role probe or without all original physics factory gates.
    foreach ($case in @(
        @{Role='Both'; Factory=$true; Physics=$true},
        @{Role='Client'; Factory=$true; Physics=$true},
        @{Role='Server'; Factory=$false; Physics=$true},
        @{Role='Server'; Factory=$true; Physics=$false}
    )) {
        $args=@{GameBin=$bin; GameExecutable=$exe;
            IndependentBin=$IndependentBin; ProbeRole=$case.Role;
            EnableServerPhysicsResourceProbe=$true;
            DisposableTestCopyConfirmed=$true}
        if ($case.Factory) { $args.EnableServerFactoryProbe=$true }
        if ($case.Physics) { $args.EnableServerPhysicsFactoryProbe=$true }
        $blocked=$false
        try { & $Launcher @args | Out-Null } catch {
            if ($_.Exception.Message -notlike
                '*EnableServerPhysicsResourceProbe requires Server-only, EnableServerFactoryProbe, and EnableServerPhysicsFactoryProbe*') {
                throw ('Unexpected server resource opt-in guard: '+$_.Exception.Message)
            }
            $blocked=$true
        }
        if (-not $blocked) { throw 'Server resource probe accepted missing prerequisites' }
        if ((Sha256 $server) -ne $beforeServer -or
            (Sha256 $client) -ne $beforeClient) {
            throw 'Invalid server resource probe modified original fake DLLs'
        }
    }

    # Phase522 client read-only field probe must never accept Server/Both
    # or bypass the earlier original surface resource read gates. The
    # generated test uses fake originals and must leave them untouched.
    foreach($case in @(
        @{Role='Both'; Surface=$true},
        @{Role='Server'; Surface=$true},
        @{Role='Client'; Surface=$false}
    )) {
        $args=@{
            GameBin=$bin;GameExecutable=$exe;IndependentBin=$IndependentBin;
            ProbeRole=$case.Role;EnableClientSurfacePhysicsDataProbe=$true;
            DisposableTestCopyConfirmed=$true
        }
        if($case.Surface){$args.EnableClientSurfacePropsReadProbe=$true}
        $blocked=$false
        try {& $Launcher @args | Out-Null} catch {
            if($_.Exception.Message -notlike '*EnableClientSurfacePhysicsDataProbe requires Client-only mode and EnableClientSurfacePropsReadProbe*') {
                throw ('Unexpected material read-only role guard: '+$_.Exception.Message)
            }
            $blocked=$true
        }
        if(-not $blocked){throw 'Original physics material probe accepted invalid role/opt-in'}
        if((Sha256 $server) -ne $beforeServer -or
           (Sha256 $client) -ne $beforeClient) {
            throw 'Invalid material read-only role modified fake retail DLLs'
        }
    }

    # Retail install layout: root/SinEpisodes.exe, root/bin/engine.dll,
    # root/SE1/GameInfo.txt and original gameplay DLLs in root/SE1/bin/.
    # The outer game root, NOT SE1/, must own the disposable marker.
    $se1Root = Join-Path $root 'SE1'
    $se1Bin = Join-Path $se1Root 'bin'
    [void](New-Item -ItemType Directory -Path $se1Bin -Force)
    [System.IO.File]::WriteAllText((Join-Path $se1Root 'GameInfo.txt'),'synthetic GameInfo only')
    [System.IO.File]::WriteAllText((Join-Path $se1Bin 'client.dll'),'synthetic SE1 client DLL')
    [System.IO.File]::WriteAllText((Join-Path $se1Bin 'server.dll'),'synthetic SE1 server DLL')
    $se1Client = Join-Path $se1Bin 'client.dll'
    $se1Server = Join-Path $se1Bin 'server.dll'
    $se1BeforeClient = Sha256 $se1Client
    $se1BeforeServer = Sha256 $se1Server
    $se1Exe = Join-Path $root 'SinEpisodes.exe'
    [System.IO.File]::WriteAllText($se1Exe,'This is NOT the real SiN executable.')
    $rejectedWrongExe = $false
    try {
        & $Launcher -GameBin $se1Bin -GameExecutable $exe `
            -IndependentBin $IndependentBin -DisposableTestCopyConfirmed | Out-Null
    } catch {
        if (-not $_.Exception.Message.Contains('requires SinEpisodes.exe')) {
            throw "Expected SE1 root executable guard, got $($_.Exception.Message)"
        }
        $rejectedWrongExe = $true
    }
    if (-not $rejectedWrongExe) { throw 'SE1 probe accepted a different root executable name' }
    $rejectedSe1 = $false
    try {
        & $Launcher -GameBin $se1Bin -GameExecutable $se1Exe `
            -IndependentBin $IndependentBin -DisposableTestCopyConfirmed | Out-Null
    } catch {
        if (-not $_.Exception.Message.Contains('SHA256 mismatch')) {
            throw "SE1 layout did not reach original DLL authentication: $($_.Exception.Message)"
        }
        $rejectedSe1 = $true
    }
    if (-not $rejectedSe1) { throw 'Synthetic SE1 layout unexpectedly passed SHA gate' }
    if ((Sha256 $se1Client) -ne $se1BeforeClient -or
        (Sha256 $se1Server) -ne $se1BeforeServer) {
        throw 'Synthetic SE1 test DLLs changed before SHA authentication'
    }
    foreach ($role in @('server.dll','client.dll')) {
        if (Test-Path -LiteralPath (Join-Path $se1Bin ("sin_independent_saved_" + $role))) {
            throw 'SE1 layout staged fake originals before SHA authentication'
        }
    }

    # A junction at the parent root is *not* visible in leaf-only attribute
    # checks. It must be rejected before original hashes or any Move-Item.
    # This exercises the bypass using synthetic, never commercial, files.
    $junction = Join-Path ([System.IO.Path]::GetTempPath()) ("sin-owner-linked-root-" + [Guid]::NewGuid().ToString("N"))
    try {
        New-Item -ItemType Junction -Path $junction -Target $root -ErrorAction Stop | Out-Null
        $linkedBin = Join-Path $junction 'bin'
        $linkedExe = Join-Path $junction 'fake-owned-test.exe'
        $rejected = $false
        try {
            & $Launcher -GameBin $linkedBin -GameExecutable $linkedExe `
                -IndependentBin $IndependentBin -DisposableTestCopyConfirmed | Out-Null
        } catch {
            if (-not $_.Exception.Message.Contains('Refusing reparse point ancestor')) {
                throw "Linked parent failed for the wrong reason: $($_.Exception.Message)"
            }
            $rejected = $true
        }
        if (-not $rejected) { throw 'Owner launcher accepted a linked disposable-game root' }
        if ((Sha256 $server) -ne $beforeServer -or
            (Sha256 $client) -ne $beforeClient) {
            throw 'Linked-root rejection modified the synthetic originals'
        }
        foreach ($role in @('server.dll','client.dll')) {
            if (Test-Path -LiteralPath (Join-Path $bin ("sin_independent_saved_" + $role))) {
                throw 'Linked-root rejection moved a synthetic original'
            }
        }
    } finally {
        # Delete only the junction itself, never recurse into its target.
        if (Test-Path -LiteralPath $junction) {
            [System.IO.Directory]::Delete($junction)
        }
    }

    # Nonlaunching test-only copy uses invented file hashes. The
    # production launcher has no override for its original SHA pins.
    $fixtureLauncher = Join-Path $root 'synthetic-stage-only.ps1'
    $fixtureText = $launcherText.Replace(
        '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650',$beforeServer).Replace(
        '6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d',$beforeClient)
    $launchAnchor = '$child = Start-Process @options'
    if (($fixtureText.Split([string[]]@($launchAnchor),[System.StringSplitOptions]::None)).Count -ne 2) {
        throw 'Cannot construct nonlaunching staging fixture'
    }
    $stageAssertions = @'
        $expectedNames = @()
        if ($ProbeRole -eq 'Both' -or $ProbeRole -eq 'Server') { $expectedNames += 'server.dll' }
        if ($ProbeRole -eq 'Both' -or $ProbeRole -eq 'Client') { $expectedNames += 'client.dll' }
        if ($staged.Count -ne $expectedNames.Count) { throw 'Wrong staged count' }
        foreach ($spec in $HashPins) {
            $live = Join-Path $bin $spec.Name
            $saved = Join-Path $bin ("sin_independent_saved_" + $spec.Name)
            if ($expectedNames -contains $spec.Name) {
                if (-not (Test-Path -LiteralPath $saved -PathType Leaf) -or
                    (Sha256 $saved) -cne $spec.Hash -or
                    (Sha256 $live) -cne (Sha256 (Join-Path $build $spec.Name))) {
                    throw "Wrong staged role or saved original: $($spec.Name)"
                }
            } else {
                if ((Test-Path -LiteralPath $saved) -or (Sha256 $live) -cne $spec.Hash) {
                    throw "Retained original was modified: $($spec.Name)"
                }
            }
        }
        throw 'SYNTHETIC_STAGE_ASSERTIONS_COMPLETE_NO_PROCESS_LAUNCHED'
'@
    $fixtureText = $fixtureText.Replace($launchAnchor,$stageAssertions)
    if ($fixtureText.Contains('Start-Process @options')) {
        throw 'Synthetic staging fixture must never launch a process'
    }
    [System.IO.File]::WriteAllText($fixtureLauncher,$fixtureText)
    Copy-Item -LiteralPath (Join-Path (Split-Path -Parent $Launcher) 'Get-OwnerIndependentStartupVerdict.ps1') -Destination $root
    foreach ($mode in @('Server','Client','Both')) {
        $testBuild = Join-Path $root ("build-" + $mode)
        [void](New-Item -ItemType Directory -Path $testBuild)
        if ($mode -eq 'Server' -or $mode -eq 'Both') {
            Copy-Item -LiteralPath (Join-Path $IndependentBin 'server.dll') -Destination $testBuild
        }
        if ($mode -eq 'Client' -or $mode -eq 'Both') {
            Copy-Item -LiteralPath (Join-Path $IndependentBin 'client.dll') -Destination $testBuild
        }
        $completed = $false
        $testArgs = @{GameBin=$bin; GameExecutable=$exe; IndependentBin=$testBuild; ProbeRole=$mode; DisposableTestCopyConfirmed=$true}
        try {
            & $fixtureLauncher @testArgs | Out-Null
        } catch {
            if (-not $_.Exception.Message.Contains('SYNTHETIC_STAGE_ASSERTIONS_COMPLETE_NO_PROCESS_LAUNCHED')) {
                throw "Role $mode synthetic staging failed: $($_.Exception.Message)"
            }
            $completed = $true
        }
        if (-not $completed -or (Sha256 $server) -cne $beforeServer -or
            (Sha256 $client) -cne $beforeClient) {
            throw "Role $mode did not restore both fake originals"
        }
        foreach ($name in @('server.dll','client.dll')) {
            if (Test-Path -LiteralPath (Join-Path $bin ("sin_independent_saved_" + $name))) {
                throw "Role $mode left a backup after synthetic restoration"
            }
        }
    }

    Write-Output "PASS owner independent launcher guardrails, role-isolated staging and both-original restoration (synthetic only)"
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
