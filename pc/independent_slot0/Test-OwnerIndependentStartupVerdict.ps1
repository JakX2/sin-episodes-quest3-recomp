# Windows PowerShell synthetic-only trace classification tests.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Classifier)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$tokens=$null; $issues=$null
[void][System.Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path -LiteralPath $Classifier).ProviderPath,[ref]$tokens,[ref]$issues)
if ($issues.Count) { throw "Invalid classifier PowerShell syntax" }
$dir = Join-Path ([System.IO.Path]::GetTempPath()) ("sin-verdict-test-" + [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $dir -Force)
$session = '0123456789abcdef0123456789abcdef'
$other = 'ffffffffffffffffffffffffffffffff'
function Verify([string]$Expected) {
    $out = @(& $Classifier -Session $session -LogDirectory $dir)
    if (-not ($out -contains ("OWNER_INDEPENDENT_VERDICT: " + $Expected))) {
        throw ("Unexpected startup verdict: " + ($out -join ' | '))
    }
    return $out
}
try {
    [void](Verify 'NO_INDEPENDENT_INIT_EVIDENCE')
    # A previous session must NEVER become evidence for this one.
    [IO.File]::WriteAllText((Join-Path $dir "sin-independent-$other-101.log"),
        "[SiN independent] role=client phase=missing-interface version=VEngineClient012" + [Environment]::NewLine)
    [void](Verify 'NO_INDEPENDENT_INIT_EVIDENCE')
    # A real game can reach exported CreateInterface and reject our proposed
    # version without ever calling client Init / server DLLInit.
    $client = Join-Path $dir "sin-independent-$session-202.log"
    [IO.File]::WriteAllText($client,(
        "[SiN independent] role=client phase=factory-rejected version=VClient012" + [Environment]::NewLine))
    $out = Verify 'GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE'
    if (-not ($out -contains 'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: client:factory-rejected:VClient012')) {
        throw "Rejected game-module factory request not preserved"
    }
    if (-not ($out -contains 'OWNER_INDEPENDENT_ROLES_OBSERVED: client')) {
        throw "Factory entry was not attributed to client.dll"
    }
    [IO.File]::WriteAllText($client,(
        "[SiN independent] role=client phase=factory-accepted version=VClient011" + [Environment]::NewLine))
    $out = Verify 'GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE'
    if (-not ($out -contains 'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: client:factory-accepted:VClient011')) {
        throw "Accepted factory request falsely classified as Init"
    }
    [IO.File]::WriteAllText($client,(
        "[SiN independent] role=client phase=factory-accepted version=VClient011" + [Environment]::NewLine +
        "[SiN independent] role=client phase=request version=VEngineClient012" + [Environment]::NewLine +
        "[SiN independent] role=client phase=found-interface version=VEngineClient012" + [Environment]::NewLine +
        "[SiN independent] role=client phase=request version=VEngineModel012" + [Environment]::NewLine +
        "[SiN independent] role=client phase=missing-interface version=VEngineModel012" + [Environment]::NewLine))
    $out = Verify 'ENGINE_ENTERED_INDEPENDENT_INIT_BUT_REJECTED'
    if (-not ($out -contains 'OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: client:missing-interface:VEngineModel012')) {
        throw "Wrong client first blocker"
    }
    if (-not ($out -contains 'OWNER_INDEPENDENT_ROLES_OBSERVED: client')) {
        throw "Wrong observed role"
    }
    $server = Join-Path $dir "sin-independent-$session-303.log"
    [IO.File]::WriteAllText($server,(
        "[SiN independent] role=server phase=request version=VEngineServer021" + [Environment]::NewLine +
        "[SiN independent] role=server phase=missing-interface version=VEngineServer021" + [Environment]::NewLine))
    $out = Verify 'ENGINE_ENTERED_INDEPENDENT_INIT_BUT_REJECTED'
    if (-not ($out -contains 'OWNER_INDEPENDENT_TRACE_FILES: 2')) { throw "Missing second log" }
    if (-not ($out -contains 'OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: AMBIGUOUS_ACROSS_PROCESSES')) { throw "Cross-process chronology incorrectly inferred" }
    if (-not (($out | Where-Object { $_ -like 'OWNER_INDEPENDENT_BLOCKERS_BY_LOG: *' }) -match 'client:missing-interface:VEngineModel012')) { throw "Client blocker omitted" }
    if (-not (($out | Where-Object { $_ -like 'OWNER_INDEPENDENT_BLOCKERS_BY_LOG: *' }) -match 'server:missing-interface:VEngineServer021')) { throw "Server blocker omitted" }
    [IO.File]::WriteAllText($client,
        "[SiN independent] role=client phase=sdk2004-preflight-only version=<none>" + [Environment]::NewLine)
    [IO.File]::WriteAllText($server,
        "[SiN independent] role=server phase=sdk2004-preflight-only version=<none>" + [Environment]::NewLine)
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    Write-Output "PASS owner session classifier: cross-session, game-module factory entry, native Init blocker, two roles, no false retail boot"
} finally {
    Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
}
