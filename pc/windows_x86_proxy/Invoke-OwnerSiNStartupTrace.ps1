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
  [switch]$DisposableTestCopyConfirmed,
  [switch]$GruntBridgeProbe
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

# The Phase615 bridge is optional. Explicit -GruntBridgeProbe verifies an
# independent PE32 x86 DLL in this already staged, disposable test copy.
# No guesswork about engine-owned NPC instances or 2006 C++ vtable layouts.
if ($GruntBridgeProbe) {
  $bridge = Join-Path $bin 'sin_grunt_behavior_c_abi.dll'
  if (-not (Test-Path -LiteralPath $bridge -PathType Leaf)) {
    throw "Missing Grunt behavior C ABI DLL in the staged game-copy bin: $bridge"
  }
  $item = Get-Item -LiteralPath $bridge -Force
  if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
    throw "Grunt bridge must be a real file, not a symlink/reparse point"
  }
  $stream = [System.IO.File]::OpenRead($bridge)
  $reader = New-Object System.IO.BinaryReader($stream)
  try {
    if ($stream.Length -lt 0x100 -or $reader.ReadUInt16() -ne 0x5a4d) {
      throw "Grunt bridge is not an MZ Windows module"
    }
    [void]$stream.Seek(0x3c, [System.IO.SeekOrigin]::Begin)
    $pe = $reader.ReadUInt32()
    if ($pe -lt 0x40 -or $pe -gt ($stream.Length - 32)) {
      throw "Grunt bridge PE header position invalid"
    }
    [void]$stream.Seek($pe, [System.IO.SeekOrigin]::Begin)
    if ($reader.ReadUInt32() -ne 0x4550 -or $reader.ReadUInt16() -ne 0x014c) {
      throw "Grunt bridge must be an Intel x86 PE32, not x64 or Android"
    }
    [void]$stream.Seek($pe + 22, [System.IO.SeekOrigin]::Begin)
    if (($reader.ReadUInt16() -band 0x2000) -eq 0) {
      throw "Grunt bridge lacks Windows DLL image characteristics"
    }
    if ($reader.ReadUInt16() -ne 0x010b) {
      throw "Grunt bridge optional header is not PE32"
    }
  } finally {
    $reader.Dispose()
    $stream.Dispose()
  }
  Write-Output "OWNER_GRUNT_BRIDGE_PE32: verified x86 DLL in disposable game-copy bin"
}

$launchedAfter = [DateTime]::UtcNow.AddSeconds(-2)
$previous = [Environment]::GetEnvironmentVariable('SIN_PC_PROXY_TRACE_FILE', 'Process')
$previousGrunt = [Environment]::GetEnvironmentVariable('SIN_PC_GRUNT_BRIDGE_PROBE', 'Process')
$bridgeSetting = '0'
if ($GruntBridgeProbe) { $bridgeSetting = '1' }
try {
  [Environment]::SetEnvironmentVariable('SIN_PC_PROXY_TRACE_FILE', '1', 'Process')
  [Environment]::SetEnvironmentVariable('SIN_PC_GRUNT_BRIDGE_PROBE', $bridgeSetting, 'Process')
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
  Write-Output ("OWNER_START_PID: {0}" -f $game.Id)
  Write-Output ("Game launcher exit code: {0}" -f $game.ExitCode)
} finally {
  [Environment]::SetEnvironmentVariable('SIN_PC_PROXY_TRACE_FILE', $previous, 'Process')
  [Environment]::SetEnvironmentVariable('SIN_PC_GRUNT_BRIDGE_PROBE', $previousGrunt, 'Process')
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
if ($GruntBridgeProbe) {
  # Only the actual launched process PID can prove an in-process bridge load.
  # Other recent log files might belong to an unrelated process.
  $specific = Join-Path $folder ("sin-pc-proxy-{0}.log" -f $game.Id)
  $observed = $false
  if (Test-Path -LiteralPath $specific -PathType Leaf) {
    $info = Get-Item -LiteralPath $specific
    if ($info.LastWriteTimeUtc -ge $launchedAfter) {
      $matched = @(Get-Content -LiteralPath $specific |
        Where-Object { $_.Contains('role=server phase=grunt-bridge-ready') })
      $observed = $matched.Count -gt 0
      $events = @(Get-Content -LiteralPath $specific |
        Where-Object { $_.Contains('role=server phase=grunt-bridge-') })
      foreach ($event in $events) { Write-Output ("OWNER_GRUNT_EVENT: {0}" -f $event) }
    }
  }
  if ($observed) {
    Write-Output "OWNER_GRUNT_BRIDGE: ABI_V1_LOADED_IN_LAUNCHED_PROCESS"
  } else {
    Write-Warning "OWNER_GRUNT_BRIDGE: NOT_PROVEN for the exact launched PID. Inspect the trace file and any child-process logs; do not infer a successful engine load."
  }
}
Write-Output "The logs stay on this PC. No original DLLs or pseudocode were uploaded."
