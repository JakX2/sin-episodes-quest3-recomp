<#
Runs an owner-provided SiN Windows executable from a *disposable test copy*.
Checks both proxy/original DLL pairs before launch and enables file-backed
CreateInterface traces for the child process. Never mutates game files,
never launches the original retail Steam folder intentionally, and never
calls original virtual methods from this script.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$GameBin,
  [Parameter(Mandatory=$true)][string]$GameExecutable,
  [string]$GameArguments = '',
  [switch]$DisposableTestCopyConfirmed
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (-not $DisposableTestCopyConfirmed) {
  throw "Refusing to launch without -DisposableTestCopyConfirmed. Provide a backed-up, disposable game installation copy."
}
if (-not (Test-Path -LiteralPath $GameBin -PathType Container)) {
  throw "GameBin must be an existing game-copy bin directory"
}
if (-not (Test-Path -LiteralPath $GameExecutable -PathType Leaf)) {
  throw "GameExecutable must be an existing .exe file"
}

$bin = (Resolve-Path -LiteralPath $GameBin).ProviderPath.TrimEnd('\')
$exe = (Resolve-Path -LiteralPath $GameExecutable).ProviderPath
$root = [System.IO.Path]::GetFullPath((Join-Path $bin '..')).TrimEnd('\')
$prefix = $root + [System.IO.Path]::DirectorySeparatorChar
if (-not $exe.EndsWith('.exe', [System.StringComparison]::OrdinalIgnoreCase) -or
    -not $exe.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "The executable must be a .exe within the same disposable game-copy root as GameBin"
}

$stager = Join-Path $PSScriptRoot 'Prepare-SiNWin32ProxyTest.ps1'
$check = @(& $stager -Action Check -GameBin $bin)
$staged = @($check | Where-Object { $_ -like 'STAGED:*' })
if ($staged.Count -ne 2) {
  throw "Expected BOTH x86 proxy modules to be staged alongside SHA256-verified originals. Run the documented Install action first."
}

$launchedAfter = [DateTime]::UtcNow.AddSeconds(-2)
$previous = [Environment]::GetEnvironmentVariable('SIN_PC_PROXY_TRACE_FILE', 'Process')
try {
  [Environment]::SetEnvironmentVariable('SIN_PC_PROXY_TRACE_FILE', '1', 'Process')
  $options = @{
    FilePath = $exe
    WorkingDirectory = [System.IO.Path]::GetDirectoryName($exe)
    PassThru = $true
    Wait = $true
  }
  if (-not [string]::IsNullOrWhiteSpace($GameArguments)) {
    $options['ArgumentList'] = $GameArguments
  }
  Write-Output "Launching owner test copy with factory tracing enabled; no DLL files modified by this launcher."
  $game = Start-Process @options
  Write-Output ("Game launcher exit code: {0}" -f $game.ExitCode)
} finally {
  [Environment]::SetEnvironmentVariable('SIN_PC_PROXY_TRACE_FILE', $previous, 'Process')
}

$folder = [System.IO.Path]::GetTempPath()
$logs = @(Get-ChildItem -LiteralPath $folder -Filter 'sin-pc-proxy-*.log' -File |
  Where-Object { $_.LastWriteTimeUtc -ge $launchedAfter } |
  Sort-Object LastWriteTimeUtc -Descending)
if ($logs.Count -eq 0) {
  Write-Warning "No factory trace file was produced during this launch. The game may have exited before loading the proxies or switched to another process."
} else {
  foreach ($log in $logs) {
    Write-Output ("OWNER_TRACE: {0}" -f $log.FullName)
  }
}
Write-Output "The logs stay on this PC. No original DLLs or pseudocode were uploaded."
