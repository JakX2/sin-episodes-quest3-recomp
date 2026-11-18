# Windows PowerShell synthetic-only trace classification tests.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Classifier)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$tokens=$null; $issues=$null
[void][System.Management.Automation.Language.Parser]::ParseFile(
    (Resolve-Path -LiteralPath $Classifier).ProviderPath,[ref]$tokens,[ref]$issues)
if ($issues.Count) { throw "Invalid classifier PowerShell syntax" }
$dir = Join-Path ([System.IO.Path]::GetTempPath()) ("sin-verdict-test-" + [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $dir -Force)
$session = '0123456789abcdef0123456789abcdef'
$other = 'ffffffffffffffffffffffffffffffff'
function Verify([string]$Expected) {
    $out = @(& $Classifier -Session $session -LogDirectory $dir)
    if (-not ($out -contains ("OWNER_INDEPENDENT_VERDICT: " + $Expected))) {
        throw ("Unexpected startup verdict: " + ($out -join ' | '))
    }
    return $out
}
try {
    [void](Verify 'NO_INDEPENDENT_INIT_EVIDENCE')
    # A previous session must NEVER become evidence for this one.
    [IO.File]::WriteAllText((Join-Path $dir "sin-independent-$other-101.log"),
        "[SiN independent] role=client phase=missing-interface version=VEngineClient012" + [Environment]::NewLine)
    [void](Verify 'NO_INDEPENDENT_INIT_EVIDENCE')
    # A real game can reach exported CreateInterface and reject our proposed
    # version without ever calling client Init / server DLLInit.
    $client = Join-Path $dir "sin-independent-$session-202.log"
    [IO.File]::WriteAllText($client,(
        "[SiN independent] role=client phase=factory-rejected version=VClient012" + [Environment]::NewLine))
    $out = Verify 'GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE'
    if (-not ($out -contains 'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: client:factory-rejected:VClient012')) {
        throw "Rejected game-module factory request not preserved"
    }
    if (-not ($out -contains 'OWNER_INDEPENDENT_ROLES_OBSERVED: client')) {
        throw "Factory entry was not attributed to client.dll"
    }
    [IO.File]::WriteAllText($client,(
        "[SiN independent] role=client phase=factory-accepted version=VClient011" + [Environment]::NewLine))
    $out = Verify 'GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE'
    if (-not ($out -contains 'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: client:factory-accepted:VClient011')) {
        throw "Accepted factory request falsely classified as Init"
    }
    # Synthetic replay of the owner's first real retail-engine trace.
    # These lines come from our independent diagnostic, not game binaries.
    # The rejected optional/unknown ServerGameEnts001 factory query did not
    # stop the engine from entering the client's virtual Init callback.
    $retailReplay = Join-Path $dir "sin-independent-$session-13124.log"
    $replayLines = @(
        '[SiN independent] role=server phase=factory-accepted version=ServerGameDLL003',
        '[SiN independent] role=server phase=factory-rejected version=ServerGameEnts001',
        '[SiN independent] role=client phase=factory-accepted version=VClient011',
        '[SiN independent] role=client phase=init-entered version=<none>',
        '[SiN independent] role=client phase=request version=VEngineClient012',
        '[SiN independent] role=client phase=found-interface version=VEngineClient012',
        '[SiN independent] role=client phase=request version=VEngineModel012',
        '[SiN independent] role=client phase=found-interface version=VEngineModel012',
        '[SiN independent] role=client phase=sdk2004-preflight-only version=<none>'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]$replayLines)
    Remove-Item -LiteralPath $client -Force
    $out = Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF'
    foreach ($expected in @(
        'OWNER_INDEPENDENT_TRACE_FILES: 1',
        'OWNER_INDEPENDENT_ROLES_OBSERVED: client,server',
        'OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: client',
        'OWNER_INDEPENDENT_FACTORY_REQUESTS: client:VEngineClient012,client:VEngineModel012',
        'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: server:factory-accepted:ServerGameDLL003,server:factory-rejected:ServerGameEnts001,client:factory-accepted:VClient011'
    )) {
        if (-not ($out -ccontains $expected)) {
            throw "Owner-first-startup replay unexpectedly changed: $expected"
        }
    }
    if (@($out | Where-Object { $_ -like '*BLOCKER*' }).Count -gt 0) {
        throw 'Unknown game-module version was falsely promoted to a fatal Init blocker'
    }
    # The second actual retail engine test used the same factory negotiation,
    # reached VClient011 Init, and found all five checked interface versions.
    # Replay it separately: no server DLLInit entry and no successful boot.
    $replayLines = @(
        '[SiN independent] role=server phase=factory-accepted version=ServerGameDLL003',
        '[SiN independent] role=server phase=factory-rejected version=ServerGameEnts001',
        '[SiN independent] role=client phase=factory-accepted version=VClient011',
        '[SiN independent] role=client phase=init-entered version=<none>',
        '[SiN independent] role=client phase=request version=VEngineClient012',
        '[SiN independent] role=client phase=found-interface version=VEngineClient012',
        '[SiN independent] role=client phase=request version=VEngineModel012',
        '[SiN independent] role=client phase=found-interface version=VEngineModel012',
        '[SiN independent] role=client phase=request version=VEngineEffects001',
        '[SiN independent] role=client phase=found-interface version=VEngineEffects001',
        '[SiN independent] role=client phase=request version=VEngineCvar002',
        '[SiN independent] role=client phase=found-interface version=VEngineCvar002',
        '[SiN independent] role=client phase=request version=EngineTraceClient003',
        '[SiN independent] role=client phase=found-interface version=EngineTraceClient003',
        '[SiN independent] role=client phase=sdk2004-preflight-only version=<none>'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]$replayLines)
    $out = Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF'
    foreach ($expected in @(
        'OWNER_INDEPENDENT_TRACE_FILES: 1',
        'OWNER_INDEPENDENT_ROLES_OBSERVED: client,server',
        'OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: client',
        'OWNER_INDEPENDENT_FACTORY_REQUESTS: client:VEngineClient012,client:VEngineModel012,client:VEngineEffects001,client:VEngineCvar002,client:EngineTraceClient003',
        'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: server:factory-accepted:ServerGameDLL003,server:factory-rejected:ServerGameEnts001,client:factory-accepted:VClient011'
    )) {
        if (-not ($out -ccontains $expected)) {
            throw "Owner-five-interface replay unexpectedly changed: $expected"
        }
    }
    if (@($out | Where-Object { $_ -like '*BLOCKER*' }).Count -gt 0) {
        throw 'Five-interface preflight should not produce an inferred fatal blocker'
    }
    # Replay the owner's real server-only experiment. The original client
    # was retained and unchanged; no server DLLInit event was seen.
    [IO.File]::WriteAllLines($retailReplay,[string[]]@(
        '[SiN independent] role=server phase=factory-accepted version=ServerGameDLL003',
        '[SiN independent] role=server phase=factory-rejected version=ServerGameEnts001'
    ))
    $out = Verify 'GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE'
    foreach ($expected in @(
        'OWNER_INDEPENDENT_ROLES_OBSERVED: server',
        'OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: ',
        'OWNER_INDEPENDENT_FACTORY_REQUESTS: ',
        'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: server:factory-accepted:ServerGameDLL003,server:factory-rejected:ServerGameEnts001'
    )) {
        if (-not ($out -ccontains $expected)) { throw "Server-only replay changed: $expected" }
    }
    # Hypothetical next-probe trace, deliberately not claimed retail evidence.
    [IO.File]::WriteAllLines($retailReplay,[string[]]@(
        '[SiN independent] role=server phase=factory-accepted version=ServerGameDLL003',
        '[SiN independent] role=server phase=factory-accepted version=ServerGameEnts001',
        '[SiN independent] role=server phase=unsupported-callback version=ServerGameEnts001.CheckTransmit'
    ))
    $out = Verify 'UNSUPPORTED_CALLBACK_DIAGNOSTIC_STOP_NO_BOOT_PROOF'
    if (-not ($out -ccontains 'OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: ') -or
        -not (($out -join '|').Contains(':server:ServerGameEnts001.CheckTransmit'))) {
        throw 'Pre-Init terminal callback falsely attributed to Init or lost'
    }
    # Actual Phase 325 owner observation: all three server factories
    # accepted; HLTV rejected; slot 11 reached BEFORE any DLLInit.
    [IO.File]::WriteAllLines($retailReplay,[string[]]@(
        '[SiN independent] role=server phase=factory-accepted version=ServerGameDLL003',
        '[SiN independent] role=server phase=factory-accepted version=ServerGameEnts001',
        '[SiN independent] role=server phase=factory-accepted version=ServerGameClients003',
        '[SiN independent] role=server phase=factory-rejected version=HLTVDirector001',
        '[SiN independent] role=server phase=unsupported-callback version=ServerGameDLL003.slot11'
    ))
    $out = Verify 'UNSUPPORTED_CALLBACK_DIAGNOSTIC_STOP_NO_BOOT_PROOF'
    foreach ($expected in @(
        'OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: ',
        'OWNER_INDEPENDENT_FACTORY_REQUESTS: ',
        'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: server:factory-accepted:ServerGameDLL003,server:factory-accepted:ServerGameEnts001,server:factory-accepted:ServerGameClients003,server:factory-rejected:HLTVDirector001'
    )) {
        if (-not ($out -ccontains $expected)) { throw "Phase 325 replay changed: $expected" }
    }
    if (-not (($out -join '|').Contains(':server:ServerGameDLL003.slot11'))) {
        throw 'Actual slot 11 stop omitted'
    }
    # Exact Phase 328 owner trace: real server Init and all 16 acquisitions.
    $retail328 = @(
        '[SiN independent] role=server phase=factory-accepted version=ServerGameDLL003',
        '[SiN independent] role=server phase=factory-accepted version=ServerGameEnts001',
        '[SiN independent] role=server phase=factory-accepted version=ServerGameClients003',
        '[SiN independent] role=server phase=factory-rejected version=HLTVDirector001',
        '[SiN independent] role=server phase=diagnostic-callback version=ServerGameDLL003.GetGameDescription',
        '[SiN independent] role=server phase=init-entered version=<none>',
        '[SiN independent] role=server phase=request version=VEngineServer021',
        '[SiN independent] role=server phase=found-interface version=VEngineServer021',
        '[SiN independent] role=server phase=request version=VoiceServer001',
        '[SiN independent] role=server phase=found-interface version=VoiceServer001',
        '[SiN independent] role=server phase=request version=VEngineCvar002',
        '[SiN independent] role=server phase=found-interface version=VEngineCvar002',
        '[SiN independent] role=server phase=request version=VEngineServerStringTable001',
        '[SiN independent] role=server phase=found-interface version=VEngineServerStringTable001',
        '[SiN independent] role=server phase=request version=StaticPropMgrServer002',
        '[SiN independent] role=server phase=found-interface version=StaticPropMgrServer002',
        '[SiN independent] role=server phase=request version=VEngineRandom001',
        '[SiN independent] role=server phase=found-interface version=VEngineRandom001',
        '[SiN independent] role=server phase=request version=IEngineSoundServer003',
        '[SiN independent] role=server phase=found-interface version=IEngineSoundServer003',
        '[SiN independent] role=server phase=request version=SpatialPartition001',
        '[SiN independent] role=server phase=found-interface version=SpatialPartition001',
        '[SiN independent] role=server phase=request version=VModelInfoServer001',
        '[SiN independent] role=server phase=found-interface version=VModelInfoServer001',
        '[SiN independent] role=server phase=request version=EngineTraceServer003',
        '[SiN independent] role=server phase=found-interface version=EngineTraceServer003',
        '[SiN independent] role=server phase=request version=VFileSystem012',
        '[SiN independent] role=server phase=found-interface version=VFileSystem012',
        '[SiN independent] role=server phase=request version=GAMEEVENTSMANAGER002',
        '[SiN independent] role=server phase=found-interface version=GAMEEVENTSMANAGER002',
        '[SiN independent] role=server phase=request version=VEngineCache001',
        '[SiN independent] role=server phase=found-interface version=VEngineCache001',
        '[SiN independent] role=server phase=request version=VSoundEmitter001',
        '[SiN independent] role=server phase=found-interface version=VSoundEmitter001',
        '[SiN independent] role=server phase=request version=ServerUploadGameStats001',
        '[SiN independent] role=server phase=found-interface version=ServerUploadGameStats001',
        '[SiN independent] role=server phase=request version=ISERVERPLUGINHELPERS001',
        '[SiN independent] role=server phase=found-interface version=ISERVERPLUGINHELPERS001',
        '[SiN independent] role=server phase=sdk2004-preflight-only version=<none>',
        '[SiN independent] role=server phase=factory-rejected version=PluginHelpersCheck001',
        '[SiN independent] role=server phase=diagnostic-callback version=ServerGameDLL003.GetGameDescription',
        '[SiN independent] role=server phase=unsupported-callback version=ServerGameDLL003.slot10'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]$retail328)
    $out = Verify 'UNSUPPORTED_CALLBACK_DIAGNOSTIC_STOP_NO_BOOT_PROOF'
    if (-not ($out -ccontains 'OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: server') -or
        -not ($out -ccontains 'OWNER_INDEPENDENT_FACTORY_REQUESTS: server:VEngineServer021,server:VoiceServer001,server:VEngineCvar002,server:VEngineServerStringTable001,server:StaticPropMgrServer002,server:VEngineRandom001,server:IEngineSoundServer003,server:SpatialPartition001,server:VModelInfoServer001,server:EngineTraceServer003,server:VFileSystem012,server:GAMEEVENTSMANAGER002,server:VEngineCache001,server:VSoundEmitter001,server:ServerUploadGameStats001,server:ISERVERPLUGINHELPERS001') -or
        -not (($out -join '|').Contains(':server:ServerGameDLL003.slot10'))) {
        throw 'Phase 328 real Init/dependency evidence changed'
    }
    # Hypothetical terminal sound result, NOT retail evidence. Test both
    # outcomes and require an explicit entry/result/terminal sequence.
    $soundPrefix = $retail328[0..37]
    foreach ($phase in @('sound-connect-succeeded','sound-connect-rejected')) {
        $soundTrace = @($soundPrefix) + @(
            '[SiN independent] role=server phase=sound-connect-entered version=VSoundEmitter001',
            ("[SiN independent] role=server phase={0} version=VSoundEmitter001" -f $phase),
            '[SiN independent] role=server phase=sound-probe-terminal-stop version=VSoundEmitter001'
        )
        [IO.File]::WriteAllLines($retailReplay,[string[]]$soundTrace)
        $out = Verify 'SOUND_CONNECT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if (-not (($out -join '|').Contains(":server:$phase"))) {
            throw 'Missing same-process sound outcome'
        }
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]]@(
        '[SiN independent] role=server phase=init-entered version=<none>',
        '[SiN independent] role=server phase=sound-probe-terminal-stop version=VSoundEmitter001'
    ))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Real Phase 330 owner trace: exactly the same 38 acquisition
    # events as Phase 328, followed by actual successful Connect.
    $retail330 = @($soundPrefix) + @(
        '[SiN independent] role=server phase=sound-connect-entered version=VSoundEmitter001',
        '[SiN independent] role=server phase=sound-connect-succeeded version=VSoundEmitter001',
        '[SiN independent] role=server phase=sound-probe-terminal-stop version=VSoundEmitter001'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]$retail330)
    $out = Verify 'SOUND_CONNECT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if (-not (($out -join '|').Contains(':server:sound-connect-succeeded'))) {
        throw 'Actual Phase 330 sound evidence missing'
    }
    # Hypothetical next boundary, not owner-observed. Preserve the
    # signed event result (including zero or negative), never infer boot.
    $eventPrefix = @($soundPrefix) + @(
        '[SiN independent] role=server phase=sound-connect-entered version=VSoundEmitter001',
        '[SiN independent] role=server phase=sound-connect-succeeded version=VSoundEmitter001',
        '[SiN independent] role=server phase=math-tables-initialized version=server-scalar',
        '[SiN independent] role=server phase=factory-state-cached version=server',
        '[SiN independent] role=server phase=event-load-entered version=resource/gameevents.res'
    )
    # Actual Phase 332 owner trace: the real engine event loader
    # returned 47 after sound success, scalar tables and factory caching.
    $retail332 = @($eventPrefix) + @(
        '[SiN independent] role=server phase=event-load-returned version=resource/gameevents.res:47',
        '[SiN independent] role=server phase=event-probe-terminal-stop version=resource/gameevents.res'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]$retail332)
    $out = Verify 'EVENT_LOAD_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $result = @($out | Where-Object { $_ -like 'OWNER_SERVER_EVENT_LOAD_RESULTS: *' })
    if ($result.Count -ne 1 -or -not $result[0].EndsWith(':47') -or
        -not ($out -ccontains 'OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: server')) {
        throw 'Actual Phase 332 event-load result missing'
    }
    # Actual Phase 337 owner evidence (registration verified, exit 0x53494E49).
    # Require entry/return, lookup verification and positive event result.
    $cvarPrefix = @($eventPrefix) + @(
        '[SiN independent] role=server phase=event-load-returned version=resource/gameevents.res:47'
    )
    # Phase 337's full positive sequence below is now owner-observed:
    # engine register and exact lookup identity completed successfully.
    $cvarTrace = @($cvarPrefix) + @(
        '[SiN independent] role=server phase=cvar-name-available version=dd_log_events',
        '[SiN independent] role=server phase=cvar-register-entered version=dd_log_events',
        '[SiN independent] role=server phase=cvar-register-returned version=dd_log_events',
        '[SiN independent] role=server phase=cvar-registration-verified version=dd_log_events',
        '[SiN independent] role=server phase=cvar-probe-result version=verified',
        '[SiN independent] role=server phase=cvar-probe-terminal-stop version=dd_log_events'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]$cvarTrace)
    $out = Verify 'CVAR_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $line = @($out | Where-Object { $_ -like 'OWNER_SERVER_CVAR_PROBE_RESULTS: *' })
    if ($line.Count -ne 1 -or -not $line[0].EndsWith(':verified')) {
        throw 'Missing verified single-variable result'
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($cvarPrefix) + @(
        '[SiN independent] role=server phase=cvar-probe-result version=existing-name',
        '[SiN independent] role=server phase=cvar-probe-terminal-stop version=dd_log_events'
    )))
    $out = Verify 'CVAR_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $line = @($out | Where-Object { $_ -like 'OWNER_SERVER_CVAR_PROBE_RESULTS: *' })
    if ($line.Count -ne 1 -or -not $line[0].EndsWith(':existing-name')) {
        throw 'Missing distinct single-variable refusal'
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($cvarPrefix) + @(
        '[SiN independent] role=server phase=cvar-probe-result version=verified',
        '[SiN independent] role=server phase=cvar-probe-terminal-stop version=dd_log_events'
    )))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Actual Phase 340 owner trace. All 24 register entry/return/verify
    # triplets and the final full-registry consistency result were observed.
    # Exit 0x53494E4D, original server restored, original client unchanged.
    $batchTrace = @($cvarPrefix) + @(
        '[SiN independent] role=server phase=cvar-batch-preflight-complete version=24'
    )
    foreach ($name in @(
        'g_debug_dynamic_difficulty',
        'dd_log_events',
        'g_debug_damage',
        'sk_dynamic_resupply_modifier',
        'sk_healthmega',
        'sk_flashgrenade_radius',
        'sk_dmg_homer_grenade',
        'sk_homer_grenade_radius',
        'sk_dmg_pathfollower_grenade',
        'sk_pathfollower_grenade_radius',
        'grunt_spawn_health',
        'ai_use_readiness',
        'ai_readiness_decay',
        'ai_new_aiming',
        'g_debug_bigmutant',
        'major_ambushdist',
        'g_debug_major_phys_hit',
        'minor_spit_arc',
        'g_debug_turret_ceiling',
        'g_debug_turret',
        'ai_newgroundturret',
        'ai_ally_manager_debug',
        'player_sprint_enabled',
        'accuracy_change_time'
    )) {
        foreach ($phase in @('cvar-batch-register-entered','cvar-batch-register-returned','cvar-batch-registration-verified')) {
            $batchTrace += ("[SiN independent] role=server phase={0} version={1}" -f $phase,$name)
        }
    }
    $batchTrace += @(
        '[SiN independent] role=server phase=cvar-batch-result version=verified:24/24',
        '[SiN independent] role=server phase=cvar-batch-terminal-stop version=24'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]$batchTrace)
    $out = Verify 'CVAR_BATCH_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if (-not (($out -join '|').Contains(':verified:24/24'))) { throw 'Actual Phase 340 batch result missing' }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($cvarPrefix) + @(
        '[SiN independent] role=server phase=cvar-batch-result version=verified:24/24',
        '[SiN independent] role=server phase=cvar-batch-terminal-stop version=24'
    )))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($cvarPrefix) + @(
        '[SiN independent] role=server phase=cvar-batch-result version=existing-name:0/24',
        '[SiN independent] role=server phase=cvar-batch-terminal-stop version=24'
    )))
    $out = Verify 'CVAR_BATCH_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if (-not (($out -join '|').Contains(':existing-name:0/24'))) { throw 'Batch refusal missing' }
    foreach ($count in @(42,0,-1)) {
        $eventTrace = @($eventPrefix) + @(
            ("[SiN independent] role=server phase=event-load-returned version=resource/gameevents.res:{0}" -f $count),
            '[SiN independent] role=server phase=event-probe-terminal-stop version=resource/gameevents.res'
        )
        [IO.File]::WriteAllLines($retailReplay,[string[]]$eventTrace)
        $out = Verify 'EVENT_LOAD_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        $result = @($out | Where-Object { $_ -like 'OWNER_SERVER_EVENT_LOAD_RESULTS: *' })
        if ($result.Count -ne 1 -or -not $result[0].EndsWith(":$count")) {
            throw 'Signed initial event result missing'
        }
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($soundPrefix) + @(
        '[SiN independent] role=server phase=event-load-returned version=resource/gameevents.res:42',
        '[SiN independent] role=server phase=event-probe-terminal-stop version=resource/gameevents.res'
    )))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Actual Phase 339 owner trace: all 21 acquisitions and real sound
    # success with exit 0x53494E4B and verified original restoration.
    $client339 = @(
        '[SiN independent] role=client phase=factory-accepted version=VClient011',
        '[SiN independent] role=client phase=init-entered version=<none>',
        '[SiN independent] role=client phase=request version=VEngineClient012',
        '[SiN independent] role=client phase=found-interface version=VEngineClient012',
        '[SiN independent] role=client phase=request version=VEngineModel012',
        '[SiN independent] role=client phase=found-interface version=VEngineModel012',
        '[SiN independent] role=client phase=request version=VEngineEffects001',
        '[SiN independent] role=client phase=found-interface version=VEngineEffects001',
        '[SiN independent] role=client phase=request version=VEngineCvar002',
        '[SiN independent] role=client phase=found-interface version=VEngineCvar002',
        '[SiN independent] role=client phase=request version=EngineTraceClient003',
        '[SiN independent] role=client phase=found-interface version=EngineTraceClient003',
        '[SiN independent] role=client phase=request version=VEngineRenderView011',
        '[SiN independent] role=client phase=found-interface version=VEngineRenderView011',
        '[SiN independent] role=client phase=request version=VDebugOverlay003',
        '[SiN independent] role=client phase=found-interface version=VDebugOverlay003',
        '[SiN independent] role=client phase=request version=VMaterialSystem076',
        '[SiN independent] role=client phase=found-interface version=VMaterialSystem076',
        '[SiN independent] role=client phase=request version=VEngineCache001',
        '[SiN independent] role=client phase=found-interface version=VEngineCache001',
        '[SiN independent] role=client phase=request version=VModelInfoClient003',
        '[SiN independent] role=client phase=found-interface version=VModelInfoClient003',
        '[SiN independent] role=client phase=request version=VEngineVGui001',
        '[SiN independent] role=client phase=found-interface version=VEngineVGui001',
        '[SiN independent] role=client phase=request version=VEngineClientStringTable001',
        '[SiN independent] role=client phase=found-interface version=VEngineClientStringTable001',
        '[SiN independent] role=client phase=request version=SpatialPartition001',
        '[SiN independent] role=client phase=found-interface version=SpatialPartition001',
        '[SiN independent] role=client phase=request version=VEngineShadowMgr002',
        '[SiN independent] role=client phase=found-interface version=VEngineShadowMgr002',
        '[SiN independent] role=client phase=request version=StaticPropMgrClient004',
        '[SiN independent] role=client phase=found-interface version=StaticPropMgrClient004',
        '[SiN independent] role=client phase=request version=IEngineSoundClient003',
        '[SiN independent] role=client phase=found-interface version=IEngineSoundClient003',
        '[SiN independent] role=client phase=request version=VFileSystem012',
        '[SiN independent] role=client phase=found-interface version=VFileSystem012',
        '[SiN independent] role=client phase=request version=VEngineRandom001',
        '[SiN independent] role=client phase=found-interface version=VEngineRandom001',
        '[SiN independent] role=client phase=request version=VENGINE_GAMEUIFUNCS_VERSION004',
        '[SiN independent] role=client phase=found-interface version=VENGINE_GAMEUIFUNCS_VERSION004',
        '[SiN independent] role=client phase=request version=GAMEEVENTSMANAGER002',
        '[SiN independent] role=client phase=found-interface version=GAMEEVENTSMANAGER002',
        '[SiN independent] role=client phase=request version=VSoundEmitter001',
        '[SiN independent] role=client phase=found-interface version=VSoundEmitter001',
        '[SiN independent] role=client phase=sound-connect-entered version=VSoundEmitter001',
        '[SiN independent] role=client phase=sound-connect-succeeded version=VSoundEmitter001'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($client339) + @(
        '[SiN independent] role=client phase=sound-probe-terminal-stop version=VSoundEmitter001'
    )))
    $out = Verify 'CLIENT_SOUND_CONNECT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if (-not ($out -ccontains 'OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: client') -or
        -not ($out -ccontains 'OWNER_INDEPENDENT_FACTORY_REQUESTS: client:VEngineClient012,client:VEngineModel012,client:VEngineEffects001,client:VEngineCvar002,client:EngineTraceClient003,client:VEngineRenderView011,client:VDebugOverlay003,client:VMaterialSystem076,client:VEngineCache001,client:VModelInfoClient003,client:VEngineVGui001,client:VEngineClientStringTable001,client:SpatialPartition001,client:VEngineShadowMgr002,client:StaticPropMgrClient004,client:IEngineSoundClient003,client:VFileSystem012,client:VEngineRandom001,client:VENGINE_GAMEUIFUNCS_VERSION004,client:GAMEEVENTSMANAGER002,client:VSoundEmitter001') -or
        -not (($out -join '|').Contains(':client:sound-connect-succeeded'))) {
        throw 'Actual Phase 339 client evidence missing'
    }
    # Actual Phase 341 owner trace, including optional stub success.
    $retail341 = @($client339) + @(
        '[SiN independent] role=client phase=textmode-checked version=absent',
        '[SiN independent] role=client phase=optional-request version=VMaterialSystemStub001',
        '[SiN independent] role=client phase=optional-found version=VMaterialSystemStub001',
        '[SiN independent] role=client phase=hardware-config-entered version=MaterialSystemHardwareConfig012',
        '[SiN independent] role=client phase=hardware-config-found version=MaterialSystemHardwareConfig012',
        '[SiN independent] role=client phase=material-probe-result version=acquired'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($retail341) + @(
        '[SiN independent] role=client phase=material-probe-terminal-stop version=MaterialSystemHardwareConfig012'
    )))
    $out = Verify 'CLIENT_MATERIAL_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if (-not (($out -join '|').Contains(':client:acquired'))) { throw 'Actual Phase 341 result missing' }
    # Future screen events are synthetic only. Preserve zero as a real
    # returned value, not proof of a rendering surface.
    foreach ($dimensions in @('1920:1080','0:0')) {
        [IO.File]::WriteAllLines($retailReplay,[string[]](@($retail341) + @(
            '[SiN independent] role=client phase=screen-query-entered version=VEngineClient012.GetScreenSize',
            ("[SiN independent] role=client phase=screen-query-returned version={0}" -f $dimensions),
            '[SiN independent] role=client phase=screen-probe-result version=returned',
            '[SiN independent] role=client phase=screen-probe-terminal-stop version=VEngineClient012.GetScreenSize'
        )))
        $out = Verify 'CLIENT_SCREEN_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if (-not (($out -join '|').Contains(":client:returned:$dimensions"))) { throw 'Screen dimensions missing' }
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($retail341) + @(
        '[SiN independent] role=client phase=screen-probe-result version=returned',
        '[SiN independent] role=client phase=screen-probe-terminal-stop version=VEngineClient012.GetScreenSize'
    )))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Synthetic alternate material outcome. A missing stub is nonfatal.
    $materialPrefix = @($client339) + @(
        '[SiN independent] role=client phase=textmode-checked version=absent',
        '[SiN independent] role=client phase=optional-request version=VMaterialSystemStub001',
        '[SiN independent] role=client phase=optional-missing version=VMaterialSystemStub001',
        '[SiN independent] role=client phase=hardware-config-entered version=MaterialSystemHardwareConfig012'
    )
    foreach ($found in @($true,$false)) {
        $status = if ($found) { 'acquired' } else { 'missing-hardware' }
        $phase = if ($found) { 'hardware-config-found' } else { 'hardware-config-missing' }
        [IO.File]::WriteAllLines($retailReplay,[string[]](@($materialPrefix) + @(
            ("[SiN independent] role=client phase={0} version=MaterialSystemHardwareConfig012" -f $phase),
            ("[SiN independent] role=client phase=material-probe-result version={0}" -f $status),
            '[SiN independent] role=client phase=material-probe-terminal-stop version=MaterialSystemHardwareConfig012'
        )))
        $out = Verify 'CLIENT_MATERIAL_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if (-not (($out -join '|').Contains(":client:$status"))) { throw 'Material result missing' }
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($client339) + @(
        '[SiN independent] role=client phase=material-probe-result version=acquired',
        '[SiN independent] role=client phase=material-probe-terminal-stop version=MaterialSystemHardwareConfig012'
    )))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')

    # Hypothetical client-only probe (not an owner-observed result).
    # Distinguish its terminal Connect from any server outcome.
    foreach ($phase in @('sound-connect-succeeded','sound-connect-rejected')) {
        [IO.File]::WriteAllLines($retailReplay,[string[]]@(
            '[SiN independent] role=client phase=factory-accepted version=VClient011',
            '[SiN independent] role=client phase=init-entered version=<none>',
            '[SiN independent] role=client phase=sound-connect-entered version=VSoundEmitter001',
            ("[SiN independent] role=client phase={0} version=VSoundEmitter001" -f $phase),
            '[SiN independent] role=client phase=sound-probe-terminal-stop version=VSoundEmitter001'
        ))
        $out = Verify 'CLIENT_SOUND_CONNECT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if (-not (($out -join '|').Contains(":client:$phase")) -or
            @($out | Where-Object { $_ -like 'OWNER_SERVER_SOUND_CONNECT_RESULTS: *' }).Count) {
            throw 'Client sound result mixed with server evidence'
        }
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]]@(
        '[SiN independent] role=client phase=init-entered version=<none>',
        '[SiN independent] role=client phase=sound-connect-entered version=VSoundEmitter001',
        '[SiN independent] role=server phase=sound-connect-succeeded version=VSoundEmitter001',
        '[SiN independent] role=client phase=sound-probe-terminal-stop version=VSoundEmitter001'
    ))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    Remove-Item -LiteralPath $retailReplay -Force
    [IO.File]::WriteAllText($client,
        "[SiN independent] role=client phase=factory-accepted version=VClient011" + [Environment]::NewLine)
    # A Shutdown-only callback does not establish that retail Init ran.
    [IO.File]::WriteAllText($client,
        "[SiN independent] role=client phase=shutdown version=<none>" + [Environment]::NewLine)
    [void](Verify 'NO_INDEPENDENT_INIT_EVIDENCE')
    # An explicit slot-zero entry is authoritative even if the body stops
    # before issuing the first engine factory request.
    [IO.File]::WriteAllText($client,(
        "[SiN independent] role=client phase=factory-accepted version=VClient011" + [Environment]::NewLine +
        "[SiN independent] role=client phase=init-entered version=<none>" + [Environment]::NewLine))
    $out = Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF'
    if (-not ($out -contains 'OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: client')) {
        throw "Explicit retail Init entry not reported"
    }
    [IO.File]::WriteAllText($client,(
        "[SiN independent] role=client phase=factory-accepted version=VClient011" + [Environment]::NewLine +
        "[SiN independent] role=client phase=request version=VEngineClient012" + [Environment]::NewLine +
        "[SiN independent] role=client phase=found-interface version=VEngineClient012" + [Environment]::NewLine +
        "[SiN independent] role=client phase=request version=VEngineModel012" + [Environment]::NewLine +
        "[SiN independent] role=client phase=missing-interface version=VEngineModel012" + [Environment]::NewLine))
    $out = Verify 'ENGINE_ENTERED_INDEPENDENT_INIT_BUT_REJECTED'
    if (-not ($out -contains 'OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: client:missing-interface:VEngineModel012')) {
        throw "Wrong client first blocker"
    }
    if (-not ($out -contains 'OWNER_INDEPENDENT_ROLES_OBSERVED: client')) {
        throw "Wrong observed role"
    }
    $server = Join-Path $dir "sin-independent-$session-303.log"
    [IO.File]::WriteAllText($server,(
        "[SiN independent] role=server phase=request version=VEngineServer021" + [Environment]::NewLine +
        "[SiN independent] role=server phase=missing-interface version=VEngineServer021" + [Environment]::NewLine))
    $out = Verify 'ENGINE_ENTERED_INDEPENDENT_INIT_BUT_REJECTED'
    if (-not ($out -contains 'OWNER_INDEPENDENT_TRACE_FILES: 2')) { throw "Missing second log" }
    if (-not ($out -contains 'OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: AMBIGUOUS_ACROSS_PROCESSES')) { throw "Cross-process chronology incorrectly inferred" }
    if (-not (($out | Where-Object { $_ -like 'OWNER_INDEPENDENT_BLOCKERS_BY_LOG: *' }) -match 'client:missing-interface:VEngineModel012')) { throw "Client blocker omitted" }
    if (-not (($out | Where-Object { $_ -like 'OWNER_INDEPENDENT_BLOCKERS_BY_LOG: *' }) -match 'server:missing-interface:VEngineServer021')) { throw "Server blocker omitted" }
    [IO.File]::WriteAllText($client,
        "[SiN independent] role=client phase=sdk2004-preflight-only version=<none>" + [Environment]::NewLine)
    [IO.File]::WriteAllText($server,
        "[SiN independent] role=server phase=sdk2004-preflight-only version=<none>" + [Environment]::NewLine)
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    Write-Output "PASS owner session classifier: cross-session, game-module factory entry, native Init blocker, two roles, no false retail boot"
} finally {
    Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
}
