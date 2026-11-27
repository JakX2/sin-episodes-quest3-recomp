# Phase 1655: owner-only original 2006 PE32 client/server.dll factory ABI probe.
# This script and adjacent EXE contain NO original game binaries.
# Live mode loads the original modules (DllMain/static constructors execute).
# Requires explicit confirmation that files reside in an owner disposable copy.
[CmdletBinding()]
param(
    [switch]$SelfTest,
    [string]$ClientDll,
    [string]$ServerDll,
    [switch]$OwnerDisposableCopyConfirmed
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-OriginalFactoryProbeArguments {
    param(
        [string]$ClientPath,
        [string]$ServerPath,
        [bool]$Confirmed
    )
    if (-not $Confirmed -or
        [string]::IsNullOrWhiteSpace($ClientPath) -or
        [string]::IsNullOrWhiteSpace($ServerPath)) { return $false }
    try {
        $clientFull = [IO.Path]::GetFullPath($ClientPath)
        $serverFull = [IO.Path]::GetFullPath($ServerPath)
    }
    catch {
        return $false
    }
    if ($clientFull -ieq $serverFull) { return $false }
    if ([IO.Path]::GetFileName($clientFull) -ine 'client.dll' -or
        [IO.Path]::GetFileName($serverFull) -ine 'server.dll') { return $false }
    if ([IO.Path]::GetFileName([IO.Path]::GetDirectoryName($clientFull)) -ine 'bin' -or
        [IO.Path]::GetFileName([IO.Path]::GetDirectoryName($serverFull)) -ine 'bin') {
        return $false
    }
    return $true
}

if ($SelfTest) {
    if ($OwnerDisposableCopyConfirmed -or
        -not [string]::IsNullOrWhiteSpace($ClientDll) -or
        -not [string]::IsNullOrWhiteSpace($ServerDll)) {
        throw 'Self-test must not have live owner-original paths or confirmation'
    }
    $client = 'C:\OriginalDisposableCopy\bin\client.dll'
    $server = 'C:\OriginalDisposableCopy\SE1\bin\server.dll'
    if (Test-OriginalFactoryProbeArguments $client $server $false) {
        throw 'Missing owner-disposable confirmation accepted'
    }
    if (-not (Test-OriginalFactoryProbeArguments $client $server $true)) {
        throw 'Valid disposable Windows PE32 path shape refused'
    }
    if (Test-OriginalFactoryProbeArguments $client $client $true) {
        throw 'Same client/server DLL alias accepted'
    }
    if (Test-OriginalFactoryProbeArguments 'C:\anything\bin\not-client.dll' $server $true) {
        throw 'Wrong client DLL basename accepted'
    }
    if (Test-OriginalFactoryProbeArguments $client 'C:\anything\bin\not-server.dll' $true) {
        throw 'Wrong server DLL basename accepted'
    }
    if (Test-OriginalFactoryProbeArguments 'C:\NotBin\client.dll' $server $true) {
        throw 'Untrusted non-bin path accepted'
    }
    if (Test-OriginalFactoryProbeArguments $client '' $true) {
        throw 'Missing original server DLL accepted'
    }
    Write-Output 'PASS original-free owner interface factory package guardrails: confirmation, exact DLL names, bin folders, distinct paths, no game access'
    exit 0
}

if (-not (Test-OriginalFactoryProbeArguments $ClientDll $ServerDll ([bool]$OwnerDisposableCopyConfirmed))) {
    throw 'Provide distinct owner-original client.dll and server.dll paths in bin folders AND -OwnerDisposableCopyConfirmed'
}

$clientPath = [IO.Path]::GetFullPath($ClientDll)
$serverPath = [IO.Path]::GetFullPath($ServerDll)
if (-not [IO.File]::Exists($clientPath) -or
    -not [IO.File]::Exists($serverPath)) {
    throw 'Original game DLL path not found. The package does not contain game binaries.'
}
$probeExe = Join-Path $PSScriptRoot 'sin_original_interfacereg_owner_probe.exe'
if (-not [IO.File]::Exists($probeExe)) {
    throw 'Owner original factory probe executable missing from this package'
}

# The native EXE additionally refuses to LoadLibraryEx until each whole
# original PE32 file has passed its exact independent SHA-256 authentication.
# Its read-only registry snapshots never invoke any vtable methods. DllMain
# and a few original trivial CreateInterface singleton thunks DO execute.
Write-Output 'ORIGINAL_FACTORY_PROBE_DISPOSABLE_COPY_CONFIRMED=true'
& $probeExe $clientPath $serverPath
$code = $LASTEXITCODE
if ($code -ne 0) {
    throw "Owner-original factory probe refused or failed (exit $code)"
}
Write-Output 'ORIGINAL_FACTORY_PROBE_COMPLETED=true'
