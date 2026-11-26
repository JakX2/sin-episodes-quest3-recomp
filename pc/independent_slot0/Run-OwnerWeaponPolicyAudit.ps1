[CmdletBinding()]
param(
    [string]$OriginalServer,
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'
$native = Join-Path $PSScriptRoot 'server.dll'
$log = Join-Path $PSScriptRoot 'weapon-policy-audit.log'
$expected = '568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
$tests = @('sin_server_npc_weapon_ammo_audit.exe', 'sin_server_npc_weapon_melee_audit.exe')
foreach ($file in @($native) + @($tests | ForEach-Object { Join-Path $PSScriptRoot $_ })) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing bundle file: $file" }
}
if (-not $SelfTest) {
    if (-not $OriginalServer) {
        $OriginalServer = Read-Host 'Enter the path to the original SiN Episodes SE1\bin\server.dll (do not use the bundled DLL)'
    }
    $OriginalServer = $OriginalServer.Trim().Trim('"')
    $OriginalServer = (Resolve-Path -LiteralPath $OriginalServer).ProviderPath
    $hash = (Get-FileHash -LiteralPath $OriginalServer -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($hash -ne $expected) { throw 'The selected original server.dll does not match the authenticated build. No tests ran.' }
}
$mode = if ($SelfTest) { 'native only' } else { 'native + original PE32' }
"SiN weapon policy audit: $mode" | Set-Content -LiteralPath $log
if ($SelfTest) { $arguments = @('--self-test', $native) } else { $arguments = @($native, $OriginalServer) }
$failed = 0
foreach ($name in $tests) {
    $exe = Join-Path $PSScriptRoot $name
    "=== $name ===" | Tee-Object -FilePath $log -Append
    $ErrorActionPreference = 'Continue'
    & $exe @arguments 2>&1 | ForEach-Object { "$_" } | Tee-Object -FilePath $log -Append
    $code = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    "Exit code: $code" | Tee-Object -FilePath $log -Append
    if ($code -ne 0) { $failed = 1 }
}
"Result: $(if ($failed) { 'FAIL' } else { 'PASS' })" | Tee-Object -FilePath $log -Append
exit $failed
