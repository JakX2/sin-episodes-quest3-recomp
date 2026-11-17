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
    '$moduleInspectionSucceeded'
)) {
    if (-not $launcherText.Contains($required)) {
        throw "Missing owner module-observation contract: $required"
    }
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

    Write-Output "PASS owner independent Windows PowerShell launcher syntax, module-observation contract and 4 non-destructive rejection gates"
} finally {
    Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction SilentlyContinue
}
