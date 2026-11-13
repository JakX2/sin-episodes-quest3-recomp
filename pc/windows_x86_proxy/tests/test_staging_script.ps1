<#
Phase203 CI regression for the owner-only PowerShell staging script.
This MUST NOT install anything: CI only has synthetic fake game originals.
Check parse and fail-closed behavior, preserve every original fake/proxy byte.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ScriptPath,
    [Parameter(Mandatory=$true)][string]$GameBin
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$tokens=$null
$errors=$null
[void][System.Management.Automation.Language.Parser]::ParseFile(
    $ScriptPath,[ref]$tokens,[ref]$errors)
if ($errors.Count -ne 0) {
    foreach ($errorRecord in $errors) { Write-Error $errorRecord.Message }
    throw "PowerShell stage script has syntax errors"
}
$names=@('server.dll','client.dll','sin_original_server.dll','sin_original_client.dll')
$before=@{}
foreach ($name in $names) {
    $path=Join-Path $GameBin $name
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Synthetic CI prerequisite missing: $path"
    }
    $before[$name]=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
}
foreach ($action in @('Check','Install','Restore')) {
    $rejected=$false
    try {
        if ($action -eq 'Install') {
            & $ScriptPath -GameBin $GameBin -Action Install -ProxyBin $GameBin | Out-Null
        } else {
            & $ScriptPath -GameBin $GameBin -Action $action | Out-Null
        }
    } catch {
        $rejected=$true
    }
    if (!$rejected) {
        throw "Unsafe synthetic CI install/test incorrectly succeeded: $action"
    }
}
foreach ($name in $names) {
    $path=Join-Path $GameBin $name
    $after=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
    if ($after -ne $before[$name]) { throw "Staging script modified a synthetic binary: $name" }
}
Write-Output "PASS: staging script parses, rejects synthetic originals and unauthorized mutations, and never changes CI DLLs"
