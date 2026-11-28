# Windows owner module-census regression: no retail executable or commercial DLL.
# Verifies x64 PowerShell -> x86 module snapshot through the EXACT reader used
# by the reversible independent-client launcher.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Launcher
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not [Environment]::Is64BitProcess) {
    throw 'Test requires the x64 PowerShell host that previously missed x86 game modules'
}
$launcherPath = (Resolve-Path -LiteralPath $Launcher -ErrorAction Stop).ProviderPath
$tokens = $null
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile($launcherPath, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count -gt 0) { throw ('Launcher syntax: ' + $parseErrors[0].Message) }

# Invoke the existing launcher only along an intentional preflight-refusal
# branch. It loads its own Toolhelp type but MUST reject before file IO,
# launching a retail process, or moving any DLL. No fake game is required.
$refused = $false
try {
    & $launcherPath -GameBin 'SIN_NONEXISTENT' -GameExecutable 'SIN_NONEXISTENT' -IndependentBin 'SIN_NONEXISTENT' | Out-Null
} catch {
    if ($_.Exception.Message -notlike '*DisposableTestCopyConfirmed*') { throw }
    $refused = $true
}
if (-not $refused) { throw 'Launcher did not refuse the synthetic preflight' }
if (-not ('SinOwnerNativeModuleReader' -as [type])) {
    throw 'The actual owner launcher did not load its native Toolhelp helper'
}

$windowsRoot = [Environment]::GetFolderPath('Windows')
if ([string]::IsNullOrWhiteSpace($windowsRoot)) { $windowsRoot = $env:windir }
$x86cmd = Join-Path $windowsRoot 'SysWOW64\cmd.exe'
if (-not (Test-Path -LiteralPath $x86cmd -PathType Leaf)) {
    throw ('Missing 32-bit Windows command shell: ' + $x86cmd)
}
$child = $null
try {
    # The only process started is the native Windows 32-bit command shell.
    # ping provides a bounded ~12-second window, without custom binaries.
    $child = Start-Process -FilePath $x86cmd -ArgumentList '/c ping 127.0.0.1 -n 13' -PassThru -WindowStyle Hidden
    $sawCmd = $false
    $sawKernel = $false
    $maxCount = 0
    $firstError = $null
    for ($i = 0; $i -lt 25; $i++) {
        $child.Refresh()
        if ($child.HasExited) { break }
        $snap = [SinOwnerNativeModuleReader]::Read($child.Id)
        $maxCount = [Math]::Max($maxCount, $snap.Modules.Count)
        if ($snap.Error -ne 0 -and $null -eq $firstError) { $firstError = $snap.Error }
        foreach ($entry in $snap.Modules) {
            if ($entry.Name -ieq 'cmd.exe' -and [string]::Equals($entry.FullPath, $x86cmd, [System.StringComparison]::OrdinalIgnoreCase)) {
                $sawCmd = $true
            }
            if ($entry.Name -ieq 'kernel32.dll' -and $entry.FullPath -like '*\SysWOW64\kernel32.dll') {
                $sawKernel = $true
            }
        }
        if ($sawCmd -and $sawKernel) { break }
        Start-Sleep -Milliseconds 150
    }
    if (-not $sawCmd -or -not $sawKernel) {
        throw ('Native Toolhelp cross-bitness probe failed: cmd={0} kernel32={1} max_count={2} first_error={3}' -f $sawCmd,$sawKernel,$maxCount,$firstError)
    }
    Write-Output ('PASS x64 owner launcher native Toolhelp observes x86 cmd.exe, SysWOW64 kernel32.dll; modules={0}' -f $maxCount)
} finally {
    # Clean up only the test-owned synthetic child. Never touch any game PID.
    if ($null -ne $child) {
        $child.Refresh()
        if (-not $child.HasExited) { Stop-Process -Id $child.Id -Force -ErrorAction SilentlyContinue }
        $child.Dispose()
    }
}
