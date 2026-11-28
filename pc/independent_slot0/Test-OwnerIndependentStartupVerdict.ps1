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
        $replay = Join-Path $dir ("sin-independent-" + $session + "-13124.log")
        $tail = if (Test-Path -LiteralPath $replay) {
            (Get-Content -LiteralPath $replay -Tail 16) -join ' | '
        } else { 'no synthetic replay file' }
        throw ("Unexpected startup verdict: " + ($out -join ' | ') + " | TRACE_TAIL: " + $tail)
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

    # Phase1687: a client prediction factory negotiation in an
    # opted-in owner launch is a diagnostic observation only.
    # It MUST NOT be classified as a completed retail engine boot.
    [IO.File]::WriteAllLines($client,[string[]]@(
        '[SiN independent] role=client phase=factory-accepted version=VClient011',
        '[SiN independent] role=client phase=init-entered version=<none>',
        '[SiN independent] role=client phase=prediction-convar-registered version=cl_predict',
        '[SiN independent] role=client phase=prediction-bound version=cl_predict-parent-value',
        '[SiN independent] role=client phase=factory-accepted version=VClientPrediction001',
        '[SiN independent] role=client phase=unsupported-callback version=VClientPrediction001.slot3-unsupported'
    ))
    $predictionOut=Verify 'UNSUPPORTED_CALLBACK_DIAGNOSTIC_STOP_NO_BOOT_PROOF'
    $expectedPrediction=('OWNER_CLIENT_PREDICTION_FACTORY_BOUNDARY: '+
        'sin-independent-'+$session+'-202.log:client:prediction-bound:cl_predict-parent-value')
    $registeredPrediction=('OWNER_CLIENT_PREDICTION_CONVAR_REGISTRATION: '+
        'sin-independent-'+$session+'-202.log:client:prediction-convar-registered:cl_predict')
    if (-not ($predictionOut -ccontains $expectedPrediction) -or
        -not ($predictionOut -ccontains $registeredPrediction) -or
        -not ($predictionOut -ccontains
            'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: client:factory-accepted:VClient011,client:factory-accepted:VClientPrediction001')) {
        throw 'Owner prediction factory/ConVar registration evidence not retained from same-session native trace'
    }
    # Phase1689: the real original engine's next request is the
    # embedded VClientEntityList003 secondary interface. Original
    # source has a full 9-slot vtable but live parent registration
    # is intentionally fatal. Accept only the new diagnostic
    # marker as a next-interface boundary, never game boot.
    [IO.File]::WriteAllLines($client,[string[]]@(
        '[SiN independent] role=client phase=factory-accepted version=VClient011',
        '[SiN independent] role=client phase=init-entered version=<none>',
        '[SiN independent] role=client phase=prediction-bound version=cl_predict-parent-value',
        '[SiN independent] role=client phase=client-entitylist-empty-bound version=secondary-plus-0x10014:9-slots',
        '[SiN independent] role=client phase=factory-accepted version=VClientPrediction001',
        '[SiN independent] role=client phase=factory-accepted version=VClientEntityList003',
        '[SiN independent] role=client phase=unsupported-callback version=CClientEntityList.primary-register-unsupported'
    ))
    $entityOut=Verify 'UNSUPPORTED_CALLBACK_DIAGNOSTIC_STOP_NO_BOOT_PROOF'
    $entityBoundary=('OWNER_CLIENT_ENTITYLIST_EMPTY_FACTORY_BOUNDARY: '+
        'sin-independent-'+$session+'-202.log:client:client-entitylist-empty-bound:secondary-plus-0x10014:9-slots')
    if (-not ($entityOut -ccontains $entityBoundary) -or
        -not ($entityOut -ccontains
            'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: client:factory-accepted:VClient011,client:factory-accepted:VClientPrediction001,client:factory-accepted:VClientEntityList003')) {
        throw 'Same-process original client secondary entitylist factory was not classified without game-boot claim'
    }

    # Phase1691: reject/fail the original post-21-factory
    # VSoundEmitter001 Connect without mistaking it for the old
    # independently terminal "sound-only" probe or game boot.
    [IO.File]::WriteAllLines($client,[string[]]@(
        '[SiN independent] role=client phase=factory-accepted version=VClient011',
        '[SiN independent] role=client phase=init-entered version=<none>',
        '[SiN independent] role=client phase=sound-connect-entered version=VSoundEmitter001',
        '[SiN independent] role=client phase=sound-connect-rejected version=VSoundEmitter001',
        '[SiN independent] role=client phase=client-sound-connect-continue-refused version=VSoundEmitter001'
    ))
    $soundRejected=Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF'
    $expectedRejected=('OWNER_CLIENT_SOUND_CONNECT_CONTINUATION: '+
        'sin-independent-'+$session+'-202.log:client:client-sound-connect-continue-refused:VSoundEmitter001')
    if(-not ($soundRejected -ccontains $expectedRejected)) {
        throw 'Original client post-factory sound Connect=false was not classified separately'
    }

    # Phase1690: after the original engine's VClientEntityList003
    # request, it asks for VCENTERPRINT002. A cold interface only
    # proves native ABI negotiation; the source slot6 allocates an
    # unavailable 0xF8-byte GUI service, so even a successful
    # factory request and subsequent fatal Init is NOT a game boot.
    [IO.File]::WriteAllLines($client,[string[]]@(
        '[SiN independent] role=client phase=factory-accepted version=VClient011',
        '[SiN independent] role=client phase=init-entered version=<none>',
        '[SiN independent] role=client phase=sound-connect-entered version=VSoundEmitter001',
        '[SiN independent] role=client phase=sound-connect-succeeded version=VSoundEmitter001',
        '[SiN independent] role=client phase=client-sound-connect-continued version=VSoundEmitter001',
        '[SiN independent] role=client phase=prediction-bound version=cl_predict-parent-value',
        '[SiN independent] role=client phase=client-entitylist-empty-bound version=secondary-plus-0x10014:9-slots',
        '[SiN independent] role=client phase=client-centerprint-cold-bound version=nullable-service:8-slots',
        '[SiN independent] role=client phase=factory-accepted version=VClientPrediction001',
        '[SiN independent] role=client phase=factory-accepted version=VClientEntityList003',
        '[SiN independent] role=client phase=factory-accepted version=VCENTERPRINT002',
        '[SiN independent] role=client phase=client-centerprint-slot4-cold version=five-arg-no-backing',
        '[SiN independent] role=client phase=client-centerprint-slot5-cold version=noarg-no-backing',
        '[SiN independent] role=client phase=unsupported-callback version=VCENTERPRINT002.slot6-backing-service-unimplemented'
    ))
    $centerOut=Verify 'UNSUPPORTED_CALLBACK_DIAGNOSTIC_STOP_NO_BOOT_PROOF'
    $expectedCenter=('OWNER_CLIENT_CENTERPRINT_COLD_FACTORY_BOUNDARY: '+
        'sin-independent-'+$session+'-202.log:client:client-centerprint-cold-bound:nullable-service:8-slots')
    $expectedCalls=('OWNER_CLIENT_CENTERPRINT_COLD_METHOD_ENTRY: '+
        'sin-independent-'+$session+'-202.log:client:client-centerprint-slot4-cold:five-arg-no-backing,'+
        'sin-independent-'+$session+'-202.log:client:client-centerprint-slot5-cold:noarg-no-backing')
    $expectedSoundContinue=('OWNER_CLIENT_SOUND_CONNECT_CONTINUATION: '+
        'sin-independent-'+$session+'-202.log:client:client-sound-connect-continued:VSoundEmitter001')
    if (-not ($centerOut -ccontains $expectedCenter) -or
        -not ($centerOut -ccontains $expectedCalls) -or
        -not ($centerOut -ccontains $expectedSoundContinue) -or
        -not ($centerOut -ccontains
            'OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: client:factory-accepted:VClient011,client:factory-accepted:VClientPrediction001,client:factory-accepted:VClientEntityList003,client:factory-accepted:VCENTERPRINT002')) {
        throw 'Same-process original centerprint cold source factory incorrectly classified as retail boot or omitted'
    }

    # The existing retail replay below removes $client after it
    # creates the next synthetic trace. Keep that cleanup order.

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
    # Actual Phase 344 owner observed 1920:1080 and attached, with
    # exit 0x53494E53 and verified restoration. Alternate refusals below
    # remain synthetic. Require the full successful screen path.
    $gaussianPrefix = @($retail341) + @(
        '[SiN independent] role=client phase=screen-query-entered version=VEngineClient012.GetScreenSize',
        '[SiN independent] role=client phase=screen-query-returned version=1920:1080',
        '[SiN independent] role=client phase=screen-probe-result version=returned'
    )
    foreach ($attachResult in @('attached','layout-or-attach-refused','missing-abi-input')) {
        $trace = @($gaussianPrefix)
        if ($attachResult -ne 'missing-abi-input') {
            $trace += '[SiN independent] role=client phase=gaussian-attach-entered version=VEngineRandom001'
        }
        if ($attachResult -eq 'attached') {
            $trace += '[SiN independent] role=client phase=gaussian-attach-verified version=VEngineRandom001'
        }
        $trace += @(
            ("[SiN independent] role=client phase=gaussian-probe-result version={0}" -f $attachResult),
            '[SiN independent] role=client phase=gaussian-probe-terminal-stop version=VEngineRandom001'
        )
        [IO.File]::WriteAllLines($retailReplay,[string[]]$trace)
        $out = Verify 'CLIENT_GAUSSIAN_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if (-not (($out -join '|').Contains(":client:$attachResult"))) { throw 'Phase 344 observed or alternate Gaussian result missing' }
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($gaussianPrefix) + @(
        '[SiN independent] role=client phase=gaussian-probe-result version=attached',
        '[SiN independent] role=client phase=gaussian-probe-terminal-stop version=VEngineRandom001'
    )))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Owner Phase 348 verified the successful client console trace; variants remain synthetic.
    $clientCvarPrefix = @($gaussianPrefix) + @(
        '[SiN independent] role=client phase=gaussian-attach-entered version=VEngineRandom001',
        '[SiN independent] role=client phase=gaussian-attach-verified version=VEngineRandom001',
        '[SiN independent] role=client phase=gaussian-probe-result version=attached'
    )
    $clientCvarTrace = @($clientCvarPrefix) + @(
        '[SiN independent] role=client phase=client-cvar-preflight-complete version=3'
    )
    foreach ($name in @('cl_starfield_distance','cl_starfield_diameter','cl_show_bloodspray')) {
        foreach ($phase in @('client-cvar-register-entered','client-cvar-register-returned','client-cvar-registration-verified')) {
            $clientCvarTrace += ("[SiN independent] role=client phase={0} version={1}" -f $phase,$name)
        }
    }
    $clientCvarTrace += @(
        '[SiN independent] role=client phase=client-cvar-probe-result version=verified:3/3',
        '[SiN independent] role=client phase=client-cvar-probe-terminal-stop version=3'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]$clientCvarTrace)
    $out = Verify 'CLIENT_CVAR_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if (-not (($out -join '|').Contains(':client:verified:3/3'))) { throw 'Client cvar result missing' }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($clientCvarPrefix) + @(
        '[SiN independent] role=client phase=client-cvar-probe-result version=existing-name:0/3',
        '[SiN independent] role=client phase=client-cvar-probe-terminal-stop version=3'
    )))
    $out = Verify 'CLIENT_CVAR_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if (-not (($out -join '|').Contains(':client:existing-name:0/3'))) { throw 'Client cvar refusal missing' }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($clientCvarPrefix) + @(
        '[SiN independent] role=client phase=client-cvar-probe-result version=verified:3/3',
        '[SiN independent] role=client phase=client-cvar-probe-terminal-stop version=3'
    )))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Owned startup is still synthetic. It must follow a complete cvar prefix.
    $ownedPrefix=@($clientCvarTrace[0..($clientCvarTrace.Count-2)])
    $ownedEvents=@(
        '[SiN independent] role=client phase=owned-startup-entered version=independent-only',
        '[SiN independent] role=client phase=owned-startup-objects-ready version=1',
        '[SiN independent] role=client phase=owned-startup-pool-ready version=2048x96',
        '[SiN independent] role=client phase=owned-startup-pool-verified version=allocate-reuse-release',
        '[SiN independent] role=client phase=owned-startup-cleanup-verified version=pool-objects',
        '[SiN independent] role=client phase=owned-startup-probe-result version=verified',
        '[SiN independent] role=client phase=owned-startup-terminal-stop version=independent-only'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]($ownedPrefix+$ownedEvents))
    $out=Verify 'CLIENT_OWNED_STARTUP_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if(-not (($out -join '|').Contains(':client:verified'))) {throw 'Owned startup result missing'}
    [IO.File]::WriteAllLines($retailReplay,[string[]]($ownedPrefix+@($ownedEvents[0],
        '[SiN independent] role=client phase=owned-startup-probe-result version=refused',
        $ownedEvents[6])))
    $out=Verify 'CLIENT_OWNED_STARTUP_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if(-not (($out -join '|').Contains(':client:refused'))) {throw 'Owned startup refusal missing'}
    [IO.File]::WriteAllLines($retailReplay,[string[]]($ownedPrefix+@($ownedEvents[0],$ownedEvents[1])+$ownedEvents[3..6]))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($clientCvarPrefix)+@($ownedEvents)))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    $vguiPrefix=$ownedPrefix+@($ownedEvents[0..5])
    $vguiEvents=@(
        '[SiN independent] role=client phase=vgui-probe-entered version=vgui2.dll',
        '[SiN independent] role=client phase=vgui-provider-found version=vgui2.dll',
        '[SiN independent] role=client phase=vgui-query-entered version=VGUI_InputInternal001',
        '[SiN independent] role=client phase=vgui-query-found version=VGUI_InputInternal001',
        '[SiN independent] role=client phase=vgui-query-entered version=VGUI_System010',
        '[SiN independent] role=client phase=vgui-query-found version=VGUI_System010',
        '[SiN independent] role=client phase=vgui-probe-result version=acquired',
        '[SiN independent] role=client phase=vgui-probe-terminal-stop version=factory-only'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]]($vguiPrefix+$vguiEvents))
    [void](Verify 'CLIENT_VGUI_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF')
    [IO.File]::WriteAllLines($retailReplay,[string[]]($vguiPrefix+@($vguiEvents[0..2])+@(
        '[SiN independent] role=client phase=vgui-probe-result version=missing-input',$vguiEvents[7])))
    [void](Verify 'CLIENT_VGUI_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF')
    # Distinguish a missing module from a loaded DLL with no factory.
    foreach($refusal in @('missing-module','missing-factory')) {
        [IO.File]::WriteAllLines($retailReplay,[string[]]($vguiPrefix+@(
            $vguiEvents[0],
            ("[SiN independent] role=client phase=vgui-probe-result version={0}" -f $refusal),
            $vguiEvents[7])))
        $out=Verify 'CLIENT_VGUI_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if(-not (($out -join '|').Contains(":client:$refusal"))) {throw 'Module/factory refusal missing'}
    }
    # Input succeeded but the second interface was unavailable.
    [IO.File]::WriteAllLines($retailReplay,[string[]]($vguiPrefix+@($vguiEvents[0..4])+@(
        '[SiN independent] role=client phase=vgui-probe-result version=missing-system',$vguiEvents[7])))
    $out=Verify 'CLIENT_VGUI_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if(-not (($out -join '|').Contains(':client:missing-system'))) {throw 'System refusal missing'}
    # Both names resolved, but the provider returned the same identity.
    [IO.File]::WriteAllLines($retailReplay,[string[]]($vguiPrefix+@($vguiEvents[0..5])+@(
        '[SiN independent] role=client phase=vgui-probe-result version=aliased-interfaces',$vguiEvents[7])))
    $out=Verify 'CLIENT_VGUI_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if(-not (($out -join '|').Contains(':client:aliased-interfaces'))) {throw 'Alias refusal missing'}
    [IO.File]::WriteAllLines($retailReplay,[string[]]($vguiPrefix+@($vguiEvents[0..2])+$vguiEvents[4..7]))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    [IO.File]::WriteAllLines($retailReplay,[string[]]($ownedPrefix+$vguiEvents))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Read-only vtable anchors: no original virtual methods are called.
    $layoutPrefix=$vguiPrefix+@($vguiEvents[0..6])
    $layoutBase=@(
        '[SiN independent] role=client phase=vgui-layout-entered version=input:0+56,system:0+24'
    )
    $layoutInput='[SiN independent] role=client phase=vgui-layout-input-verified version=0+56'
    $layoutSystem='[SiN independent] role=client phase=vgui-layout-system-verified version=0+24'
    $layoutStop='[SiN independent] role=client phase=vgui-layout-terminal-stop version=read-only'
    [IO.File]::WriteAllLines($retailReplay,[string[]]($layoutPrefix+$layoutBase+@(
        $layoutInput,$layoutSystem,
        '[SiN independent] role=client phase=vgui-layout-probe-result version=verified',
        $layoutStop)))
    $out=Verify 'CLIENT_VGUI_VTABLE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if(-not (($out -join '|').Contains(':client:verified'))) {throw 'VGUI vtable success missing'}
    [IO.File]::WriteAllLines($retailReplay,[string[]]($layoutPrefix+$layoutBase+@(
        '[SiN independent] role=client phase=vgui-layout-probe-result version=invalid-input',
        $layoutStop)))
    $out=Verify 'CLIENT_VGUI_VTABLE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if(-not (($out -join '|').Contains(':client:invalid-input'))) {throw 'VGUI input refusal missing'}
    [IO.File]::WriteAllLines($retailReplay,[string[]]($layoutPrefix+$layoutBase+@(
        $layoutInput,
        '[SiN independent] role=client phase=vgui-layout-probe-result version=invalid-system',
        $layoutStop)))
    $out=Verify 'CLIENT_VGUI_VTABLE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if(-not (($out -join '|').Contains(':client:invalid-system'))) {throw 'VGUI system refusal missing'}
    # A fabricated success without verified input/system anchors must be rejected.
    [IO.File]::WriteAllLines($retailReplay,[string[]]($layoutPrefix+$layoutBase+@(
        '[SiN independent] role=client phase=vgui-layout-probe-result version=verified',
        $layoutStop)))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    [IO.File]::WriteAllLines($retailReplay,[string[]]($vguiPrefix+$layoutBase+@(
        '[SiN independent] role=client phase=vgui-layout-probe-result version=invalid-input',
        $layoutStop)))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Original VEngineVGui001 virtual slot1 with argument two, after
    # fully ordered VGUI provider and layout verification.
    $engineSlotPrefix=$layoutPrefix+$layoutBase+@(
        $layoutInput,$layoutSystem,
        '[SiN independent] role=client phase=vgui-layout-probe-result version=verified'
    )
    $engineSlotEntry='[SiN independent] role=client phase=engine-vgui-slot1-entered version=VEngineVGui001:arg2'
    $engineSlotStop='[SiN independent] role=client phase=engine-vgui-slot1-terminal-stop version=original-slot1'
    foreach($outcome in @('nonzero','zero','invalid-vtable','invalid-object','invalid-method')) {
        [IO.File]::WriteAllLines($retailReplay,[string[]]($engineSlotPrefix+@(
            $engineSlotEntry,
            ("[SiN independent] role=client phase=engine-vgui-slot1-result version={0}" -f $outcome),
            $engineSlotStop)))
        $out=Verify 'CLIENT_ENGINE_VGUI_SLOT1_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if(-not (($out -join '|').Contains(":client:$outcome"))) {throw "Engine VGUI slot1 outcome missing: $outcome"}
    }
    # Missing original-ABI prerequisite must never become a verified result.
    [IO.File]::WriteAllLines($retailReplay,[string[]]($layoutPrefix+$layoutBase+@(
        $engineSlotEntry,
        '[SiN independent] role=client phase=engine-vgui-slot1-result version=nonzero',
        $engineSlotStop)))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    [IO.File]::WriteAllLines($retailReplay,[string[]]($engineSlotPrefix+@(
        '[SiN independent] role=client phase=engine-vgui-slot1-result version=nonzero',
        $engineSlotStop)))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # VGui_InitInterfacesList owns eight exact factory names and the
    # IVGui.Init(factory*,1) call before the panel lookup.
    $controlsStart='[SiN independent] role=client phase=vgui-controls-bootstrap-entered version=CLIENT:factory-count-1'
    $controlsStop='[SiN independent] role=client phase=vgui-controls-bootstrap-terminal-stop version=source2004'
    $controlsNames=@('VGUI_ivgui006','VGUI_Panel009','VGUI_Surface028','VGUI_Scheme010','VGUI_System010','VGUI_Input005','VGUI_Localize003','VFileSystem012')
    $controlsEvents=New-Object System.Collections.Generic.List[string]
    foreach($name in $controlsNames) {
        $controlsEvents.Add(("[SiN independent] role=client phase=vgui-controls-request version={0}" -f $name))
        $controlsEvents.Add(("[SiN independent] role=client phase=vgui-controls-found version={0}" -f $name))
    }
    foreach($return in @('true','false')) {
        [IO.File]::WriteAllLines($retailReplay,[string[]]($engineSlotPrefix+@($controlsStart)+
            @($controlsEvents)+@(
            '[SiN independent] role=client phase=vgui-controls-init-entered version=VGUI_ivgui006:factory-count-1',
            ("[SiN independent] role=client phase=vgui-controls-init-returned version={0}" -f $return),
            ("[SiN independent] role=client phase=vgui-controls-bootstrap-result version=init-{0}" -f $return),
            $engineSlotEntry,
            '[SiN independent] role=client phase=engine-vgui-slot1-result version=nonzero',
            $engineSlotStop,
            $controlsStop)))
        $out=Verify 'CLIENT_VGUI_CONTROLS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if(-not (($out -join '|').Contains(":client:init-${return}:panel-nonzero"))) {
            throw "VGUI controls initialization classification missing for $return"
        }
    }
    $missingEvents=New-Object System.Collections.Generic.List[string]
    for($i=0;$i -lt $controlsNames.Count;$i++){
        $name=$controlsNames[$i]
        $missingEvents.Add(("[SiN independent] role=client phase=vgui-controls-request version={0}" -f $name))
        $state=if($i -eq 3){'missing'}else{'found'}
        $missingEvents.Add(("[SiN independent] role=client phase=vgui-controls-{0} version={1}" -f $state,$name))
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]]($engineSlotPrefix+@($controlsStart)+
        @($missingEvents)+@(
        '[SiN independent] role=client phase=vgui-controls-bootstrap-result version=missing-interface',
        $controlsStop)))
    $out=Verify 'CLIENT_VGUI_CONTROLS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if(-not (($out -join '|').Contains(':client:missing-interface:panel-not-reached'))) {
        throw 'VGUI controls missing-interface classifier result absent'
    }
    # A claimed success without Init invocation must not be accepted.
    [IO.File]::WriteAllLines($retailReplay,[string[]]($engineSlotPrefix+@($controlsStart)+
        @($controlsEvents)+@(
        '[SiN independent] role=client phase=vgui-controls-bootstrap-result version=init-true',
        $controlsStop)))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # A missing VGUI interface must not be followed by an accepted Init.
    [IO.File]::WriteAllLines($retailReplay,[string[]]($engineSlotPrefix+@($controlsStart)+
        @($missingEvents)+@(
        '[SiN independent] role=client phase=vgui-controls-init-entered version=VGUI_ivgui006:factory-count-1',
        '[SiN independent] role=client phase=vgui-controls-init-returned version=true',
        '[SiN independent] role=client phase=vgui-controls-bootstrap-result version=init-true',
        $controlsStop)))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Post-VGUI original secondary IAppSystem slot-2 QueryInterface.
    $matSurfaceEntry='[SiN independent] role=client phase=mat-surface-query-entered version=VGUI_Surface028:IAppSystem+4'
    $matSurfaceStop='[SiN independent] role=client phase=mat-surface-query-terminal-stop version=original-abi'
    $matSurfacePrefix=$engineSlotPrefix+@($controlsStart)+@($controlsEvents)+@(
        '[SiN independent] role=client phase=vgui-controls-init-entered version=VGUI_ivgui006:factory-count-1',
        '[SiN independent] role=client phase=vgui-controls-init-returned version=true',
        '[SiN independent] role=client phase=vgui-controls-bootstrap-result version=init-true',
        $engineSlotEntry,
        '[SiN independent] role=client phase=engine-vgui-slot1-result version=nonzero',
        $engineSlotStop)
    foreach($state in @('found','missing-interface','invalid-vtable','invalid-method')) {
        [IO.File]::WriteAllLines($retailReplay,[string[]]($matSurfacePrefix+@(
            $matSurfaceEntry,
            ("[SiN independent] role=client phase=mat-surface-query-result version={0}" -f $state),
            $matSurfaceStop)))
        $out=Verify 'CLIENT_MAT_SURFACE_QUERY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if(-not (($out -join '|').Contains(":client:$state"))) {throw "MatSystemSurface $state outcome missing"}
    }
    # Classifier must not promote a fabricated success missing the
    # exact original GetPanel call-and-return prerequisite.
    [IO.File]::WriteAllLines($retailReplay,[string[]]($engineSlotPrefix+@($controlsStart)+
        @($controlsEvents)+@(
        '[SiN independent] role=client phase=vgui-controls-init-entered version=VGUI_ivgui006:factory-count-1',
        '[SiN independent] role=client phase=vgui-controls-init-returned version=true',
        '[SiN independent] role=client phase=vgui-controls-bootstrap-result version=init-true',
        $matSurfaceEntry,
        '[SiN independent] role=client phase=mat-surface-query-result version=found',
        $matSurfaceStop)))
    [void](Verify 'DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF')
    # Owner-original CSoundEmitterSystem::Init delegates to the
    # already acquired VSoundEmitter001 interface slot 1 after surface.
    $soundInitEntry='[SiN independent] role=client phase=sound-system-init-entered version=CSoundEmitterSystem:VSoundEmitter001+1'
    $soundInitStop='[SiN independent] role=client phase=sound-system-init-terminal-stop version=original-slot1'
    $matSurfaceFound='[SiN independent] role=client phase=mat-surface-query-result version=found'
    $soundInitPrefix = [string[]](@($matSurfacePrefix) + @($matSurfaceEntry) + @($matSurfaceFound))
    if (-not ($soundInitPrefix -contains $matSurfaceFound)) {
        throw 'Synthetic sound Init fixture omitted its mandatory surface-result predecessor'
    }
    foreach($state in @('called-true','called-false','invalid-object',
                        'invalid-vtable','invalid-method')) {
        $soundReplay = New-Object 'System.Collections.Generic.List[string]'
        foreach ($event in $soundInitPrefix) { $soundReplay.Add([string]$event) }
        $soundReplay.Add($soundInitEntry)
        $soundReplay.Add(("[SiN independent] role=client phase=sound-system-init-result version={0}" -f $state))
        $soundReplay.Add($soundInitStop)
        [IO.File]::WriteAllLines($retailReplay,$soundReplay.ToArray())
        $written = [IO.File]::ReadAllLines($retailReplay)
        $surfaceStartIndex = [Array]::IndexOf($written,$matSurfaceEntry)
        $surfaceFoundIndex = [Array]::IndexOf($written,$matSurfaceFound)
        if ($surfaceStartIndex -lt 0 -or $surfaceFoundIndex -ne ($surfaceStartIndex + 1) -or
            @($written | Where-Object { $_ -ceq $matSurfaceEntry }).Count -ne 1 -or
            @($written | Where-Object { $_ -ceq $matSurfaceFound }).Count -ne 1) {
            throw ("Sound fixture surface-result order invalid: entered={0}, found={1}, total={2}; matching={3}" -f
                $surfaceStartIndex,$surfaceFoundIndex,$written.Length,
                (@($written | Where-Object { $_ -like '*mat-surface*' }) -join '|'))
        }
        $out=Verify 'CLIENT_SOUND_SYSTEM_INIT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if(-not (($out -join '|').Contains(":client:$state"))) {throw "Sound system Init outcome missing: $state"}
    }
    # Invalid sound traces must NOT be promoted to SoundSystem Init,
    # but retain the already verified earlier engine-panel milestone.
    [IO.File]::WriteAllLines($retailReplay,[string[]]($matSurfacePrefix+@(
        $matSurfaceEntry,
        $soundInitEntry,
        '[SiN independent] role=client phase=sound-system-init-result version=called-true',
        $soundInitStop)))
    $negative=Verify 'CLIENT_ENGINE_VGUI_SLOT1_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if (($negative -join '|').Contains('OWNER_CLIENT_SOUND_SYSTEM_INIT_RESULTS:')) {
        throw 'Missing surface-result improperly promoted sound Init'
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]]($matSurfacePrefix+@(
        $matSurfaceEntry,
        '[SiN independent] role=client phase=mat-surface-query-result version=missing-interface',
        $soundInitEntry,
        '[SiN independent] role=client phase=sound-system-init-result version=called-true',
        $soundInitStop)))
    $negative=Verify 'CLIENT_ENGINE_VGUI_SLOT1_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    if (($negative -join '|').Contains('OWNER_CLIENT_SOUND_SYSTEM_INIT_RESULTS:')) {
        throw 'Failed surface-query improperly promoted sound Init'
    }
    # Synthetic future stats-content probe: require the same-process sound
    # game-system true return before allowing a post-sound verdict.
    $statsPrefix=[string[]](@($soundInitPrefix)+@(
        $soundInitEntry,
        '[SiN independent] role=client phase=sound-system-init-result version=called-true'
    ))
    foreach($case in @(
        @{ Counts='62:8:156:66'; State='verified-original-counts' },
        @{ Counts='61:8:156:66'; State='unexpected-retail-counts' },
        @{ Counts='0:0:0:0'; State='missing-absolute-root' },
        @{ Counts='0:0:0:0'; State='asset-read-or-parse-refused' }
    )) {
        $trace=[string[]](@($statsPrefix)+@(
            '[SiN independent] role=client phase=stats-asset-probe-entered version=scripts/stats',
            ("[SiN independent] role=client phase=stats-asset-probe-counts version={0}" -f $case.Counts),
            ("[SiN independent] role=client phase=stats-asset-probe-result version={0}" -f $case.State),
            '[SiN independent] role=client phase=stats-asset-probe-terminal-stop version=independent-content-io'
        ))
        [IO.File]::WriteAllLines($retailReplay,$trace)
        $out=Verify 'CLIENT_STATS_ASSET_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if(-not (($out -join '|').Contains(":client:$($case.State):$($case.Counts)"))) {
            throw "Stats asset probe outcome missing: $($case.State)"
        }
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($soundInitPrefix)+@(
        $soundInitEntry,
        '[SiN independent] role=client phase=sound-system-init-result version=called-false',
        '[SiN independent] role=client phase=stats-asset-probe-entered version=scripts/stats',
        '[SiN independent] role=client phase=stats-asset-probe-counts version=62:8:156:66',
        '[SiN independent] role=client phase=stats-asset-probe-result version=verified-original-counts',
        '[SiN independent] role=client phase=stats-asset-probe-terminal-stop version=independent-content-io'
    )))
    $out=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($out -join '|').Contains('OWNER_CLIENT_STATS_ASSET_PROBE_RESULTS:')) {
        throw 'Failed sound Init improperly promoted the stats probe'
    }
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($statsPrefix)+@(
        '[SiN independent] role=client phase=stats-asset-probe-entered version=scripts/stats',
        '[SiN independent] role=client phase=stats-asset-probe-counts version=0:0:0:0',
        '[SiN independent] role=client phase=stats-asset-probe-result version=verified-original-counts',
        '[SiN independent] role=client phase=stats-asset-probe-terminal-stop version=independent-content-io'
    )))
    $out=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($out -join '|').Contains('OWNER_CLIENT_STATS_ASSET_PROBE_RESULTS:')) {
        throw 'Mismatched counts improperly promoted the stats probe'
    }
    # SDK2004 VFileSystem read-only owner probe fixtures are synthetic.
    # Require the same-process original sound callback to return true.
    $engineFsPrefix=[string[]](@($soundInitPrefix)+@(
        $soundInitEntry,
        '[SiN independent] role=client phase=sound-system-init-result version=called-true'
    ))
    $engineFsStart='[SiN independent] role=client phase=engine-fs-presence-entered version=VFileSystem012:source2004'
    $engineFsStop='[SiN independent] role=client phase=engine-fs-terminal-stop version=read-only-diagnostic'
    foreach($kind in @('both-present','missing-definitions','missing-layout','invalid-abi')) {
        $trace=[string[]](@($engineFsPrefix)+@(
            $engineFsStart,
            ('[SiN independent] role=client phase=engine-fs-presence-result version='+$kind),
            $engineFsStop
        ))
        [IO.File]::WriteAllLines($retailReplay,$trace)
        $out=Verify 'CLIENT_ENGINE_FS_PRESENCE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if(-not (($out -join '|').Contains((':client:'+$kind)))) {
            throw ('Engine filesystem presence not classified: '+$kind)
        }
    }
    $engineFsReadStart='[SiN independent] role=client phase=engine-fs-read-entered version=VFileSystem012:read-only'
    foreach($case in @(
        @{ Counts='62:8:156:66'; State='parsed-and-verified' },
        @{ Counts='61:8:156:66'; State='unexpected-counts' },
        @{ Counts='0:0:0:0'; State='read-refused' },
        @{ Counts='0:0:0:0'; State='invalid-abi' }
    )) {
        $trace=[string[]](@($engineFsPrefix)+@(
            $engineFsStart,
            '[SiN independent] role=client phase=engine-fs-presence-result version=both-present',
            $engineFsReadStart,
            ('[SiN independent] role=client phase=engine-fs-read-counts version='+$case.Counts),
            ('[SiN independent] role=client phase=engine-fs-read-result version='+$case.State),
            $engineFsStop
        ))
        [IO.File]::WriteAllLines($retailReplay,$trace)
        $out=Verify 'CLIENT_ENGINE_FS_READ_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if(-not (($out -join '|').Contains((':client:both-present:'+$case.State+':'+$case.Counts)))) {
            throw ('Engine filesystem read not classified: '+$case.State)
        }
    }
    # Proposed original-mode staged Stats diagnostic (synthetic ONLY).
    # A standalone staged result must follow a valid SAME-SESSION
    # original sound Init, filesystem presence and combined engine read.
    $stagedReadPrefix=[string[]](@($engineFsPrefix)+@(
        $engineFsStart
        '[SiN independent] role=client phase=engine-fs-presence-result version=both-present'
        $engineFsReadStart
        '[SiN independent] role=client phase=engine-fs-read-counts version=62:8:156:66'
        '[SiN independent] role=client phase=engine-fs-read-result version=parsed-and-verified'
    ))
    $stagedSteps=[string[]]@(
        '[SiN independent] role=client phase=engine-fs-staged-entered version=rb-GAME:rt-null:owned-only'
        '[SiN independent] role=client phase=engine-fs-staged-counts version=66:62:8:156:66'
        '[SiN independent] role=client phase=engine-fs-staged-stage version=7:7'
        '[SiN independent] role=client phase=engine-fs-staged-result version=owned-staged-verified-not-engine-init'
        '[SiN independent] role=client phase=engine-fs-staged-terminal-stop version=original-modes-owned-only'
    )
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($stagedReadPrefix)+@($stagedSteps)))
    $out=Verify 'CLIENT_ENGINE_FS_STAGED_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedLine=('OWNER_CLIENT_ENGINE_FS_STAGED_RESULTS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'owned-staged-verified-not-engine-init:66:62:8:156:66:7:7')
    if (-not ($out -ccontains $expectedLine)) {
        throw ('Original staged mode not attributed to its own session: '+($out -join '|'))
    }
    # New hook-proof trace is a DISTINCT terminal after staged success,
    # and can only be classified with the correct same-session 27:28:5.
    $hookSteps=[string[]]@(
        '[SiN independent] role=client phase=engine-fs-hooks-entered version=CUserMessages:owned-only'
        '[SiN independent] role=client phase=engine-fs-hooks-ids version=27:28:5'
        '[SiN independent] role=client phase=engine-fs-hooks-count version=3'
        '[SiN independent] role=client phase=engine-fs-hooks-result version=three-owned-hooks-bound-not-dispatched'
        '[SiN independent] role=client phase=engine-fs-hooks-terminal-stop version=independent-only'
    )
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)))
    $hookOut=Verify 'CLIENT_ENGINE_FS_HOOKS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedHooks=('OWNER_CLIENT_ENGINE_FS_HOOK_RESULTS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'three-owned-hooks-bound-not-dispatched:27:28:5:3')
    if (-not ($hookOut -ccontains $expectedHooks)) {
        throw ('Independent client Stats owned-hook trace not bound: '+($hookOut -join '|'))
    }
    # Phase 486 must be a *distinct* fourth diagnostic, accepted only
    # after the same-session previous 27:28:5 owned hook milestone.
    $soundscapeSteps=[string[]]@(
        '[SiN independent] role=client phase=engine-fs-soundscape-entered version=rb-GAME:read-only'
        '[SiN independent] role=client phase=engine-fs-soundscape-counts version=7:301:242:3'
        '[SiN independent] role=client phase=engine-fs-soundscape-result version=owned-soundscape-catalog-verified-original-files'
        '[SiN independent] role=client phase=engine-fs-soundscape-detail version=complete:owned-engine-soundscape-catalog-ready-no-game-init:owned-soundscape-manifest-ready:independent-soundscape-catalog-ready:7:7'
        '[SiN independent] role=client phase=engine-fs-soundscape-terminal-stop version=owner-original-files-no-game-init'
    )
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($soundscapeSteps)))
    $soundscapeOut=Verify 'CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedSoundscape=('OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'owned-soundscape-catalog-verified-original-files:7:301:242:3')
    $expectedDetail=('OWNER_CLIENT_SOUNDSCAPE_DETAILS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'complete:owned-engine-soundscape-catalog-ready-no-game-init:'+
        'owned-soundscape-manifest-ready:independent-soundscape-catalog-ready:7:7')
    if (-not ($soundscapeOut -ccontains $expectedSoundscape) -or
        -not ($soundscapeOut -ccontains $expectedHooks) -or
        -not ($soundscapeOut -ccontains $expectedDetail)) {
        throw ('Soundscape did not follow same-session owned Stats hooks: '+($soundscapeOut -join '|'))
    }
    # Phase 497: strictly opt-in read-only original C_SoundscapeSystem
    # ICvar::FindVar snapshot. It must occur IN THE SAME LOG, after
    # the successful hook + catalog prefix and before terminal stop.
    # It is advisory and NEVER makes this a game-boot verdict.
    $soundscapeCvarSteps=[string[]]@(
        '[SiN independent] role=client phase=engine-fs-soundscape-cvar-entered version=VEngineCvar002:slot3:read-only'
        '[SiN independent] role=client phase=engine-fs-soundscape-cvar-result version=original-soundscape-both-vars-cached:2:2:1:1'
        '[SiN independent] role=client phase=engine-fs-soundscape-cvar-terminal-stop version=no-convar-writes-no-game-init'
    )
    $withCvar=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($withCvar)))
    $withCvarOut=Verify 'CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedCvar=('OWNER_CLIENT_SOUNDSCAPE_CVAR_LOOKUP_RESULTS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'original-soundscape-both-vars-cached:2:2:1:1')
    if(-not ($withCvarOut -ccontains $expectedCvar) -or
       -not ($withCvarOut -ccontains $expectedDetail)) {
        throw ('Same-session optional original ICvar lookup was not reported: '+($withCvarOut -join '|'))
    }
    # Opt-in original CClientDLL physics Factory: three interface
    # queries after the COMPLETE original same-session CVar trace.
    # This is read-only, no 0x241927A0 coupling and NO VClient011
    # Init success, even when all three interface names exist.
    $physicsSteps=[string[]]@(
        '[SiN independent] role=client phase=engine-fs-physics-entered version=physics-factory:three-original:read-only'
        '[SiN independent] role=client phase=engine-fs-physics-result version=original-physics-three-available:3:3:1'
        '[SiN independent] role=client phase=engine-fs-physics-terminal-stop version=no-physics-coupling-no-game-init'
    )
    $withPhysics=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($withPhysics)))
    $physicsOut=Verify 'CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedPhysics=('OWNER_CLIENT_PHYSICS_FACTORY_RESULTS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'original-physics-three-available:3:3:1')
    if(-not ($physicsOut -ccontains $expectedPhysics) -or
       -not ($physicsOut -ccontains $expectedCvar) -or
       -not ($physicsOut -ccontains $expectedSoundscape)) {
        throw ('Opt-in original triple physics Factory was not recorded after verified CVar/session: '+($physicsOut -join '|'))
    }
    foreach($status in @(
        'original-physics-missing-first:1:0:0',
        'original-physics-missing-second:2:1:0',
        'original-physics-missing-third:3:2:0',
        'original-physics-no-factory:0:0:0')) {
        $missingPhysics=@($physicsSteps)
        $missingPhysics[1]=$missingPhysics[1].Replace(
            'original-physics-three-available:3:3:1',$status)
        $missingTrace=[string[]](@($soundscapeSteps[0..3])+
            @($soundscapeCvarSteps)+@($missingPhysics)+@($soundscapeSteps[4]))
        [IO.File]::WriteAllLines($retailReplay,
            [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($missingTrace)))
        $missingOut=@(& $Classifier -Session $session -LogDirectory $dir)
        if(-not (($missingOut -join '|').Contains(
            'OWNER_CLIENT_PHYSICS_FACTORY_RESULTS: ')) -or
           (($missingOut -join '|').Contains($expectedPhysics))) {
            throw ('Missing original physics Factory classification not reported accurately: '+$status)
        }
    }
    # Original surfaceproperties::Init 0x241927A0 downstream
    # resource read. It is only accepted after the same client has
    # actually reported all three physics interface Factory results.
    # This is an owned resource SHAPE, not real Source parsing
    # into the physics object's virtual slot1.
    $surfacePropsSteps=[string[]]@(
        '[SiN independent] role=client phase=engine-fs-surfaceprops-entered version=VFileSystem012:surfaceproperties:rb-dual-scope'
        '[SiN independent] role=client phase=engine-fs-surfaceprops-result version=original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0'
        '[SiN independent] role=client phase=engine-fs-surfaceprops-terminal-stop version=no-physics-surface-registration-no-game-init'
    )
    $fullTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($fullTrace)))
    $propsOut=Verify 'CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedProps=('OWNER_CLIENT_SURFACEPROPS_RESULTS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0')
    if(-not ($propsOut -ccontains $expectedProps) -or
       -not ($propsOut -ccontains $expectedPhysics) -or
       -not ($propsOut -ccontains $expectedCvar)) {
        throw ('Original surfaceprops manifest and 2-source shape not accepted after true physics 3/3: '+($propsOut -join '|'))
    }
    # Phase522: the initial engine table is PREPOPULATED. Verify that
    # three Source2004 vslot5 material samples classify even when NO
    # ParseSurfaceData call has been requested in this client session.
    $materialSteps=[string[]]@(
        '[SiN independent] role=client phase=engine-fs-material-entered version=VPhysicsSurfaceProps001:slots2-3-5:read-only',
        '[SiN independent] role=client phase=engine-fs-material-result version=original-physics-material-three-samples:94:3',
        '[SiN independent] role=client phase=engine-fs-material-sample version=default:0:0.8:0.2:1000:0:0',
        '[SiN independent] role=client phase=engine-fs-material-sample version=solidmetal:1:0.3:0.1:7800:0.2:0',
        '[SiN independent] role=client phase=engine-fs-material-sample version=metalvehicle:85:0.4:0.4:3900:0.3:0',
        '[SiN independent] role=client phase=engine-fs-material-terminal-stop version=read-only-physics-fields-no-game-init'
    )
    $materialTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($materialSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($materialTrace)))
    $materialOut=Verify 'CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedMaterial=('OWNER_CLIENT_SURFACEPHYSICS_MATERIAL_RESULTS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'original-physics-material-three-samples:94:3')
    if(-not ($materialOut -ccontains $expectedMaterial) -or
       -not ($materialOut -ccontains $expectedProps) -or
       ($materialOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_REGISTRATION_RESULTS:')) {
        throw 'Pure read-only physics surface tuples incorrectly depend on native parser mutation'
    }
    foreach($mutation in @('missing-sample','duplicate-sample','bad-float','bad-index','missing-resources')) {
        $bad=@($materialSteps)
        $badResources=@($surfacePropsSteps)
        if($mutation -eq 'missing-sample') {
            $bad=@($materialSteps[0],$materialSteps[1],
                $materialSteps[2],$materialSteps[3],$materialSteps[5])
        } elseif($mutation -eq 'duplicate-sample') {
            $bad=@($materialSteps[0],$materialSteps[1],
                $materialSteps[2],$materialSteps[3],$materialSteps[4],
                $materialSteps[4],$materialSteps[5])
        } elseif($mutation -eq 'bad-float') {
            $bad[3]=$bad[3].Replace('0.3:0.1:7800','NaN:0.1:7800')
        } elseif($mutation -eq 'bad-index') {
            $bad[4]=$bad[4].Replace('metalvehicle:85:','metalvehicle:99:')
        } elseif($mutation -eq 'missing-resources') {
            $badResources=@()
        }
        $trace=[string[]](@($soundscapeSteps[0..3])+
            @($soundscapeCvarSteps)+@($physicsSteps)+
            @($badResources)+@($bad)+@($soundscapeSteps[4]))
        [IO.File]::WriteAllLines($retailReplay,
            [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($trace)))
        $badOut=@(& $Classifier -Session $session -LogDirectory $dir)
        if(($badOut -join '|').Contains('OWNER_CLIENT_SURFACEPHYSICS_MATERIAL_RESULTS:')) {
            throw ('Forged physics material sample elevated to owner evidence: '+$mutation)
        }
    }
    # Phase 515: mutating ParseSurfaceData is accepted only after
    # all prior owner-gated soundscape, physics and resource results.
    $registrationSteps=[string[]]@(
        '[SiN independent] role=client phase=engine-fs-registration-entered version=VPhysicsSurfaceProps001:slot1:explicit-opt-in'
        '[SiN independent] role=client phase=engine-fs-registration-result version=original-surfaceprops-register-two-calls-returned:2:2:11:22'
        '[SiN independent] role=client phase=engine-fs-registration-terminal-stop version=two-engine-call-returns-not-game-init'
    )
    $registeredTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($registrationSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($registeredTrace)))
    $registrationOut=Verify 'CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedRegistered=('OWNER_CLIENT_SURFACEPROPS_REGISTRATION_RESULTS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'original-surfaceprops-register-two-calls-returned:2:2:11:22')
    if(-not ($registrationOut -ccontains $expectedRegistered) -or
       -not ($registrationOut -ccontains $expectedProps) -or
       -not ($registrationOut -ccontains $expectedPhysics)) {
        throw 'Properly ordered opt-in surface registration return was not classified'
    }
    # Phase 516: Source SDK 2004 surface count + name index lookup
    # is a distinct downstream, read-only ENGINE call after two actual
    # ParseSurfaceData invocations. Values are invented fixture data.
    $readbackSteps=[string[]]@(
        '[SiN independent] role=client phase=engine-fs-readback-entered version=VPhysicsSurfaceProps001:slots2-3:read-only'
        '[SiN independent] role=client phase=engine-fs-readback-result version=original-physics-surface-readback-three-in-range:93:0:1:84'
        '[SiN independent] role=client phase=engine-fs-readback-terminal-stop version=SDK2004-count-and-indices-not-game-init'
    )
    $fullReadbackTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($registrationSteps)+
        @($readbackSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($fullReadbackTrace)))
    $readbackOut=Verify 'CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedReadback=('OWNER_CLIENT_SURFACEPROPS_READBACK_RESULTS: '+
        'sin-independent-'+$session+'-13124.log:client:'+
        'original-physics-surface-readback-three-in-range:93:0:1:84')
    if(-not ($readbackOut -ccontains $expectedReadback) -or
       -not ($readbackOut -ccontains $expectedRegistered)) {
        throw 'Valid SDK2004 count/index data not classified after authentic synthetic registration sequence'
    }
    $duplicateReadback=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($registrationSteps)+
        @($readbackSteps)+@($readbackSteps[1])+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($duplicateReadback)))
    $duplicateReadbackOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($duplicateReadbackOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_READBACK_RESULTS:')) {
        throw 'Duplicate SDK2004 count/index result was trusted'
    }
    $forgedReadback=@($readbackSteps)
    $forgedReadback[1]=$forgedReadback[1].Replace(
        'three-in-range:93:0:1:84','three-in-range:93:0:1:93')
    $badReadbackTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($registrationSteps)+
        @($forgedReadback)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($badReadbackTrace)))
    $forgedReadbackOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($forgedReadbackOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_READBACK_RESULTS:')) {
        throw 'Surface index outside returned count accepted'
    }
    $earlyReadbackTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($readbackSteps)+
        @($registrationSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($earlyReadbackTrace)))
    $earlyReadbackOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($earlyReadbackOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_READBACK_RESULTS:')) {
        throw 'SDK2004 readback accepted before registration method returned'
    }
    $registerOnlyTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($registrationSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($registerOnlyTrace)))
    $registerOnlyOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($registerOnlyOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_READBACK_RESULTS:') -or
       -not ($registerOnlyOut -ccontains $expectedRegistered)) {
        throw 'Registration-only diagnostic was confused with optional SDK2004 readback'
    }
    $duplicateRegistration=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($registrationSteps)+
        @($registrationSteps[1])+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($duplicateRegistration)))
    $duplicateRegistrationOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($duplicateRegistrationOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_REGISTRATION_RESULTS:')) {
        throw 'Duplicate surface registration result was elevated to owner evidence'
    }
    $forgedRegistration=@($registrationSteps)
    $forgedRegistration[1]=$forgedRegistration[1].Replace(
        'two-calls-returned:2:2:11:22','two-calls-returned:2:1:11:22')
    $forgedRegistrationTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($forgedRegistration)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($forgedRegistrationTrace)))
    $forgedRegistrationOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($forgedRegistrationOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_REGISTRATION_RESULTS:')) {
        throw 'Two physics parse returns were forged with only one call'
    }
    $prematureRegistration=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($registrationSteps)+@($surfacePropsSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($prematureRegistration)))
    $prematureRegistrationOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($prematureRegistrationOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_REGISTRATION_RESULTS:')) {
        throw 'Registration accepted before validated physics surface resource reads'
    }
    $missingResources=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($registrationSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($missingResources)))
    $withoutResources=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($withoutResources -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_REGISTRATION_RESULTS:')) {
        throw 'Registration accepted without original surface resource shape proof'
    }
    # Restoring the read-only fixture shows zero registration evidence.
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($fullTrace)))
    $readOnlyOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($readOnlyOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_REGISTRATION_RESULTS:')) {
        throw 'The original read-only experiment was classified as a mutating surface registration'
    }
    $refusedProps=@($surfacePropsSteps)
    $refusedProps[1]=$refusedProps[1].Replace(
        'original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0',
        'original-surfaceprops-retail-resource-refused:2:1:0:0:0')
    $refusedTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($refusedProps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($refusedTrace)))
    $refusedPropsOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(-not (($refusedPropsOut -join '|').Contains(
        'OWNER_CLIENT_SURFACEPROPS_RESULTS: ')) -or
       (($refusedPropsOut -join '|').Contains($expectedProps))) {
        throw 'Original surfaceprops source read refusal not classified separately'
    }
    $forgedProps=@($surfacePropsSteps)
    $forgedProps[1]=$forgedProps[1].Replace(
        'original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0',
        'original-surfaceprops-retail-resource-shape-verified:2:2:93:70:0')
    $forgedTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($forgedProps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($forgedTrace)))
    $forgedPropsOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($forgedPropsOut -join '|').Contains(
        'OWNER_CLIENT_SURFACEPROPS_RESULTS:')) {
        throw 'Fabricated 93/70 surfaceprops was accepted as retail 93/71'
    }
    $duplicateProps=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+
        @($surfacePropsSteps)+@($surfacePropsSteps[1])+
        @($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($duplicateProps)))
    $duplicatePropsOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($duplicatePropsOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_RESULTS:')) {
        throw 'Duplicate original surfaceprops result was promoted as proof'
    }
    $earlyProps=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($surfacePropsSteps)+
        @($physicsSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($earlyProps)))
    $earlyPropsOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($earlyPropsOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_RESULTS:')) {
        throw 'Surfaceprops result was accepted before the three physics names'
    }
    $missingPhysics=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($surfacePropsSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($missingPhysics)))
    $noPhysicsOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($noPhysicsOut -join '|').Contains('OWNER_CLIENT_SURFACEPROPS_RESULTS:')) {
        throw 'Surfaceprops result accepted without original physics Factory'
    }
    $forgedPhysics=@($physicsSteps)
    $forgedPhysics[1]=$forgedPhysics[1].Replace(
        'original-physics-three-available:3:3:1',
        'original-physics-three-available:3:2:1')
    $forgedTrace=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($forgedPhysics)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($forgedTrace)))
    $forgedOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($forgedOut -join '|').Contains('OWNER_CLIENT_PHYSICS_FACTORY_RESULTS:')) {
        throw 'Forged original physics success with only 2/3 sources accepted'
    }
    $duplicatePhysics=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($physicsSteps)+@($physicsSteps[1])+
        @($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($duplicatePhysics)))
    $duplicateOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($duplicateOut -join '|').Contains('OWNER_CLIENT_PHYSICS_FACTORY_RESULTS:')) {
        throw 'Duplicate original physics factory result promoted to owner evidence'
    }
    $earlyPhysics=[string[]](@($soundscapeSteps[0..3])+
        @($physicsSteps)+@($soundscapeCvarSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($earlyPhysics)))
    $earlyOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($earlyOut -join '|').Contains('OWNER_CLIENT_PHYSICS_FACTORY_RESULTS:')) {
        throw 'Physics Factory was accepted before the required owner ConVar stage'
    }
    $missingPriorCvar=[string[]](@($soundscapeSteps[0..3])+
        @($physicsSteps)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($missingPriorCvar)))
    $missingPriorOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($missingPriorOut -join '|').Contains('OWNER_CLIENT_PHYSICS_FACTORY_RESULTS:')) {
        throw 'Physics Factory was accepted without original ConVar evidence'
    }
    $duplicatedCvar=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeCvarSteps)+@($soundscapeCvarSteps[1])+
        @($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($duplicatedCvar)))
    $dupCvarOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($dupCvarOut -join '|').Contains('OWNER_CLIENT_SOUNDSCAPE_CVAR_LOOKUP_RESULTS:')) {
        throw 'Duplicated original CVar lookup event accepted'
    }
    $forgedCvar=@($soundscapeCvarSteps)
    $forgedCvar[1]=$forgedCvar[1].Replace(
        'original-soundscape-both-vars-cached:2:2:1:1',
        'original-soundscape-both-vars-cached:2:2:0:1')
    $forgedTrace=[string[]](@($soundscapeSteps[0..3])+
        @($forgedCvar)+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($forgedTrace)))
    $forgedCvarOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($forgedCvarOut -join '|').Contains('OWNER_CLIENT_SOUNDSCAPE_CVAR_LOOKUP_RESULTS:')) {
        throw 'Inconsistent original soundscape ConVar cached status accepted'
    }
    $wrongOrder=[string[]](@($soundscapeCvarSteps)+@($soundscapeSteps))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($wrongOrder)))
    $earlyCvarOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($earlyCvarOut -join '|').Contains('OWNER_CLIENT_SOUNDSCAPE_CVAR_LOOKUP_RESULTS:')) {
        throw 'Premature ConVar lookup was promoted to same-session soundscape proof'
    }
    # Diagnostic fields are strictly advisory. An invalid encoded
    # value or duplicate detail must not appear in the trusted verdict
    # even if the preceding soundscape count/result prefix was valid.
    $unsafeDetail=@($soundscapeSteps)
    $unsafeDetail[3]=$unsafeDetail[3].Replace(
        'complete:owned-engine-soundscape-catalog-ready-no-game-init:',
        'scripts/retail-secret.txt:'
    )
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($unsafeDetail)))
    $unsafeOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($unsafeOut -join '|').Contains('OWNER_CLIENT_SOUNDSCAPE_DETAILS:')) {
        throw 'Path-containing diagnostic was accepted into text-only summary'
    }
    $duplicateDetail=[string[]](@($soundscapeSteps[0..3])+
        @($soundscapeSteps[3])+@($soundscapeSteps[4]))
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($duplicateDetail)))
    $duplicateOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($duplicateOut -join '|').Contains('OWNER_CLIENT_SOUNDSCAPE_DETAILS:')) {
        throw 'Duplicate soundscape diagnosis accepted'
    }
    $wrongSoundscape=@($soundscapeSteps)
    $wrongSoundscape[1]=$wrongSoundscape[1].Replace('7:301:242:3','7:300:242:3')
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($wrongSoundscape)))
    $refused=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($refused -join '|').Contains('OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS:')) {
        throw 'Invalid original soundscape counts passed strict trace classifier'
    }
    $wrongSoundscape=@($soundscapeSteps)
    $wrongSoundscape[2]=$wrongSoundscape[2].Replace(
        'owned-soundscape-catalog-verified-original-files',
        'unknown-success')
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($wrongSoundscape)))
    $refused=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($refused -join '|').Contains('OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS:')) {
        throw 'Unknown soundscape report accepted by session classifier'
    }
    # No prior hook proof is never a valid soundscape milestone.
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($soundscapeSteps)))
    $refused=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($refused -join '|').Contains('OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS:')) {
        throw 'Soundscape catalog result promoted without owned Stats hooks'
    }
    $wrongIdsForSoundscape=@($hookSteps)
    $wrongIdsForSoundscape[1]=$wrongIdsForSoundscape[1].Replace('27:28:5','27:5:28')
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($wrongIdsForSoundscape)+@($soundscapeSteps)))
    $refused=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($refused -join '|').Contains('OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS:')) {
        throw 'Swapped prior hook ID trace improperly unlocked soundscape milestone'
    }
    # Refusal is distinct from success. The outcome is not promoted
    # to the verified 7/301/242/3 line by the gate.
    $refusedSoundscape=@($soundscapeSteps)
    $refusedSoundscape[2]=$refusedSoundscape[2].Replace(
        'owned-soundscape-catalog-verified-original-files',
        'owned-soundscape-original-counts-refused')
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($hookSteps)+@($refusedSoundscape)))
    $refused=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($refused -join '|').Contains($expectedSoundscape)) {
        throw 'Refused soundscape state classified as verified retail milestone'
    }
    $wrongIds=@($hookSteps)
    $wrongIds[1]=$wrongIds[1].Replace('27:28:5','27:5:28')
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($stagedReadPrefix)+@($stagedSteps)+@($wrongIds)))
    $refused=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($refused -join '|').Contains('OWNER_CLIENT_ENGINE_FS_HOOK_RESULTS:')) {
        throw 'Swapped owned user-message IDs improperly promoted as success'
    }
    [IO.File]::WriteAllLines($retailReplay,
        [string[]](@($engineFsPrefix)+@($hookSteps)))
    $refused=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($refused -join '|').Contains('OWNER_CLIENT_ENGINE_FS_HOOK_RESULTS:')) {
        throw 'User-message hook proof promoted without prior staged resources'
    }
    # Reject impossible "success" if counts were forged/mismatched.
    $badStages=@($stagedSteps)
    $badStages[1]=$badStages[1].Replace('66:62:8:156:66','66:61:8:156:66')
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($stagedReadPrefix)+@($badStages)))
    $out=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($out -join '|').Contains('OWNER_CLIENT_ENGINE_FS_STAGED_RESULTS:')) {
        throw 'Forged original-mode staged result promoted despite wrong counts'
    }
    # No same-log combined read means no staged milestone, even if its own
    # tail claims original resource modes and all nominal source counts.
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($engineFsPrefix)+@($stagedSteps)))
    $out=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($out -join '|').Contains('OWNER_CLIENT_ENGINE_FS_STAGED_RESULTS:')) {
        throw 'Original-mode staged result promoted without filesystem prerequisites'
    }
    # Do not claim an engine-filesystem milestone after a false sound Init.
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($soundInitPrefix)+@(
        $soundInitEntry,
        '[SiN independent] role=client phase=sound-system-init-result version=called-false',
        $engineFsStart,
        '[SiN independent] role=client phase=engine-fs-presence-result version=both-present',
        $engineFsStop
    )))
    $out=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($out -join '|').Contains('OWNER_CLIENT_ENGINE_FS_PRESENCE_RESULTS:')) {
        throw 'False sound Init improperly promoted engine filesystem evidence'
    }
    # Do not accept a read verdict that lacks its presence predecessor.
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($engineFsPrefix)+@(
        $engineFsReadStart,
        '[SiN independent] role=client phase=engine-fs-read-counts version=62:8:156:66',
        '[SiN independent] role=client phase=engine-fs-read-result version=parsed-and-verified',
        $engineFsStop
    )))
    $out=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($out -join '|').Contains('OWNER_CLIENT_ENGINE_FS_READ_RESULTS:')) {
        throw 'Read result promoted despite missing filesystem presence'
    }
    # Do not accept a forged successful count pairing.
    [IO.File]::WriteAllLines($retailReplay,[string[]](@($engineFsPrefix)+@(
        $engineFsStart,
        '[SiN independent] role=client phase=engine-fs-presence-result version=both-present',
        $engineFsReadStart,
        '[SiN independent] role=client phase=engine-fs-read-counts version=0:0:0:0',
        '[SiN independent] role=client phase=engine-fs-read-result version=parsed-and-verified',
        $engineFsStop
    )))
    $out=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($out -join '|').Contains('OWNER_CLIENT_ENGINE_FS_READ_RESULTS:')) {
        throw 'Read result promoted despite invalid original counts'
    }
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
    # Phase 517: the server's source-authenticated three physics names
    # are a separate real-engine factory stage, not a playable DLLInit.
    Remove-Item -LiteralPath $client -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $retailReplay -Force -ErrorAction SilentlyContinue
    $serverPhysicsPrefix=[string[]]@(
        '[SiN independent] role=server phase=factory-accepted version=ServerGameDLL003',
        '[SiN independent] role=server phase=init-entered version=<none>'
    )
    $serverPhysicsSteps=[string[]]@(
        '[SiN independent] role=server phase=server-physics-entered version=original-server-second-factory:read-only',
        '[SiN independent] role=server phase=server-physics-result version=original-server-physics-three-available:3:3:1',
        '[SiN independent] role=server phase=server-physics-terminal-stop version=no-manifest-coupling-no-server-init'
    )
    [IO.File]::WriteAllLines($server,[string[]](@($serverPhysicsPrefix)+@($serverPhysicsSteps)))
    $serverPhysicsOut=Verify 'SERVER_PHYSICS_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
    $expectedServerPhysics=('OWNER_SERVER_ORIGINAL_PHYSICS_FACTORY_RESULTS: '+
        'sin-independent-'+$session+'-303.log:server:original-server-physics-three-available:3:3:1')
    if(-not ($serverPhysicsOut -ccontains $expectedServerPhysics)) {
        throw 'Original server physics 3/3 was not classified after correct DLLInit'
    }
    foreach($failure in @('missing-first:1:0:0','missing-second:2:1:0','missing-third:3:2:0','missing-factory:0:0:0')) {
        $missing=@($serverPhysicsSteps)
        $missing[1]=$missing[1].Replace('three-available:3:3:1',$failure)
        [IO.File]::WriteAllLines($server,[string[]](@($serverPhysicsPrefix)+@($missing)))
        $failedOut=Verify 'SERVER_PHYSICS_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF'
        if(($failedOut -join '|').Contains($expectedServerPhysics) -or
           -not (($failedOut -join '|').Contains("original-server-physics-$failure"))) {
            throw ('Original server factory failure not distinctly recorded: '+$failure)
        }
    }
    [IO.File]::WriteAllLines($server,[string[]](@($serverPhysicsPrefix)+
        @($serverPhysicsSteps)+@($serverPhysicsSteps[1])))
    $duplicateServerOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($duplicateServerOut -join '|').Contains('OWNER_SERVER_ORIGINAL_PHYSICS_FACTORY_RESULTS:')) {
        throw 'Duplicate server physics result elevated to owner evidence'
    }
    [IO.File]::WriteAllLines($server,[string[]](@($serverPhysicsPrefix)+
        @($serverPhysicsSteps[0])+
        @('[SiN independent] role=server phase=server-physics-result version=original-server-physics-three-available:3:2:1')+
        @($serverPhysicsSteps[2])))
    $forgedServerOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($forgedServerOut -join '|').Contains('OWNER_SERVER_ORIGINAL_PHYSICS_FACTORY_RESULTS:')) {
        throw 'Inconsistent 2/3 server physics success was accepted'
    }
    [IO.File]::WriteAllLines($server,[string[]]@($serverPhysicsSteps))
    $missingInitOut=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($missingInitOut -join '|').Contains('OWNER_SERVER_ORIGINAL_PHYSICS_FACTORY_RESULTS:')) {
        throw 'Server physics stage accepted without earlier same-log DLLInit'
    }
    # Phase520: role-specific server manifest/2-script evidence is
    # accepted only after the complete 3/3 original server physics
    # Factory sequence, never from a standalone or forged resource line.
    $serverResourceSteps=[string[]]@(
        '[SiN independent] role=server phase=server-surfaceprops-entered version=VFileSystem012:server-original:rb-dual-scope',
        '[SiN independent] role=server phase=server-surfaceprops-result version=original-server-surfaceprops-two-sources-verified:2:2:93:71:0',
        '[SiN independent] role=server phase=server-surfaceprops-terminal-stop version=read-only-manifest-no-server-init'
    )
    $fullServerRead=[string[]](@($serverPhysicsPrefix)+
        @($serverPhysicsSteps)+@($serverResourceSteps))
    [IO.File]::WriteAllLines($server,$fullServerRead)
    $resourceOut=@(& $Classifier -Session $session -LogDirectory $dir)
    $expectedServerResource='OWNER_SERVER_ORIGINAL_SURFACEPROPS_READ_RESULTS: '+
        'sin-independent-'+$session+'-303.log:server:original-server-surfaceprops-two-sources-verified:2:2:93:71:0'
    if(-not ($resourceOut -ccontains $expectedServerResource) -or
       -not ($resourceOut -ccontains $expectedServerPhysics)) {
        throw 'Authenticated original server 2-source read denied after complete Factory 3/3'
    }
    # Duplicate result, wrong counts or missing Factory must never
    # produce the trusted owner surface read classification.
    foreach($bad in @(
        [string[]](@($serverPhysicsPrefix)+@($serverResourceSteps)),
        [string[]](@($serverPhysicsPrefix)+@($serverPhysicsSteps)+@($serverResourceSteps)+@($serverResourceSteps[1])),
        [string[]](@($serverPhysicsPrefix)+@($serverResourceSteps)+@($serverPhysicsSteps))
    )) {
        [IO.File]::WriteAllLines($server,[string[]]$bad)
        $negative=@(& $Classifier -Session $session -LogDirectory $dir)
        if(($negative -join '|').Contains('OWNER_SERVER_ORIGINAL_SURFACEPROPS_READ_RESULTS:')) {
            throw 'Forged/missing/out-of-order server surface read elevated to original proof'
        }
    }
    $badResource=@($serverResourceSteps)
    $badResource[1]=$badResource[1].Replace('2:2:93:71:0','2:2:93:72:0')
    [IO.File]::WriteAllLines($server,[string[]](@($serverPhysicsPrefix)+
        @($serverPhysicsSteps)+@($badResource)))
    $wrongCount=@(& $Classifier -Session $session -LogDirectory $dir)
    if(($wrongCount -join '|').Contains('OWNER_SERVER_ORIGINAL_SURFACEPROPS_READ_RESULTS:')) {
        throw 'Inconsistent server surface properties count was trusted'
    }
    [IO.File]::WriteAllLines($server,[string[]](@($serverPhysicsPrefix)+
        @($serverPhysicsSteps)+@(
            $serverResourceSteps[0],
            '[SiN independent] role=server phase=server-surfaceprops-result version=original-server-surfaceprops-resource-refused:2:1:80:62:0',
            $serverResourceSteps[2])))
    $refused=@(& $Classifier -Session $session -LogDirectory $dir)
    if(-not (($refused -join '|').Contains('original-server-surfaceprops-resource-refused:2:1:80:62:0'))) {
        throw 'Explicit original server resource refusal was not retained'
    }
    Write-Output "PASS owner session classifier: cross-session, game-module factory entry, native Init blocker, two roles, no false retail boot"
} finally {
    Remove-Item -LiteralPath $dir -Recurse -Force -ErrorAction SilentlyContinue
}
