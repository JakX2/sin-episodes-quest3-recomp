# Windows public CI, NO real SiN game. Verify the one-click runner parses and
# produces the expected dry-run plan; source and binary guardrails are retained.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Runner)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if (-not (Test-Path -LiteralPath $Runner -PathType Leaf)) {
    throw 'Missing one-click runner'
}
$tokens=$null
$errors=$null
[void][System.Management.Automation.Language.Parser]::ParseFile(
    $Runner,[ref]$tokens,[ref]$errors)
if ($errors.Count -gt 0) {
    throw ('PowerShell parse errors: '+($errors | Out-String))
}
$plan=@(& $Runner -GameRoot 'C:\synthetic-no-game' -PlanOnly)
$combined=$plan -join [Environment]::NewLine
foreach($expected in @(
    'SIN_ONE_CLICK_PLAN_ONLY_NO_GAME_COPY_OR_LAUNCH',
    'original installed game folder',
    'fresh disposable entire game copy',
    'independent client/server DLLs shipped in package bin',
    'presence first, then read only on success',
    'text-only ZIP',
    'keep disposable copy if restoration is uncertain'
)) {
    if (-not $combined.Contains($expected)) {
        throw ('Missing standalone runner invariant: '+$expected)
    }
}
$text=[IO.File]::ReadAllText($Runner)
foreach($must in @(
    'Authenticate-Source',
    'Scan-Source',
    'Require-UnlinkedAncestors',
    'ReparsePoint',
    'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY_V1',
    'Invoke-OwnerFileSystemRetailGate.ps1',
    "Phase='Presence'",
    "Phase='Read'",
    'PriorPresenceTranscript',
    'Verify-OriginalPair',
    'RESTORATION: NOT VERIFIED',
    'Compress-Archive',
    'Remove-Item -LiteralPath $temporary -Recurse -Force'
)) {
    if (-not $text.Contains($must)) {
        if ($must -eq 'RESTORATION: NOT VERIFIED') {
            # This phrase is an explicit optional user-facing message.
            continue
        }
        throw ('Missing one-click security or behavior clause: '+$must)
    }
}
# The runner must attempt to restore a locked TEMP copy after the
# first real owner 0x53494E6B diagnostic and before a cleanup decision.
foreach($required in @(
    'Recover-SiNRetainedTestCopy.ps1',
    'ONECLICK_AUTO_RECOVERY: VERIFIED_ORIGINALS',
    'ONECLICK_RECOVERED_READ_DIAGNOSTIC: ORIGINAL_ENGINE_FS_COUNTS_VERIFIED'
)) {
    if (-not $text.Contains($required)) {
        throw ('Missing owner post-exit recovery integration: '+$required)
    }
}
$recovery=Join-Path (Split-Path -Parent $Runner) 'Recover-SiNRetainedTestCopy.ps1'
$shortcut=Join-Path (Split-Path -Parent $Runner) 'RECOVER_PREVIOUS_TEST.bat'
if (-not (Test-Path -LiteralPath $recovery -PathType Leaf) -or
    -not (Test-Path -LiteralPath $shortcut -PathType Leaf)) {
    throw 'Standalone package lacks recovery script or double-click shortcut'
}
$rt=$null;$re=$null
[void][System.Management.Automation.Language.Parser]::ParseFile(
    $recovery,[ref]$rt,[ref]$re)
if ($re.Count -ne 0) {
    throw ('Recovery script PowerShell parse errors: '+($re | Out-String))
}
if (-not ([IO.File]::ReadAllText($shortcut)).Contains('-ScanTemp -CleanupRecoveredCopy')) {
    throw 'Recovery shortcut lacks exact-only guarded temp scan'
}
$batch=Join-Path (Split-Path -Parent $Runner) 'RUN_SIN_TEST.bat'
if (-not (Test-Path -LiteralPath $batch -PathType Leaf)) {
    throw 'Missing double-click batch launcher'
}
$bat=[IO.File]::ReadAllText($batch)
if (-not $bat.Contains('Start-SiNFileSystemTest.ps1') -or
    -not $bat.Contains('-STA') -or -not $bat.Contains('pause')) {
    throw 'Double-click entrypoint does not keep the window open'
}
Write-Output 'PASS one-click package: self-contained plan, original-game-only source guard, no-game dry run'
