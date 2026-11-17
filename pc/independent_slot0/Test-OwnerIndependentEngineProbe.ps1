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
    [System.IO.File]::WriteAllText((Join-Path $bin 'client.dll'),'synthetic invalid owner DLL')
    [System.IO.File]::WriteAllText((Join-Path $bin 'server.dll'),'synthetic invalid owner DLL')
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

    Write-Output "PASS owner independent Windows PowerShell launcher syntax, module sampling and 5 non-destructive safety rejection gates"
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
