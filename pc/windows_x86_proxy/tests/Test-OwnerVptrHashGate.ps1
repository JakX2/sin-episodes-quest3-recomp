<#
Negative regression: a synthetic fake may never be passed to the owner-original
vptr path. Require the exact distinct authentication-failure exit code before
LoadLibrary or any CreateInterface use. No retail assets are needed.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$Probe,
  [Parameter(Mandatory=$true)][string]$FakeDll,
  [ValidateSet('server','client')][string]$Role = 'server'
)
$ErrorActionPreference='Stop'
# Windows PowerShell 5.1 wraps a native process's expected stderr as a
# non-terminating NativeCommandError. Preserve the exact process exit code
# without turning that intentionally rejected fake into a script exception.
$previousPreference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
try {
  $output = & $Probe '--owner' $Role $FakeDll 2>&1 | Out-String
  $exit = $LASTEXITCODE
} finally {
  $ErrorActionPreference = $previousPreference
}
if ($exit -ne 2) { throw "Expected SHA gate exit code 2; got $exit. Output: $output" }
if ($output -notmatch 'full original DLL size/SHA256 authentication') {
  throw "Owner-original mismatch did not report authentication failure. Output: $output"
}
Write-Output "PASS: unrecognized synthetic module rejected before owner original LoadLibrary"
