<#
Owner-only reversible game-copy staging for original SiN Episodes 2006.
Check never changes files. Install and Restore require explicit confirmation.
No owner DLL/asset bytes are copied to the research repository.
#>
[CmdletBinding()]
param(
  [ValidateSet('Check','Install','Restore')][string]$Action='Check',
  [Parameter(Mandatory=$true)][string]$GameBin,
  [string]$ProxyBin,
  [switch]$DisposableTestCopyConfirmed
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
function Get-LocalSha256([string]$path) {
  $hash=[System.Security.Cryptography.SHA256]::Create()
  $stream=[System.IO.File]::OpenRead($path)
  try {
    return [System.BitConverter]::ToString($hash.ComputeHash($stream)).Replace('-','').ToLowerInvariant()
  } finally {
    $stream.Dispose()
    $hash.Dispose()
  }
}
$Originals=@(
  @{Name='server.dll';Sibling='sin_original_server.dll';Hash='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'},
  @{Name='client.dll';Sibling='sin_original_client.dll';Hash='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'}
)
function Require-Original([string]$path,[string]$digest) {
  if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing owner original: $path" }
  $hash=(Get-LocalSha256 $path)
  if ($hash -ne $digest) { throw "Original 2006 SiN DLL SHA256 mismatch: $path" }
}
function Require-PE32([string]$path) {
  $stream=[System.IO.File]::OpenRead($path)
  $reader=New-Object System.IO.BinaryReader($stream)
  try {
    if ($reader.ReadUInt16() -ne 0x5a4d) { throw "Not MZ: $path" }
    [void]$stream.Seek(0x3c,[System.IO.SeekOrigin]::Begin)
    $pe=$reader.ReadUInt32()
    if ($pe -lt 0x40 -or $pe -gt ($stream.Length-32)) { throw "Invalid PE offset: $path" }
    [void]$stream.Seek($pe,[System.IO.SeekOrigin]::Begin)
    if ($reader.ReadUInt32() -ne 0x4550) { throw "Invalid PE signature: $path" }
    if ($reader.ReadUInt16() -ne 0x014c) { throw "Not Intel Win32 x86: $path" }
  } finally { $reader.Dispose();$stream.Dispose() }
}
if (!(Test-Path -LiteralPath $GameBin -PathType Container)) {
  throw "GameBin must be an existing owner-created game copy: $GameBin"
}
$dir=(Resolve-Path -LiteralPath $GameBin).ProviderPath
if ($Action -ne 'Check' -and !$DisposableTestCopyConfirmed) {
  throw "Refusing to modify files without -DisposableTestCopyConfirmed. Use an independent, fully backed-up test copy, never the retail installation."
}
if ($Action -eq 'Install') {
  if ([string]::IsNullOrWhiteSpace($ProxyBin) -or !(Test-Path -LiteralPath $ProxyBin -PathType Container)) {
    throw "Install requires -ProxyBin pointing to the built Win32 proxy directory"
  }
  $proxyDir=(Resolve-Path -LiteralPath $ProxyBin).ProviderPath
  if ([string]::Equals($proxyDir.TrimEnd('\'),$dir.TrimEnd('\'),[System.StringComparison]::OrdinalIgnoreCase)) {
    throw "ProxyBin and GameBin must be different directories"
  }
  $plan=@()
  foreach($spec in $Originals) {
    $live=Join-Path $dir $spec.Name
    $sibling=Join-Path $dir $spec.Sibling
    $built=Join-Path $proxyDir $spec.Name
    if (Test-Path -LiteralPath $sibling) { throw "Already staged, refusing duplicate install: $sibling" }
    Require-Original $live $spec.Hash
    if (!(Test-Path -LiteralPath $built -PathType Leaf)) { throw "Missing built proxy: $built" }
    Require-PE32 $built
    $digest=(Get-LocalSha256 $built)
    if ($digest -eq $spec.Hash) { throw "Built proxy must not be the owner-original binary: $built" }
    $plan+=@{Live=$live;Sibling=$sibling;Built=$built;Hash=$digest}
  }
  $moved=@()
  try {
    foreach($part in $plan) {
      Move-Item -LiteralPath $part.Live -Destination $part.Sibling
      $moved+=$part
    }
    foreach($part in $plan) {
      Copy-Item -LiteralPath $part.Built -Destination $part.Live
      $got=(Get-LocalSha256 $part.Live)
      if ($got -ne $part.Hash) { throw "Proxy file copy did not preserve SHA256" }
    }
  } catch {
    $err=$_
    foreach($part in $moved) {
      if (Test-Path -LiteralPath $part.Live) {
        Remove-Item -LiteralPath $part.Live -Force
      }
      if (Test-Path -LiteralPath $part.Sibling) {
        Move-Item -LiteralPath $part.Sibling -Destination $part.Live
      }
    }
    throw "Install failed; rollback attempted: $err"
  }
  Write-Output "STAGED: owner originals moved to sibling names; independently built x86 proxies installed."
  Write-Output "No original game executable was launched. Keep independent backups."
  return
}
if ($Action -eq 'Restore') {
  foreach($spec in $Originals) {
    $live=Join-Path $dir $spec.Name
    $sibling=Join-Path $dir $spec.Sibling
    Require-Original $sibling $spec.Hash
    if (!(Test-Path -LiteralPath $live -PathType Leaf)) { throw "Missing installed proxy: $live" }
    $hash=(Get-LocalSha256 $live)
    if ($hash -eq $spec.Hash) { Write-Output "ALREADY RESTORED: $live";continue }
    $pending="$live.sin-restore-pending"
    $backup="$live.sin-proxy-pre-restore"
    if ((Test-Path -LiteralPath $pending) -or (Test-Path -LiteralPath $backup)) {
      throw "Restore files already present, inspect manually: $live"
    }
    Copy-Item -LiteralPath $sibling -Destination $pending
    try {
      [System.IO.File]::Replace($pending,$live,$backup)
      Require-Original $live $spec.Hash
    } finally {
      if (Test-Path -LiteralPath $pending) { Remove-Item -LiteralPath $pending -Force }
    }
    Write-Output "RESTORED: $live (former proxy retained at $backup)"
  }
  Write-Output "Renamed owner originals retained as separate recoverable copies."
  return
}
foreach($spec in $Originals) {
  $live=Join-Path $dir $spec.Name
  $sibling=Join-Path $dir $spec.Sibling
  if (Test-Path -LiteralPath $sibling -PathType Leaf) {
    Require-Original $sibling $spec.Hash
    if (!(Test-Path -LiteralPath $live -PathType Leaf)) { throw "Staged original exists but current proxy is missing: $live" }
    Write-Output "STAGED: $($spec.Name) has exact original in $sibling"
  } else {
    Require-Original $live $spec.Hash
    Write-Output "ORIGINAL: $($spec.Name) SHA256 verified"
  }
}
Write-Output "CHECK passed. No files changed."
