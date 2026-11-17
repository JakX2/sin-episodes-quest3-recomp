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
