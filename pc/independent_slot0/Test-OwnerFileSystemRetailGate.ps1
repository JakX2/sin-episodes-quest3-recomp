# Public CI exercises only owner-gate preflight and synthetic transcripts.
# No owner game files, no DLL staging, no game executable ever launched.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Gate)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest

$fakeGame='C:\SinDisposableSynthetic'
$fakeBuild='C:\SiNSyntheticIndependentBin'
$dir=Join-Path ([System.IO.Path]::GetTempPath()) ('sin-fs-gate-tests-'+[Guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $dir -Force)
try {
    function Reject([hashtable]$Arguments,[string]$Expected) {
        $rejected=$false
        try {
            & $Gate @Arguments | Out-Null
        } catch {
            if (-not $_.Exception.Message.Contains($Expected)) {
                throw ('Unexpected refusal: '+$_.Exception.Message+' ; wanted '+$Expected)
            }
            $rejected=$true
        }
        if (-not $rejected) {
            throw ('Required owner gate refusal missing: '+$Expected)
        }
    }
    $basic=@{DisposableGameRoot=$fakeGame;IndependentBin=$fakeBuild}
    Reject $basic 'DisposableTestCopyConfirmed'

    $plan=@(& $Gate @basic -PlanOnly)
    $text=$plan -join [Environment]::NewLine
    foreach($required in @(
        'OWNER_ENGINE_FS_GATE: PLAN_ONLY_NO_GAME_LAUNCHED',
        'OWNER_ENGINE_FS_GATE_PHASE: Presence',
        'EnableClientEngineFsProbe',
        'EnableClientSoundSystemInitProbe',
        'EnableClientVGuiControlsBootstrapProbe',
        'EnableClientMatSurfaceProbe',
        'ProbeRole',
        'DisposableTestCopyConfirmed'
    )) {
        if(-not $text.Contains($required)) {
            throw ('Missing planned presence flag: '+$required)
        }
    }
    if($text.Contains('EnableClientEngineFsReadProbe')) {
        throw 'Presence-only plan accidentally enables deep engine reads'
    }
    Reject (@{DisposableGameRoot=$fakeGame;IndependentBin=$fakeBuild;
              Phase='Read';PlanOnly=$true}) 'previous verified owner presence transcript'

    $session='0123456789abcdef0123456789abcdef'
    $presence=Join-Path $dir 'presence.txt'
    $lines=[string[]]@(
        'OWNER_INDEPENDENT_PROBE_ROLE: CLIENT',
        'OWNER_CLIENT_ENGINE_FS_PROBE: ENABLED_TERMINAL_DIAGNOSTIC',
        'OWNER_CLIENT_ENGINE_FS_READ_PROBE: DISABLED',
        ('OWNER_INDEPENDENT_SESSION: '+$session),
        'OWNER_ENGINE_EXIT_CODE: 1397313128',
        'OWNER_ENGINE_EXIT_HEX: 0x53494E68',
        'OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE',
        ('OWNER_CLIENT_ENGINE_FS_PRESENCE_RESULTS: sin-independent-'+$session+
         '-2222.log:client:both-present'),
        'OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_PRESENCE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    )
    [System.IO.File]::WriteAllLines($presence,$lines)
    $read=@{DisposableGameRoot=$fakeGame;IndependentBin=$fakeBuild;
            Phase='Read';PlanOnly=$true;PriorPresenceTranscript=$presence}
    $readText=@(& $Gate @read) -join [Environment]::NewLine
    foreach($required in @(
        'OWNER_ENGINE_FS_GATE: PLAN_ONLY_NO_GAME_LAUNCHED',
        'OWNER_ENGINE_FS_GATE_PHASE: Read',
        ('OWNER_ENGINE_FS_GATE_PREVIOUS_PRESENCE_SESSION: '+$session),
        'EnableClientEngineFsProbe',
        'EnableClientEngineFsReadProbe',
        'EnableClientSoundSystemInitProbe'
    )) {
        if(-not $readText.Contains($required)) {
            throw ('Missing read gate planned prerequisite: '+$required)
        }
    }
    # All of these modifications must block the read phase, even though the
    # text still contains plausible marker names and both-present strings.
    foreach($mutation in @(
        @('VERIFIED_COMPLETE','INCOMPLETE_MANUAL_RECOVERY_REQUIRED'),
        @('0x53494E68','0x53494E69'),
        @('1397313128','1397313129'),
        @('OWNER_CLIENT_ENGINE_FS_READ_PROBE: DISABLED',
          'OWNER_CLIENT_ENGINE_FS_READ_PROBE: ENABLED_TERMINAL_DIAGNOSTIC'),
        @('sin-independent-'+$session+'-2222.log',
          'sin-independent-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa-2222.log'),
        @('CLIENT_ENGINE_FS_PRESENCE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF',
          'CLIENT_SOUND_SYSTEM_INIT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'),
        @('OWNER_INDEPENDENT_PROBE_ROLE: CLIENT','OWNER_INDEPENDENT_PROBE_ROLE: SERVER')
    )) {
        $wrong=Join-Path $dir 'mutated.txt'
        [System.IO.File]::WriteAllText($wrong,(
            [System.IO.File]::ReadAllText($presence).Replace(
                [string]$mutation[0],[string]$mutation[1])
        ))
        $bad=@{DisposableGameRoot=$fakeGame;IndependentBin=$fakeBuild;
               Phase='Read';PlanOnly=$true;PriorPresenceTranscript=$wrong}
        Reject $bad 'Presence evidence lacks'
        Remove-Item -LiteralPath $wrong -Force
    }
    # Duplicate session IDs are not allowed, nor is stale earlier evidence
    # that belongs to an unrelated process.
    $duplicate=Join-Path $dir 'duplicate.txt'
    [System.IO.File]::WriteAllLines($duplicate,[string[]](@($lines)+
        @('OWNER_INDEPENDENT_SESSION: '+$session)))
    Reject (@{DisposableGameRoot=$fakeGame;IndependentBin=$fakeBuild;
              Phase='Read';PlanOnly=$true;PriorPresenceTranscript=$duplicate}) 'single exact session'

    Reject (@{DisposableGameRoot=$fakeGame;IndependentBin=$fakeBuild;
              Phase='Presence';PlanOnly=$true;PriorPresenceTranscript=$presence}) 'valid only for Read'
    Reject (@{DisposableGameRoot='C:\Steam\steamapps\common\SiN';
              IndependentBin=$fakeBuild;PlanOnly=$true}) 'Steam installation'
    Reject (@{DisposableGameRoot='not-absolute';
              IndependentBin=$fakeBuild;PlanOnly=$true}) 'absolute path'
    # No normal game launch can proceed on arbitrary fake paths because
    # the original launcher performs hashes, marker and path preflight.
    if(-not $text.Contains('PLAN_ONLY_NO_GAME_LAUNCHED') -or
       -not $readText.Contains('PLAN_ONLY_NO_GAME_LAUNCHED')) {
        throw 'Owner gate synthetic tests attempted to launch a game'
    }
    Write-Output 'PASS owner filesystem gate: presence first, read transcript, restoration, session isolation and no-game CI'
} finally {
    if(Test-Path -LiteralPath $dir) {
        Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
    }
}
