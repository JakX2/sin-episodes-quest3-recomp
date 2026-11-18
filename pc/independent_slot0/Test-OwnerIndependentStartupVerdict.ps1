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
