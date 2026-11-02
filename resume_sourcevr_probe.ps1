#requires -Version 5.1
<#
Continue a private SourceVR build that failed AFTER Apktool decoded files
but BEFORE the APP_MOD_LIB Smali patch. No original APK is modified.
Run without arguments for read-only resume validation; pass -Build to finish.
#>
[CmdletBinding()]
param(
    [switch]$Build,
    [string]$SourceDir = (Join-Path $env:USERPROFILE 'Desktop\source vr'),
    [string]$Probe = (Join-Path $env:USERPROFILE 'Desktop\sinprobe\libclient.so'),
    [string]$BuildOutput = (Join-Path $env:USERPROFILE 'Desktop\sinprobe-build'),
    [string]$ToolRoot = (Join-Path $env:USERPROFILE 'Desktop\sourcevr-build-tools')
)
$ErrorActionPreference = 'Stop'
$builder = Join-Path $PSScriptRoot 'tools\sourcevr_isolated_apk_builder.py'
$jdk = Join-Path $ToolRoot 'jdk'
$sdk = Join-Path $ToolRoot 'android-sdk'
$apktool = Join-Path $ToolRoot 'apktool\apktool.bat'
$zipalign = Join-Path $sdk 'build-tools\35.0.0\zipalign.exe'
$apksigner = Join-Path $sdk 'build-tools\35.0.0\apksigner.bat'
$aapt = Join-Path $sdk 'build-tools\35.0.0\aapt.exe'
$keytool = Join-Path $jdk 'bin\keytool.exe'
$env:JAVA_HOME = $jdk
$env:ANDROID_HOME = $sdk
$env:ANDROID_SDK_ROOT = $sdk
$env:PATH = (Join-Path $jdk 'bin') + ';' + (Join-Path $ToolRoot 'apktool') +
    ';' + (Join-Path $sdk 'build-tools\35.0.0') + ';' + $env:PATH
foreach ($path in @($builder, $SourceDir, $Probe, $BuildOutput,
                     $apktool, $zipalign, $apksigner, $aapt, $keytool)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing expected path: $path" }
}
$argsForBuilder = @('--extracted-dir', $SourceDir, '--probe', $Probe,
    '--output', $BuildOutput, '--apktool', $apktool,
    '--zipalign', $zipalign, '--apksigner', $apksigner,
    '--aapt', $aapt, '--keytool', $keytool, '--resume')
if ($Build) {
    Write-Host '[BUILD] Resume from already-decoded private SourceVR files.'
    $argsForBuilder += '--build'
} else {
    Write-Host '[CHECK] Read-only inspection of the previous failed build.'
}
& py $builder @argsForBuilder
if ($LASTEXITCODE -ne 0) { throw 'SourceVR resume check/build failed. Share the output.' }
if ($Build) {
    Write-Host '[SUCCESS] Separate debug APK built (not installed).' -ForegroundColor Green
} else {
    Write-Host '[READY] Resume preflight passed. Re-run with -Build.' -ForegroundColor Green
}
