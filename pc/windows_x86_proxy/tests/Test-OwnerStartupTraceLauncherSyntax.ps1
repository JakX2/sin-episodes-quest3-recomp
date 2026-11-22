param(
  [Parameter(Mandatory=$true)][string]$Launcher
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not (Test-Path -LiteralPath $Launcher -PathType Leaf)) {
  throw "Missing owner-only test-copy launcher"
}
$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile(
  (Resolve-Path -LiteralPath $Launcher).ProviderPath,
  [ref]$tokens, [ref]$parseErrors
)
if ($null -ne $parseErrors -and $parseErrors.Count -gt 0) {
  throw ("PowerShell launcher syntax failed: " + $parseErrors[0].Message)
}
Write-Output "PASS: owner SiN startup trace launcher PowerShell syntax"
$source = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $Launcher).ProviderPath)
$required = @(
  '[switch]$GruntBridgeProbe',
  'SIN_PC_GRUNT_BRIDGE_PROBE',
  'OWNER_GRUNT_BRIDGE_PE32',
  'OWNER_GRUNT_BRIDGE: ABI_V1_LOADED_IN_LAUNCHED_PROCESS',
  'OWNER_GRUNT_BRIDGE: NOT_PROVEN',
  'sin-pc-proxy-{0}.log'
)
foreach ($witness in $required) {
  if (-not $source.Contains($witness)) {
    throw ("Missing Phase616 owner-only Grunt runtime preflight witness: " + $witness)
  }
}
Write-Output "PASS: Phase616 owner-only Grunt bridge flag, PE32 gate and PID-specific verdict are present"
