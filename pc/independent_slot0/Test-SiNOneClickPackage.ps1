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
$stagedPlan=@(& $Runner -GameRoot 'C:\synthetic-no-game' -PlanOnly -StagedStats)
$hooksPlan=@(& $Runner -GameRoot 'C:\synthetic-no-game' -PlanOnly -StagedStats -HookProof)
$soundscapePlan=@(& $Runner -GameRoot 'C:\synthetic-no-game' -PlanOnly -StagedStats -HookProof -SoundscapeCatalog)
if (-not (($soundscapePlan -join '|').Contains(
    'SOUNDSCAPE PROOF: original rb/GAME manifest and rb/null script catalog; no audio or game Init'))) {
    throw 'Soundscape opt-in one-click must be dry-run only'
}

$cvarPlan=@(& $Runner -GameRoot 'C:\synthetic-no-game' -PlanOnly -StagedStats -HookProof -SoundscapeCatalog -SoundscapeCvarLookup)
if(-not (($cvarPlan -join '|').Contains(
    'SOUNDSCAPE CVAR LOOKUP: opt-in read-only VEngineCvar002 slot3 FindVar for two names, no writes or game Init'))) {
    throw 'Soundscape original ICvar lookup must be explicit opt-in and no-game dry run'
}
$physicsPlan=@(& $Runner -GameRoot 'C:\synthetic-no-game' -PlanOnly -StagedStats -HookProof -SoundscapeCatalog -SoundscapeCvarLookup -PhysicsFactoryLookup)
if(-not (($physicsPlan -join '|').Contains(
    'PHYSICS FACTORY LOOKUP: opt-in read-only 2006 VPhysics030/VPhysicsSurfaceProps001/VPhysicsCollision007; no coupling or game Init'))) {
    throw '2006 original physics three-query stage must be explicit and read-only'
}
$surfacePropsPlan=@(& $Runner -GameRoot 'C:\synthetic-no-game' -PlanOnly -StagedStats -HookProof -SoundscapeCatalog -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead)
if(-not (($surfacePropsPlan -join '|').Contains(
    'SURFACEPROPS READ: opt-in read-only original rb/GAME manifest and two rb/nullptr scripts, 93 definitions, no physics registration'))) {
    throw 'Original Source 2006 surfaceprops resource stage must be explicit and read-only'
}
$registerPlan=@(& $Runner -GameRoot 'C:\synthetic-no-game' -PlanOnly -StagedStats -HookProof -SoundscapeCatalog -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead -SurfacePropsRegister)
if(-not (($registerPlan -join '|').Contains(
    'SURFACEPROPS REGISTER: EXPLICIT ENGINE MUTATION. Calls original VPhysicsSurfaceProps001 slot1 with both resources in disposable copy; still NO game boot.'))) {
    throw 'Phase 515 engine-mutating ParseSurfaceData test must be opt-in even in PlanOnly'
}
foreach($unrequested in @($plan,$soundscapePlan,$cvarPlan,$physicsPlan,$surfacePropsPlan)) {
    if(($unrequested -join '|').Contains('SURFACEPROPS REGISTER:')) {
        throw 'Original physics engine mutation was enabled without explicit -SurfacePropsRegister'
    }
}
if(($soundscapePlan -join '|').Contains('SURFACEPROPS READ:') -or
   ($cvarPlan -join '|').Contains('SURFACEPROPS READ:') -or
   ($physicsPlan -join '|').Contains('SURFACEPROPS READ:')) {
    throw 'Physics surfaceprops resource reader accidentally enabled before opt-in'
}
if(($soundscapePlan -join '|').Contains('PHYSICS FACTORY LOOKUP:') -or
   ($cvarPlan -join '|').Contains('PHYSICS FACTORY LOOKUP:')) {
    throw 'Physics Factory probe unexpectedly enabled without explicit flag'
}
if(($soundscapePlan -join '|').Contains('SOUNDSCAPE CVAR LOOKUP:')) {
    throw 'Default soundscape diagnostic accidentally enabled original ICvar calls'
}
if (-not (($hooksPlan -join '|').Contains(
    'HOOK PROOF: three independently owned CUserMessages hook IDs 27:28:5, not dispatch'))) {
    throw 'Owned hook proof one-click plan must be distinct and have no game'
}
if (-not (($stagedPlan -join '|').Contains(
    'THIRD TEST: rb/GAME definitions and rt/null layout, independent 66-ID binding'))) {
    throw 'Staged stats one-click plan must be distinct from previous default'
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
    'SoundscapeCvarLookup',
    'ReadOnlySoundscapeCvarLookup',
    'PhysicsFactoryLookup',
    'ReadOnlyPhysicsFactoryProbe',
    'SurfacePropsRead',
    'ReadOnlySurfacePropsReadProbe',
    'SurfacePropsRegister',
    'ExplicitSurfacePropsRegisterProbe',
    'Original surfaceprops ParseSurfaceData (explicit engine mutation):',
    'Original surfaceprops manifest and scripts (read-only):',
    'Original soundscape ConVar lookup (read-only):',
    'Original physics Factory triple (read-only):',
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
foreach($required in @(
    'Phase=$(if($SoundscapeCatalog){''Soundscape''}elseif($HookProof){''Hooks''}else{''Staged''})',
    'PriorReadTranscript',
    'ONECLICK_RECOVERED_STAGED_DIAGNOSTIC: ORIGINAL_MODES_AND_INDEPENDENT_CATALOG_VERIFIED',
    'Original-mode staged 66-stat binding'
)) {
    if (-not $text.Contains($required)) {
        throw ('Missing one-click original staged resource milestone: '+$required)
    }
}
foreach($required in @(
    'HookProof',
    'Phase=$(if($SoundscapeCatalog){''Soundscape''}elseif($HookProof){''Hooks''}else{''Staged''})',
    'ONECLICK_RECOVERED_HOOK_DIAGNOSTIC: OWNED_27_28_5_VERIFIED',
    'Independent Stats CUserMessages hooks 27:28:5'
)) {
    if (-not $text.Contains($required)) {
        throw ('Missing hook proof gate or recovery invariant: '+$required)
    }
}
$hookBatch=Join-Path (Split-Path -Parent $Runner) 'RUN_STATS_HOOK_PROOF.bat'
if (-not (Test-Path -LiteralPath $hookBatch -PathType Leaf)) {
    throw 'Missing separate double-click hook proof BAT'
}
$hookText=[IO.File]::ReadAllText($hookBatch)
if (-not $hookText.Contains('Start-SiNFileSystemTest.ps1') -or
    -not $hookText.Contains('-StagedStats -HookProof') -or
    -not $hookText.Contains('-STA') -or
    -not $hookText.Contains('pause')) {
    throw 'Hook proof must be double-click, three-phase, independently packaged'
}
$soundscapeBatch=Join-Path (Split-Path -Parent $Runner) 'RUN_SOUNDSCAPE_CATALOG_TEST.bat'
$soundscapeDoc=Join-Path (Split-Path -Parent $Runner) 'OWNER_SOUNDSCAPE_RETAIL_GATE_20261120.md'
if (-not (Test-Path -LiteralPath $soundscapeBatch -PathType Leaf) -or
    -not (Test-Path -LiteralPath $soundscapeDoc -PathType Leaf)) {
    throw 'Missing separately packaged soundscape milestone and safety guide'
}
$soundscapeBatchText=[IO.File]::ReadAllText($soundscapeBatch)
if (-not $soundscapeBatchText.Contains('Start-SiNFileSystemTest.ps1') -or
    -not $soundscapeBatchText.Contains('-StagedStats -HookProof -SoundscapeCatalog') -or
    -not $soundscapeBatchText.Contains('-STA') -or
    -not $soundscapeBatchText.Contains('pause')) {
    throw 'Soundscape four-stage owner diagnostic must be opt-in and interactive'
}
foreach($marker in @(
    'SoundscapeCatalog requires -StagedStats -HookProof',
    'OWNER_ENGINE_FS_GATE_RESULT: SOUNDSCAPE_DIAGNOSTIC_ONLY_NO_GAME_BOOT',
    'ONECLICK_RECOVERED_SOUNDSCAPE_DIAGNOSTIC: ORIGINAL_7_301_242_3_VERIFIED',
    'OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS:',
    'Soundscape read/parse stage:',
    'OWNER_CLIENT_SOUNDSCAPE_DETAILS:',
    'OWNER_ENGINE_EXIT_HEX: 0x53494E76'
)){
    if (-not ([IO.File]::ReadAllText($Runner)).Contains($marker)) {
        throw ('Missing four-stage source/test evidence gate: '+$marker)
    }
}
$hookDoc=Join-Path (Split-Path -Parent $Runner) 'OWNER_STATS_HOOK_PROOF_20261120.md'
if (-not (Test-Path -LiteralPath $hookDoc -PathType Leaf)) {
    throw 'Missing authentic hook-proof documentation'
}
$stagedBatch=Join-Path (Split-Path -Parent $Runner) 'RUN_STAGED_STATS_TEST.bat'
if (-not (Test-Path -LiteralPath $stagedBatch -PathType Leaf)) {
    throw 'Missing staged stats double-click BAT'
}
$batText=[IO.File]::ReadAllText($stagedBatch)
if (-not $batText.Contains('Start-SiNFileSystemTest.ps1') -or
    -not $batText.Contains('-StagedStats') -or
    -not $batText.Contains('-STA') -or
    -not $batText.Contains('pause')) {
    throw 'Staged stats must launch a self-contained three-phase test'
}
$ownerDoc=Join-Path (Split-Path -Parent $Runner) 'OWNER_ORIGINAL_STAGED_STATS_RETAIL_GATE_20261120.md'
if (-not (Test-Path -LiteralPath $ownerDoc -PathType Leaf)) {
    throw 'Missing explanation of original staged modes and experimental limit'
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
