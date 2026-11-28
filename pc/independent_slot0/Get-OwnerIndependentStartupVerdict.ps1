# Offline classifier for ONE opt-in independent Windows game-launch session.
# Consumes only owner-local diagnostic logs; never executes or reads game code.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{32}$')][string]$Session,
    [Parameter(Mandatory=$true)][string]$LogDirectory
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not (Test-Path -LiteralPath $LogDirectory -PathType Container)) {
    throw "Missing startup trace directory"
}
$logs = @(Get-ChildItem -LiteralPath $LogDirectory -File -Filter ("sin-independent-{0}-*.log" -f $Session) |
    Where-Object { $_.Name -cmatch ("^sin-independent-{0}-[0-9]+\.log$" -f $Session) } |
    Sort-Object Name)
$seen = @{}
$issues = New-Object System.Collections.Generic.List[string]
$requests = New-Object System.Collections.Generic.List[string]
$factoryResults = New-Object System.Collections.Generic.List[string]
$predictionFactoryEvents = New-Object System.Collections.Generic.List[string]
$predictionConVarEvents = New-Object System.Collections.Generic.List[string]
$entityListFactoryEvents = New-Object System.Collections.Generic.List[string]
$callbackEvents = 0
$unsupportedCallbacks = New-Object System.Collections.Generic.List[string]
$soundCallbacks = New-Object System.Collections.Generic.List[string]
$serverPhysicsLogs = New-Object System.Collections.Generic.List[string]
$serverSurfacePropsLogs = New-Object System.Collections.Generic.List[string]
$soundTerminalLogs = New-Object System.Collections.Generic.List[string]
$clientSoundTerminalLogs = New-Object System.Collections.Generic.List[string]
$clientMaterialTerminalLogs = New-Object System.Collections.Generic.List[string]
$clientScreenTerminalLogs = New-Object System.Collections.Generic.List[string]
$gaussianTerminalLogs = New-Object System.Collections.Generic.List[string]
$clientCvarTerminalLogs = New-Object System.Collections.Generic.List[string]
$ownedStartupLogs = New-Object System.Collections.Generic.List[string]
$vguiLogs = New-Object System.Collections.Generic.List[string]
$vguiLayoutLogs = New-Object System.Collections.Generic.List[string]
$engineVGuiSlot1Logs = New-Object System.Collections.Generic.List[string]
$vguiControlsLogs = New-Object System.Collections.Generic.List[string]
$matSurfaceLogs = New-Object System.Collections.Generic.List[string]
$soundSystemInitLogs = New-Object System.Collections.Generic.List[string]
$statsAssetProbeLogs = New-Object System.Collections.Generic.List[string]
$engineFsPresenceLogs = New-Object System.Collections.Generic.List[string]
$engineFsReadLogs = New-Object System.Collections.Generic.List[string]
$engineFsStagedLogs = New-Object System.Collections.Generic.List[string]
$engineFsHooksLogs = New-Object System.Collections.Generic.List[string]
$engineFsSoundscapeLogs = New-Object System.Collections.Generic.List[string]
$engineFsSoundscapeDetailLogs = New-Object System.Collections.Generic.List[string]
$engineFsSoundscapeCvarLogs = New-Object System.Collections.Generic.List[string]
$engineFsPhysicsFactoryLogs = New-Object System.Collections.Generic.List[string]
$engineFsSurfacePropsLogs = New-Object System.Collections.Generic.List[string]
$engineFsRegistrationLogs = New-Object System.Collections.Generic.List[string]
$engineFsReadbackLogs = New-Object System.Collections.Generic.List[string]
$engineFsMaterialLogs = New-Object System.Collections.Generic.List[string]
$vguiControlsNames=@('VGUI_ivgui006','VGUI_Panel009','VGUI_Surface028','VGUI_Scheme010','VGUI_System010','VGUI_Input005','VGUI_Localize003','VFileSystem012')
$vguiEvents=@('vgui-probe-entered:vgui2.dll','vgui-provider-found:vgui2.dll','vgui-query-entered:VGUI_InputInternal001','vgui-query-found:VGUI_InputInternal001','vgui-query-entered:VGUI_System010','vgui-query-found:VGUI_System010')
$ownedStartupEvents = @('owned-startup-entered:independent-only','owned-startup-objects-ready:1','owned-startup-pool-ready:2048x96','owned-startup-pool-verified:allocate-reuse-release','owned-startup-cleanup-verified:pool-objects')
$clientCvarNames = @('cl_starfield_distance','cl_starfield_diameter','cl_show_bloodspray')
$eventTerminalLogs = New-Object System.Collections.Generic.List[string]
$cvarTerminalLogs = New-Object System.Collections.Generic.List[string]
$batchTerminalLogs = New-Object System.Collections.Generic.List[string]
$batchNames = @(
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
)
$explicitInitRoles = @{}
$legacyInitEvents = 0
$blockersByLog = New-Object System.Collections.Generic.List[string]
foreach ($log in $logs) {
    $logFirstBlocker = $null
    $soundEntered = $false
    $serverInitEnteredInLog=$false
    $serverPhysicsStage=0
    $serverPhysicsEvents=0
    $serverPhysicsResult=$null
    $serverSurfacePropsStage=0
    $serverSurfacePropsEvents=0
    $serverSurfacePropsResult=$null
    $clientSoundEntered = $false
    $clientSoundResult = $null
    $clientMaterialStage = 0
    $clientMaterialResult = $null
    $clientHardwareFound = $false
    $screenStage = 0
    $screenDimensions = $null
    $screenResult = $null
    $gaussianStage = 0
    $gaussianResult = $null
    $clientCvarStage = 0
    $clientCvarVerified = 0
    $clientCvarResult = $null
    $ownedStartupStage = 0
    $ownedStartupResult = $null
    $vguiStage=0
    $vguiResult=$null
    $vguiLayoutStage=0
    $vguiLayoutResult=$null
    $engineVGuiSlot1Stage=0
    $engineVGuiSlot1Result=$null
    $vguiControlsStage=0
    $vguiControlsIndex=0
    $vguiControlsMissing=0
    $vguiControlsInitReturn=$null
    $vguiControlsResult=$null
    $matSurfaceStage=0
    $matSurfaceResult=$null
    $soundSystemInitStage=0
    $soundSystemInitResult=$null
    $statsAssetStage=0
    $statsAssetCounts=$null
    $statsAssetResult=$null
    $engineFsStage=0
    $engineFsPresence=$null
    $engineFsRead=$null
    $engineFsCounts=$null
    $engineFsStagedStage=0
    $engineFsStagedCounts=$null
    $engineFsStagedSteps=$null
    $engineFsStagedResult=$null
    $engineFsHooksStage=0
    $engineFsHookIds=$null
    $engineFsHookCount=$null
    $engineFsHookResult=$null
    $soundscapeStage=0
    $soundscapeCounts=$null
    $soundscapeResult=$null
    $soundscapeDetail=$null
    $soundscapeDetailCount=0
    $soundscapeCvarStage=0
    $soundscapeCvarEvents=0
    $soundscapeCvarResult=$null
    $physicsStage=0
    $physicsEventCount=0
    $physicsResult=$null
    $surfacePropsStage=0
    $surfacePropsEventCount=0
    $surfacePropsResult=$null
    $materialStage=0
    $materialEventCount=0
    $materialResult=$null
    $materialSampleCount=0
    $materialCount=-1
    $registrationStage=0
    $registrationEventCount=0
    $registrationResult=$null
    $readbackStage=0
    $readbackEventCount=0
    $readbackResult=$null
    $soundInitSeenEvents=New-Object System.Collections.Generic.List[string]
    $soundResult = $null
    $eventStage = 0
    $eventResult = $null
    $cvarStage = 0
    $cvarResult = $null
    $batchVerified = 0
    $batchStage = 0
    $batchResult = $null
    foreach ($line in (Get-Content -LiteralPath $log.FullName)) {
        if ($line -notmatch '^\[SiN independent\] role=(client|server) phase=([a-z0-9-]+) version=([^\s]+)$') {
            continue
        }
        $role = $Matches[1]
        $phase = $Matches[2]
        $version = $Matches[3]
        $seen[$role] = $true
        if ($phase -eq 'factory-accepted' -or $phase -eq 'factory-rejected') {
            $factoryResults.Add(("{0}:{1}:{2}" -f $role,$phase,$version))
        } else {
            $callbackEvents++
        }
        if ($phase -eq 'unsupported-callback') {
            $unsupportedCallbacks.Add(("{0}:{1}:{2}" -f $log.Name,$role,$version))
        }
        if ($role -eq 'client' -and
            $phase -in @('prediction-bound','prediction-bound-refused')) {
            $predictionFactoryEvents.Add(("{0}:client:{1}:{2}" -f
                $log.Name,$phase,$version))
        }
        if ($role -eq 'client' -and
            $phase -in @('prediction-convar-registered',
                          'prediction-convar-register-refused')) {
            $predictionConVarEvents.Add(("{0}:client:{1}:{2}" -f
                $log.Name,$phase,$version))
        }
        if ($role -eq 'client' -and
            $phase -in @('client-entitylist-empty-bound',
                          'client-entitylist-empty-refused')) {
            $entityListFactoryEvents.Add(("{0}:client:{1}:{2}" -f
                $log.Name,$phase,$version))
        }
        if ($role -eq 'client' -and $version -eq 'VSoundEmitter001') {
            if ($phase -eq 'sound-connect-entered') {
                $clientSoundEntered = $true
                $clientSoundResult = $null
            } elseif ($clientSoundEntered -and $phase -cin @('sound-connect-succeeded','sound-connect-rejected')) {
                $clientSoundResult = $phase
            } elseif ($clientSoundEntered -and $null -ne $clientSoundResult -and $phase -eq 'sound-probe-terminal-stop') {
                $clientSoundTerminalLogs.Add(("{0}:client:{1}" -f $log.Name,$clientSoundResult))
                $clientSoundEntered = $false
                $clientSoundResult = $null
            }
        }
        if ($role -eq 'client' -and $clientSoundResult -eq 'sound-connect-succeeded') {
            if ($clientMaterialStage -eq 0 -and $phase -eq 'textmode-checked' -and $version -cin @('present','absent')) {
                $clientMaterialStage = 1
            } elseif ($clientMaterialStage -eq 1 -and $phase -eq 'optional-request' -and $version -eq 'VMaterialSystemStub001') {
                $clientMaterialStage = 2
            } elseif ($clientMaterialStage -eq 2 -and $phase -cin @('optional-found','optional-missing') -and $version -eq 'VMaterialSystemStub001') {
                $clientMaterialStage = 3
            } elseif ($clientMaterialStage -eq 3 -and $phase -eq 'hardware-config-entered' -and $version -eq 'MaterialSystemHardwareConfig012') {
                $clientMaterialStage = 4
            } elseif ($clientMaterialStage -eq 4 -and $phase -cin @('hardware-config-found','hardware-config-missing') -and $version -eq 'MaterialSystemHardwareConfig012') {
                $clientHardwareFound = $phase -eq 'hardware-config-found'
                $clientMaterialStage = 5
            } elseif ($phase -eq 'material-probe-result') {
                if (($version -eq 'acquired' -and $clientMaterialStage -eq 5 -and $clientHardwareFound) -or
                    ($version -eq 'missing-hardware' -and $clientMaterialStage -eq 5 -and -not $clientHardwareFound) -or
                    ($version -eq 'invalid-commandline' -and $clientMaterialStage -eq 0) -or
                    ($version -eq 'invalid-material' -and $clientMaterialStage -eq 3)) {
                    $clientMaterialResult = $version
                    $clientMaterialStage = 6
                }
            } elseif ($clientMaterialStage -eq 6 -and $phase -eq 'material-probe-terminal-stop' -and $version -eq 'MaterialSystemHardwareConfig012') {
                $clientMaterialTerminalLogs.Add(("{0}:client:{1}" -f $log.Name,$clientMaterialResult))
                $clientMaterialStage = 7
            }
        }
        if ($role -eq 'client' -and $clientMaterialStage -eq 6 -and $clientMaterialResult -eq 'acquired') {
            if ($screenStage -eq 0 -and $phase -eq 'screen-query-entered' -and $version -eq 'VEngineClient012.GetScreenSize') {
                $screenStage = 1
            } elseif ($screenStage -eq 1 -and $phase -eq 'screen-query-returned' -and $version -cmatch '^(-?[0-9]+):(-?[0-9]+)$') {
                $w = 0; $h = 0
                if ([int]::TryParse($Matches[1],[ref]$w) -and [int]::TryParse($Matches[2],[ref]$h)) {
                    $screenDimensions = $version
                    $screenStage = 2
                }
            } elseif ($phase -eq 'screen-probe-result') {
                if ($version -eq 'returned' -and $screenStage -eq 2) {
                    $screenResult = "returned:$screenDimensions"
                    $screenStage = 3
                } elseif ($version -eq 'missing-abi-input' -and $screenStage -eq 1) {
                    $screenResult = $version
                    $screenStage = 3
                }
            } elseif ($screenStage -eq 3 -and $phase -eq 'screen-probe-terminal-stop' -and $version -eq 'VEngineClient012.GetScreenSize') {
                $clientScreenTerminalLogs.Add(("{0}:client:{1}" -f $log.Name,$screenResult))
                $screenStage = 4
            }
        }
        if ($role -eq 'client' -and $screenStage -eq 3 -and $screenResult -like 'returned:*') {
            if ($gaussianStage -eq 0 -and $phase -eq 'gaussian-attach-entered' -and $version -eq 'VEngineRandom001') {
                $gaussianStage = 1
            } elseif ($gaussianStage -eq 1 -and $phase -eq 'gaussian-attach-verified' -and $version -eq 'VEngineRandom001') {
                $gaussianStage = 2
            } elseif ($phase -eq 'gaussian-probe-result') {
                if (($gaussianStage -eq 2 -and $version -eq 'attached') -or
                    ($gaussianStage -eq 1 -and $version -eq 'layout-or-attach-refused') -or
                    ($gaussianStage -eq 0 -and $version -eq 'missing-abi-input')) {
                    $gaussianResult = $version
                    $gaussianStage = 3
                }
            } elseif ($gaussianStage -eq 3 -and $phase -eq 'gaussian-probe-terminal-stop' -and $version -eq 'VEngineRandom001') {
                $gaussianTerminalLogs.Add(("{0}:client:{1}" -f $log.Name,$gaussianResult))
                $gaussianStage = 4
            }
        }
        if ($role -eq 'client' -and $gaussianStage -eq 3 -and $gaussianResult -eq 'attached') {
            if ($clientCvarStage -eq 0 -and $phase -eq 'client-cvar-preflight-complete' -and $version -eq '3') {
                $clientCvarStage = 1
            } elseif ($clientCvarVerified -lt $clientCvarNames.Count -and $version -ceq $clientCvarNames[$clientCvarVerified]) {
                if ($phase -eq 'client-cvar-register-entered' -and $clientCvarStage -eq 1) { $clientCvarStage = 2 }
                elseif ($phase -eq 'client-cvar-register-returned' -and $clientCvarStage -eq 2) { $clientCvarStage = 3 }
                elseif ($phase -eq 'client-cvar-registration-verified' -and $clientCvarStage -eq 3) {
                    $clientCvarVerified++
                    $clientCvarStage = 1
                }
            } elseif ($phase -eq 'client-cvar-probe-result' -and $version -cmatch '^(verified|invalid-interface|existing-name|invalid-list|bad-owned-object|lookup-mismatch|unexpected-relink):([0-3])/3$') {
                $status = $Matches[1]; $count = [int]$Matches[2]
                if ($count -eq $clientCvarVerified -and
                    ($status -ne 'verified' -or ($clientCvarStage -eq 1 -and $count -eq 3))) {
                    $clientCvarResult = $version
                    $clientCvarStage = 4
                }
            } elseif ($clientCvarStage -eq 4 -and $phase -eq 'client-cvar-probe-terminal-stop' -and $version -eq '3') {
                $clientCvarTerminalLogs.Add(("{0}:client:{1}" -f $log.Name,$clientCvarResult))
                $clientCvarStage = 5
            }
        }
        if ($role -eq 'client' -and $clientCvarStage -eq 4 -and $clientCvarResult -eq 'verified:3/3') {
            if ($ownedStartupStage -lt $ownedStartupEvents.Count -and
                ("${phase}:${version}" -ceq $ownedStartupEvents[$ownedStartupStage])) {
                $ownedStartupStage++
            } elseif ($phase -eq 'owned-startup-probe-result' -and
                (($version -eq 'verified' -and $ownedStartupStage -eq $ownedStartupEvents.Count) -or
                 ($version -eq 'refused' -and $ownedStartupStage -ge 1))) {
                $ownedStartupResult=$version
            } elseif ($null -ne $ownedStartupResult -and $phase -eq 'owned-startup-terminal-stop' -and $version -eq 'independent-only') {
                $ownedStartupLogs.Add(("{0}:client:{1}" -f $log.Name,$ownedStartupResult))
                $clientCvarStage=5
            }
        }
        if ($role -eq 'client' -and $clientCvarStage -eq 4 -and $ownedStartupResult -eq 'verified') {
            if ($vguiStage -lt $vguiEvents.Count -and ("${phase}:${version}" -ceq $vguiEvents[$vguiStage])) {
                $vguiStage++
            } elseif ($phase -eq 'vgui-probe-result' -and (
                ($vguiStage -eq 1 -and $version -in @('missing-module','missing-factory')) -or
                ($vguiStage -eq 3 -and $version -eq 'missing-input') -or
                ($vguiStage -eq 5 -and $version -eq 'missing-system') -or
                ($vguiStage -eq 6 -and $version -in @('acquired','aliased-interfaces')))) {
                $vguiResult=$version
            } elseif ($null -ne $vguiResult -and $phase -eq 'vgui-probe-terminal-stop' -and $version -eq 'factory-only') {
                $vguiLogs.Add(("{0}:client:{1}" -f $log.Name,$vguiResult))
                $clientCvarStage=5
            }
        }
        if ($role -eq 'client' -and $vguiStage -eq 6 -and $vguiResult -eq 'acquired') {
            if ($vguiLayoutStage -eq 0 -and $phase -eq 'vgui-layout-entered' -and
                $version -eq 'input:0+56,system:0+24') {
                $vguiLayoutStage=1
            } elseif ($vguiLayoutStage -eq 1 -and $phase -eq 'vgui-layout-input-verified' -and
                $version -eq '0+56') {
                $vguiLayoutStage=2
            } elseif ($vguiLayoutStage -eq 2 -and $phase -eq 'vgui-layout-system-verified' -and
                $version -eq '0+24') {
                $vguiLayoutStage=3
            } elseif ($phase -eq 'vgui-layout-probe-result' -and (
                ($vguiLayoutStage -eq 1 -and $version -eq 'invalid-input') -or
                ($vguiLayoutStage -eq 2 -and $version -eq 'invalid-system') -or
                ($vguiLayoutStage -eq 3 -and $version -eq 'verified'))) {
                $vguiLayoutResult=$version
                $vguiLayoutStage=4
            } elseif ($vguiLayoutStage -eq 4 -and $phase -eq 'vgui-layout-terminal-stop' -and
                $version -eq 'read-only') {
                $vguiLayoutLogs.Add(("{0}:client:{1}" -f $log.Name,$vguiLayoutResult))
                $vguiLayoutStage=5
            }
        }
        if ($role -eq 'client' -and $vguiLayoutStage -eq 4 -and $vguiLayoutResult -eq 'verified') {
            if ($engineVGuiSlot1Stage -eq 0 -and $phase -eq 'engine-vgui-slot1-entered' -and $version -eq 'VEngineVGui001:arg2') {
                $engineVGuiSlot1Stage=1
            } elseif ($engineVGuiSlot1Stage -eq 1 -and $phase -eq 'engine-vgui-slot1-result' -and
                $version -in @('nonzero','zero','invalid-object','invalid-vtable','invalid-method')) {
                $engineVGuiSlot1Stage=2
                $engineVGuiSlot1Result=$version
            } elseif ($engineVGuiSlot1Stage -eq 2 -and $phase -eq 'engine-vgui-slot1-terminal-stop' -and
                $version -eq 'original-slot1') {
                $engineVGuiSlot1Logs.Add(("{0}:client:{1}" -f $log.Name,$engineVGuiSlot1Result))
                $engineVGuiSlot1Stage=3
            }
        }
        if ($role -eq 'client' -and $vguiLayoutStage -eq 4 -and $vguiLayoutResult -eq 'verified') {
            if ($vguiControlsStage -eq 0 -and $phase -eq 'vgui-controls-bootstrap-entered' -and
                $version -eq 'CLIENT:factory-count-1') {
                $vguiControlsStage=1
            } elseif ($vguiControlsStage -eq 1 -and $vguiControlsIndex -lt $vguiControlsNames.Count -and
                $phase -eq 'vgui-controls-request' -and $version -ceq $vguiControlsNames[$vguiControlsIndex]) {
                $vguiControlsStage=2
            } elseif ($vguiControlsStage -eq 2 -and $version -ceq $vguiControlsNames[$vguiControlsIndex] -and
                $phase -in @('vgui-controls-found','vgui-controls-missing')) {
                if($phase -eq 'vgui-controls-missing') {$vguiControlsMissing++}
                $vguiControlsIndex++
                $vguiControlsStage=1
            } elseif ($vguiControlsStage -eq 1 -and $vguiControlsIndex -eq $vguiControlsNames.Count -and
                $vguiControlsMissing -eq 0 -and $phase -eq 'vgui-controls-init-entered' -and
                $version -eq 'VGUI_ivgui006:factory-count-1') {
                $vguiControlsStage=3
            } elseif ($vguiControlsStage -eq 3 -and $phase -eq 'vgui-controls-init-returned' -and
                $version -in @('true','false')) {
                $vguiControlsStage=4
                $vguiControlsInitReturn=$version
            } elseif ($vguiControlsStage -eq 4 -and $phase -eq 'vgui-controls-bootstrap-result' -and
                $version -eq ('init-'+$vguiControlsInitReturn)) {
                $vguiControlsStage=5
                $vguiControlsResult=$version
            } elseif ($vguiControlsStage -eq 1 -and $vguiControlsIndex -eq $vguiControlsNames.Count -and
                $phase -eq 'vgui-controls-bootstrap-result' -and (
                ($vguiControlsMissing -gt 0 -and $version -eq 'missing-interface') -or
                ($vguiControlsMissing -eq 0 -and $version -in @('invalid-core-object','invalid-core-vtable','invalid-init-method')))) {
                $vguiControlsStage=5
                $vguiControlsResult=$version
            } elseif ($vguiControlsStage -eq 5 -and $phase -eq 'vgui-controls-bootstrap-terminal-stop' -and
                $version -eq 'source2004' -and (
                ($vguiControlsResult -notin @('init-true','init-false')) -or
                ($engineVGuiSlot1Stage -eq 3))) {
                $vguiControlsLogs.Add(("{0}:client:{1}:panel-{2}" -f $log.Name,
                    $vguiControlsResult,$(if($engineVGuiSlot1Stage -eq 3){$engineVGuiSlot1Result}else{'not-reached'})))
                $vguiControlsStage=6
            }
        }
        if ($role -eq 'client' -and $vguiControlsStage -eq 5 -and
            $vguiControlsResult -in @('init-true','init-false') -and
            $engineVGuiSlot1Stage -eq 3 -and $engineVGuiSlot1Result -eq 'nonzero') {
            if ($matSurfaceStage -eq 0 -and $phase -eq 'mat-surface-query-entered' -and
                $version -eq 'VGUI_Surface028:IAppSystem+4') {
                $matSurfaceStage=1
            } elseif ($matSurfaceStage -eq 1 -and $phase -eq 'mat-surface-query-result' -and
                $version -in @('found','missing-interface','invalid-surface',
                               'invalid-appsystem-subobject','invalid-vtable','invalid-method')) {
                $matSurfaceStage=2
                $matSurfaceResult=$version
            } elseif ($matSurfaceStage -eq 2 -and $phase -eq 'mat-surface-query-terminal-stop' -and
                $version -eq 'original-abi') {
                $matSurfaceLogs.Add(("{0}:client:{1}" -f $log.Name,$matSurfaceResult))
                $matSurfaceStage=3
            }
        }
        if($role -eq 'client' -and $phase -like 'mat-surface-*') {
            $soundInitSeenEvents.Add(("mat:$phase=$version;previous=$matSurfaceStage/$matSurfaceResult;control=$vguiControlsStage/$vguiControlsResult;panel=$engineVGuiSlot1Stage/$engineVGuiSlot1Result"))
        }
        if($role -eq 'client' -and $phase -like 'sound-system-init-*') {
            $soundInitSeenEvents.Add(("$phase=$version;mat=$matSurfaceStage/$matSurfaceResult;control=$vguiControlsStage/$vguiControlsResult;panel=$engineVGuiSlot1Stage/$engineVGuiSlot1Result"))
        }
        if ($role -eq 'client' -and $matSurfaceStage -eq 2 -and
            $matSurfaceResult -eq 'found') {
            if ($soundSystemInitStage -eq 0 -and
                $phase -eq 'sound-system-init-entered' -and
                $version -eq 'CSoundEmitterSystem:VSoundEmitter001+1') {
                $soundSystemInitStage=1
            } elseif ($soundSystemInitStage -eq 1 -and
                $phase -eq 'sound-system-init-result' -and
                $version -in @('called-true','called-false','invalid-object',
                               'invalid-vtable','invalid-method')) {
                $soundSystemInitStage=2
                $soundSystemInitResult=$version
            } elseif ($soundSystemInitStage -eq 2 -and
                $phase -eq 'sound-system-init-terminal-stop' -and
                $version -eq 'original-slot1') {
                $soundSystemInitLogs.Add(("{0}:client:{1}" -f $log.Name,$soundSystemInitResult))
                $soundSystemInitStage=3
            }
        }
        if ($role -eq 'client' -and $soundSystemInitStage -eq 2 -and
            $soundSystemInitResult -eq 'called-true') {
            if ($statsAssetStage -eq 0 -and
                $phase -eq 'stats-asset-probe-entered' -and
                $version -eq 'scripts/stats') {
                $statsAssetStage=1
            } elseif ($statsAssetStage -eq 1 -and
                $phase -eq 'stats-asset-probe-counts' -and
                $version -cmatch '^([0-9]{1,4}:){3}[0-9]{1,4}$') {
                $statsAssetStage=2
                $statsAssetCounts=$version
            } elseif ($statsAssetStage -eq 2 -and
                $phase -eq 'stats-asset-probe-result' -and
                (($version -eq 'verified-original-counts' -and
                  $statsAssetCounts -eq '62:8:156:66') -or
                 ($version -eq 'unexpected-retail-counts' -and
                  $statsAssetCounts -ne '62:8:156:66') -or
                 ($version -in @('missing-absolute-root','asset-read-or-parse-refused') -and
                  $statsAssetCounts -eq '0:0:0:0'))) {
                $statsAssetStage=3
                $statsAssetResult=$version
            } elseif ($statsAssetStage -eq 3 -and
                $phase -eq 'stats-asset-probe-terminal-stop' -and
                $version -eq 'independent-content-io') {
                $statsAssetProbeLogs.Add(('{0}:client:{1}:{2}' -f $log.Name,
                    $statsAssetResult,$statsAssetCounts))
                $statsAssetStage=4
            }
        }
        if ($role -eq 'client' -and $soundSystemInitStage -eq 2 -and
            $soundSystemInitResult -eq 'called-true') {
            if ($engineFsStage -eq 0 -and
                $phase -eq 'engine-fs-presence-entered' -and
                $version -eq 'VFileSystem012:source2004') {
                $engineFsStage=1
            } elseif ($engineFsStage -eq 1 -and
                $phase -eq 'engine-fs-presence-result' -and
                $version -in @('both-present','missing-definitions',
                               'missing-layout','invalid-abi')) {
                $engineFsPresence=$version
                $engineFsStage=2
            } elseif ($engineFsStage -eq 2 -and
                $engineFsPresence -eq 'both-present' -and
                $phase -eq 'engine-fs-read-entered' -and
                $version -eq 'VFileSystem012:read-only') {
                $engineFsStage=3
            } elseif ($engineFsStage -eq 3 -and
                $phase -eq 'engine-fs-read-counts' -and
                $version -cmatch '^[0-9]+:[0-9]+:[0-9]+:[0-9]+$') {
                $engineFsCounts=$version
                $engineFsStage=4
            } elseif ($engineFsStage -eq 4 -and
                $phase -eq 'engine-fs-read-result' -and
                (($version -eq 'parsed-and-verified' -and
                  $engineFsCounts -eq '62:8:156:66') -or
                 ($version -eq 'unexpected-counts' -and
                  $engineFsCounts -ne '62:8:156:66') -or
                 ($version -in @('invalid-abi','read-refused',
                                 'missing-definitions','missing-layout') -and
                  $engineFsCounts -eq '0:0:0:0'))) {
                $engineFsRead=$version
                $engineFsStage=5
            } elseif ($phase -eq 'engine-fs-terminal-stop' -and
                $version -eq 'read-only-diagnostic') {
                if ($engineFsStage -eq 2) {
                    $engineFsPresenceLogs.Add(('{0}:client:{1}' -f $log.Name,$engineFsPresence))
                    $engineFsStage=6
                } elseif ($engineFsStage -eq 5) {
                    $engineFsReadLogs.Add(('{0}:client:{1}:{2}:{3}' -f $log.Name,
                        $engineFsPresence,$engineFsRead,$engineFsCounts))
                    $engineFsStage=6
                }
            }
        }
        # This classifier only treats two-stage resources as meaningful
        # AFTER the same session log contains successful actual engine
        # FileExists and combined Read/Parse. The added resource diagnostic
        # calls rb/GAME for definitions and rt/null for layout and owns
        # its 66-entry catalog outside proprietary KeyValues.
        if ($role -eq 'client' -and $engineFsStage -eq 5 -and
            $engineFsPresence -eq 'both-present' -and
            $engineFsRead -eq 'parsed-and-verified' -and
            $engineFsCounts -eq '62:8:156:66') {
            if ($engineFsStagedStage -eq 0 -and
                $phase -eq 'engine-fs-staged-entered' -and
                $version -eq 'rb-GAME:rt-null:owned-only') {
                $engineFsStagedStage=1
            } elseif ($engineFsStagedStage -eq 1 -and
                $phase -eq 'engine-fs-staged-counts' -and
                $version -cmatch '^([0-9]+:){4}[0-9]+$') {
                $engineFsStagedCounts=$version
                $engineFsStagedStage=2
            } elseif ($engineFsStagedStage -eq 2 -and
                $phase -eq 'engine-fs-staged-stage' -and
                $version -cmatch '^[0-7]:[0-7]$') {
                $engineFsStagedSteps=$version
                $engineFsStagedStage=3
            } elseif ($engineFsStagedStage -eq 3 -and
                $phase -eq 'engine-fs-staged-result' -and
                (($version -eq 'owned-staged-verified-not-engine-init' -and
                  $engineFsStagedCounts -eq '66:62:8:156:66' -and
                  $engineFsStagedSteps -eq '7:7') -or
                 ($version -eq 'definitions-stage-refused' -and
                  $engineFsStagedCounts -eq '0:0:0:0:0' -and
                  $engineFsStagedSteps -eq '2:3') -or
                 ($version -eq 'layout-stage-refused' -and
                  $engineFsStagedCounts -eq '0:0:0:0:0' -and
                  $engineFsStagedSteps -eq '3:4') -or
                 ($version -in @('invalid-engine-abi','unexpected-staged-counts')))) {
                $engineFsStagedResult=$version
                $engineFsStagedStage=4
            } elseif ($engineFsStagedStage -eq 4 -and
                $phase -eq 'engine-fs-staged-terminal-stop' -and
                $version -eq 'original-modes-owned-only') {
                $engineFsStagedLogs.Add(('{0}:client:{1}:{2}:{3}' -f
                    $log.Name,$engineFsStagedResult,$engineFsStagedCounts,
                    $engineFsStagedSteps))
                $engineFsStagedStage=5
            }
        }
        # Only SAME-LOG successful original staged stats may unlock a
        # distinct owned user-message hook verdict (not dispatch).
        if ($role -eq 'client' -and $engineFsStagedStage -eq 5 -and
            $engineFsStagedResult -eq 'owned-staged-verified-not-engine-init' -and
            $engineFsStagedCounts -eq '66:62:8:156:66' -and
            $engineFsStagedSteps -eq '7:7') {
            if ($engineFsHooksStage -eq 0 -and
                $phase -eq 'engine-fs-hooks-entered' -and
                $version -eq 'CUserMessages:owned-only') {
                $engineFsHooksStage=1
            } elseif ($engineFsHooksStage -eq 1 -and
                $phase -eq 'engine-fs-hooks-ids' -and
                $version -cmatch '^(-?[0-9]+:){2}-?[0-9]+$') {
                $engineFsHookIds=$version
                $engineFsHooksStage=2
            } elseif ($engineFsHooksStage -eq 2 -and
                $phase -eq 'engine-fs-hooks-count' -and
                $version -cmatch '^[0-3]$') {
                $engineFsHookCount=$version
                $engineFsHooksStage=3
            } elseif ($engineFsHooksStage -eq 3 -and
                $phase -eq 'engine-fs-hooks-result' -and
                (($version -eq 'three-owned-hooks-bound-not-dispatched' -and
                  $engineFsHookIds -eq '27:28:5' -and
                  $engineFsHookCount -eq '3') -or
                 $version -eq 'refused')) {
                $engineFsHookResult=$version
                $engineFsHooksStage=4
            } elseif ($engineFsHooksStage -eq 4 -and
                $phase -eq 'engine-fs-hooks-terminal-stop' -and
                $version -eq 'independent-only') {
                $engineFsHooksLogs.Add(('{0}:client:{1}:{2}:{3}' -f
                    $log.Name,$engineFsHookResult,$engineFsHookIds,
                    $engineFsHookCount))
                $engineFsHooksStage=5
            }
        }
        # Source-only fourth stage. Same-session 27:28:5 hook proof
        # must precede any putative original soundscape catalog result.
        if ($role -eq 'client' -and $engineFsHooksStage -eq 5 -and
            $engineFsHookResult -eq 'three-owned-hooks-bound-not-dispatched' -and
            $engineFsHookIds -eq '27:28:5' -and $engineFsHookCount -eq '3') {
            if ($soundscapeStage -eq 0 -and
                $phase -eq 'engine-fs-soundscape-entered' -and
                $version -eq 'rb-GAME:read-only') {
                $soundscapeStage=1
            } elseif ($soundscapeStage -eq 1 -and
                $phase -eq 'engine-fs-soundscape-counts' -and
                $version -cmatch '^[0-9]+:[0-9]+:[0-9]+:[0-9]+$') {
                $soundscapeCounts=$version
                $soundscapeStage=2
            } elseif ($soundscapeStage -eq 2 -and
                $phase -eq 'engine-fs-soundscape-result' -and
                (($version -eq 'owned-soundscape-catalog-verified-original-files' -and
                  $soundscapeCounts -eq '7:301:242:3') -or
                 $version -cin @('owned-soundscape-read-or-parse-refused',
                                 'owned-soundscape-original-counts-refused'))) {
                $soundscapeResult=$version
                $soundscapeStage=3
            } elseif ($soundscapeStage -eq 3 -and
                $phase -eq 'engine-fs-soundscape-detail' -and
                $version -cmatch '^(resolve-abi|read-manifest|validate-manifest|read-script|script-budget|build-catalog|complete):[a-z0-9-]{1,64}:[a-z0-9-]{1,64}:[a-z0-9-]{1,64}:[0-9]{1,2}:[0-9]{1,2}$') {
                $soundscapeDetailCount++
                if($soundscapeDetailCount -eq 1) {
                    $soundscapeDetail=$version
                }
            } elseif ($soundscapeStage -eq 3 -and
                $phase -clike 'engine-fs-soundscape-cvar-*') {
                # Advisory independent FindVar trace only. Count every
                # related event, even a malformed/duplicated one, so
                # nothing can masquerade as a clean three-event result.
                $soundscapeCvarEvents++
                if($soundscapeCvarStage -eq 0 -and
                   $phase -eq 'engine-fs-soundscape-cvar-entered' -and
                   $version -eq 'VEngineCvar002:slot3:read-only') {
                    $soundscapeCvarStage=1
                } elseif($soundscapeCvarStage -eq 1 -and
                    $phase -eq 'engine-fs-soundscape-cvar-result' -and
                    $version -cmatch '^(original-soundscape-both-vars-cached|original-soundscape-one-var-cached|original-soundscape-vars-unavailable|soundscape-cvar-invalid-abi|soundscape-cvar-callback-refused):([0-2]):([0-2]):([01]):([01])$') {
                    $kind=$Matches[1]
                    $requested=[int]$Matches[2]
                    $found=[int]$Matches[3]
                    $mixer=[int]$Matches[4]
                    $dsp=[int]$Matches[5]
                    $validCounters=$found -le $requested
                    $validKind=switch($kind) {
                        'original-soundscape-both-vars-cached' {$mixer -eq 1 -and $dsp -eq 1}
                        'original-soundscape-one-var-cached' {$mixer -ne $dsp}
                        'original-soundscape-vars-unavailable' {$mixer -eq 0 -and $dsp -eq 0}
                        'soundscape-cvar-invalid-abi' {$requested -eq 0 -and $found -eq 0 -and $mixer -eq 0 -and $dsp -eq 0}
                        'soundscape-cvar-callback-refused' {$mixer -eq 0 -and $dsp -eq 0}
                        default {$false}
                    }
                    if($validCounters -and $validKind) {
                        $soundscapeCvarResult=$version
                        $soundscapeCvarStage=2
                    }
                } elseif($soundscapeCvarStage -eq 2 -and
                    $phase -eq 'engine-fs-soundscape-cvar-terminal-stop' -and
                    $version -eq 'no-convar-writes-no-game-init') {
                    $soundscapeCvarStage=3
                }
            } elseif ($soundscapeStage -eq 3 -and
                $phase -clike 'engine-fs-physics-*') {
                # Physics is a strictly LATER, optional query of the
                # original VClient011 second (physics) Factory. Require
                # a COMPLETE prior CVar event sequence in this SAME
                # client session; no premature or duplicated result can
                # become trusted owner proof.
                $physicsEventCount++
                if($soundscapeCvarStage -eq 3 -and
                   $soundscapeCvarEvents -eq 3 -and
                   $null -ne $soundscapeCvarResult) {
                    if($physicsStage -eq 0 -and
                       $phase -eq 'engine-fs-physics-entered' -and
                       $version -eq 'physics-factory:three-original:read-only') {
                        $physicsStage=1
                    } elseif($physicsStage -eq 1 -and
                        $phase -eq 'engine-fs-physics-result' -and
                        $version -cmatch '^(original-physics-three-available:3:3:1|original-physics-missing-first:1:0:0|original-physics-missing-second:2:1:0|original-physics-missing-third:3:2:0|original-physics-no-factory:0:0:0)$') {
                        $physicsResult=$version
                        $physicsStage=2
                    } elseif($physicsStage -eq 2 -and
                        $phase -eq 'engine-fs-physics-terminal-stop' -and
                        $version -eq 'no-physics-coupling-no-game-init') {
                        $physicsStage=3
                    }
                }
            } elseif ($soundscapeStage -eq 3 -and
                $phase -clike 'engine-fs-surfaceprops-*') {
                $surfacePropsEventCount++
                if($physicsStage -eq 3 -and
                   $physicsEventCount -eq 3 -and
                   $physicsResult -ceq 'original-physics-three-available:3:3:1' -and
                   $soundscapeCvarStage -eq 3 -and
                   $soundscapeCvarEvents -eq 3) {
                    if($surfacePropsStage -eq 0 -and
                       $phase -eq 'engine-fs-surfaceprops-entered' -and
                       $version -eq 'VFileSystem012:surfaceproperties:rb-dual-scope') {
                        $surfacePropsStage=1
                    } elseif($surfacePropsStage -eq 1 -and
                        $phase -eq 'engine-fs-surfaceprops-result' -and
                        $version -cmatch '^(original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0|original-surfaceprops-engine-filesystem-invalid:0:0:0:0:0|original-surfaceprops-retail-resource-refused:([0-2]):([0-2]):([0-9]{1,3}):([0-9]{1,3}):([0-9]{1,3}))$') {
                        $valid=($version -eq 'original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0' -or
                           $version -eq 'original-surfaceprops-engine-filesystem-invalid:0:0:0:0:0')
                        if(-not $valid -and $Matches[2] -ne '') {
                            $attempted=[int]$Matches[2]
                            $read=[int]$Matches[3]
                            $valid=($read -le $attempted)
                        }
                        if($valid) {
                            $surfacePropsResult=$version
                            $surfacePropsStage=2
                        }
                    } elseif($surfacePropsStage -eq 2 -and
                        $phase -eq 'engine-fs-surfaceprops-terminal-stop' -and
                        $version -eq 'no-physics-surface-registration-no-game-init') {
                        $surfacePropsStage=3
                    }
                }
            } elseif ($soundscapeStage -eq 3 -and
                $phase -clike 'engine-fs-material-*') {
                # Phase522: material field reads are strictly opt-in, may
                # occur BEFORE registration, and must never create phantom
                # owner proof from duplicate or cross-session float lines.
                $materialEventCount++
                if($surfacePropsStage -eq 3 -and
                   $surfacePropsEventCount -eq 3 -and
                   $surfacePropsResult -ceq 'original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0' -and
                   $physicsStage -eq 3 -and $physicsEventCount -eq 3 -and
                   $physicsResult -ceq 'original-physics-three-available:3:3:1') {
                    if($materialStage -eq 0 -and
                       $phase -eq 'engine-fs-material-entered' -and
                       $version -eq 'VPhysicsSurfaceProps001:slots2-3-5:read-only') {
                        $materialStage=1
                    } elseif($materialStage -eq 1 -and
                        $phase -eq 'engine-fs-material-result' -and
                        $version -cmatch '^original-physics-material-(invalid-object|index-refused|no-data-slot|null-data|unreadable-data|nonfinite-data|three-samples):(-?[0-9]{1,6}):([0-3])$') {
                        $kind=$Matches[1]
                        $n=[int]$Matches[2]
                        $sample=[int]$Matches[3]
                        $success=($kind -ceq 'three-samples' -and
                            $n -ge 3 -and $n -le 16384 -and $sample -eq 3)
                        $failed=($kind -cne 'three-samples' -and
                            $n -ge -1 -and $n -le 16384 -and $sample -le 2)
                        if($success -or $failed) {
                            $materialCount=$n
                            $materialResult=$version
                            $materialStage=2
                        }
                    } elseif($materialStage -eq 2 -and
                        $phase -eq 'engine-fs-material-sample' -and
                        $materialResult -cmatch '^original-physics-material-three-samples:' -and
                        $materialSampleCount -lt 3) {
                        $target=@('default','solidmetal','metalvehicle')[$materialSampleCount]
                        $pattern='^'+[regex]::Escape($target)+':([0-9]{1,5}):([-+0-9.eE]+):([-+0-9.eE]+):([-+0-9.eE]+):([-+0-9.eE]+):([-+0-9.eE]+)$'
                        $m=[regex]::Match($version,$pattern)
                        if($m.Success -and [int]$m.Groups[1].Value -lt $materialCount) {
                            $numericOk=$true
                            for($j=2;$j -le 6;$j++){
                                $v=0.0
                                if(-not [double]::TryParse(
                                    $m.Groups[$j].Value,
                                    [System.Globalization.NumberStyles]::Float,
                                    [System.Globalization.CultureInfo]::InvariantCulture,
                                    [ref]$v) -or
                                   [double]::IsInfinity($v) -or
                                   [double]::IsNaN($v) -or
                                   [math]::Abs($v) -gt 1000000000.0) {
                                    $numericOk=$false
                                }
                            }
                            if($numericOk){$materialSampleCount++}
                        }
                    } elseif($materialStage -eq 2 -and
                        $phase -eq 'engine-fs-material-terminal-stop' -and
                        $version -eq 'read-only-physics-fields-no-game-init') {
                        $success=($materialResult -cmatch '^original-physics-material-three-samples:' -and
                            $materialSampleCount -eq 3 -and $materialEventCount -eq 6)
                        $failed=($materialResult -cnotmatch '^original-physics-material-three-samples:' -and
                            $materialSampleCount -eq 0 -and $materialEventCount -eq 3)
                        if($success -or $failed){$materialStage=3}
                    }
                }
            } elseif ($soundscapeStage -eq 3 -and
                $phase -clike 'engine-fs-registration-*') {
                # Only same-session ordered entry/result/terminal records
                # after VERIFIED source reads attest parser invocations.
                $registrationEventCount++
                if($surfacePropsStage -eq 3 -and
                   $surfacePropsEventCount -eq 3 -and
                   $surfacePropsResult -ceq 'original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0' -and
                   $physicsStage -eq 3 -and
                   $physicsResult -ceq 'original-physics-three-available:3:3:1') {
                    if($registrationStage -eq 0 -and
                       $phase -eq 'engine-fs-registration-entered' -and
                       $version -eq 'VPhysicsSurfaceProps001:slot1:explicit-opt-in') {
                        $registrationStage=1
                    } elseif($registrationStage -eq 1 -and
                       $phase -eq 'engine-fs-registration-result' -and
                       $version -cmatch '^(original-surfaceprops-register-(?:no-interface|invalid-filesystem|resources-refused|raw-shape-refused|method-refused|two-calls-returned)):([0-2]):([0-2]):(-?[0-9]{1,11}):(-?[0-9]{1,11})$') {
                        $kind=$Matches[1]
                        $sourceCount=[int]$Matches[2]
                        $callCount=[int]$Matches[3]
                        $valid=($kind -ceq 'original-surfaceprops-register-two-calls-returned' -and
                                $sourceCount -eq 2 -and $callCount -eq 2) -or
                               ($kind -cne 'original-surfaceprops-register-two-calls-returned' -and
                                $callCount -eq 0)
                        if($valid) {
                            $registrationResult=$version
                            $registrationStage=2
                        }
                    } elseif($registrationStage -eq 2 -and
                       $phase -eq 'engine-fs-registration-terminal-stop' -and
                       $version -eq 'two-engine-call-returns-not-game-init') {
                        $registrationStage=3
                    }
                }
            } elseif ($soundscapeStage -eq 3 -and
                $phase -clike 'engine-fs-readback-*') {
                # SDK2004-guided slots2/3 are separate opt-in calls after
                # exactly two ParseSurfaceData methods returned this session.
                $readbackEventCount++
                if($registrationStage -eq 3 -and
                   $registrationEventCount -eq 3 -and
                   $registrationResult -cmatch '^original-surfaceprops-register-two-calls-returned:2:2:-?[0-9]{1,11}:-?[0-9]{1,11}$' -and
                   $surfacePropsStage -eq 3 -and
                   $surfacePropsResult -ceq 'original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0') {
                    if($readbackStage -eq 0 -and
                       $phase -eq 'engine-fs-readback-entered' -and
                       $version -eq 'VPhysicsSurfaceProps001:slots2-3:read-only') {
                        $readbackStage=1
                    } elseif($readbackStage -eq 1 -and
                       $phase -eq 'engine-fs-readback-result') {
                        $match=[regex]::Match($version,
                            '^(original-physics-surface-readback-(?:invalid-object|missing-count-slot|missing-index-slot|count-invalid|index-invalid|some-unavailable|three-in-range)):(-?[0-9]{1,6}):(-?[0-9]{1,6}):(-?[0-9]{1,6}):(-?[0-9]{1,6})$')
                        if($match.Success) {
                            $kind=$match.Groups[1].Value
                            $count=[int]$match.Groups[2].Value
                            $indices=@([int]$match.Groups[3].Value,
                                       [int]$match.Groups[4].Value,
                                       [int]$match.Groups[5].Value)
                            $allInRange=($count -ge 1 -and $count -le 16384)
                            $anyAbsent=$false
                            $anyInvalid=$false
                            foreach($id in $indices) {
                                if($id -eq -1) {$anyAbsent=$true}
                                elseif($id -lt -1 -or $id -ge $count) {
                                    $anyInvalid=$true
                                }
                            }
                            $valid=$false
                            if($kind -ceq 'original-physics-surface-readback-three-in-range') {
                                $valid=($allInRange -and -not $anyAbsent -and -not $anyInvalid)
                            } elseif($kind -ceq 'original-physics-surface-readback-some-unavailable') {
                                $valid=($count -ge 0 -and $count -le 16384 -and $anyAbsent -and -not $anyInvalid)
                            } elseif($kind -ceq 'original-physics-surface-readback-index-invalid') {
                                $valid=($count -ge 0 -and $count -le 16384 -and $anyInvalid)
                            } elseif($kind -ceq 'original-physics-surface-readback-count-invalid') {
                                $valid=(($count -lt 0 -or $count -gt 16384) -and
                                    ($indices -join ',') -ceq '-1,-1,-1')
                            } else {
                                $valid=($count -eq -1 -and ($indices -join ',') -ceq '-1,-1,-1')
                            }
                            if($valid){
                                $readbackResult=$version
                                $readbackStage=2
                            }
                        }
                    } elseif($readbackStage -eq 2 -and
                       $phase -eq 'engine-fs-readback-terminal-stop' -and
                       $version -eq 'SDK2004-count-and-indices-not-game-init') {
                        $readbackStage=3
                    }
                }
            } elseif ($soundscapeStage -eq 3 -and
                $phase -eq 'engine-fs-soundscape-terminal-stop' -and
                $version -eq 'owner-original-files-no-game-init') {
                $engineFsSoundscapeLogs.Add(('{0}:client:{1}:{2}' -f
                    $log.Name,$soundscapeResult,$soundscapeCounts))
                # Detail is optional for compatibility with old saved
                # owner traces. Accept it ONLY after this same-session
                # terminal sequence and exactly one bounded field.
                if($soundscapeDetailCount -eq 1 -and $null -ne $soundscapeDetail) {
                    $engineFsSoundscapeDetailLogs.Add(('{0}:client:{1}' -f
                        $log.Name,$soundscapeDetail))
                }
                if($soundscapeDetailCount -eq 1 -and
                   $soundscapeCvarEvents -eq 3 -and
                   $soundscapeCvarStage -eq 3 -and
                   $null -ne $soundscapeCvarResult) {
                    $engineFsSoundscapeCvarLogs.Add(('{0}:client:{1}' -f
                        $log.Name,$soundscapeCvarResult))
                }
                if($soundscapeDetailCount -eq 1 -and
                   $soundscapeCvarEvents -eq 3 -and
                   $soundscapeCvarStage -eq 3 -and
                   $physicsEventCount -eq 3 -and
                   $physicsStage -eq 3 -and
                   $null -ne $physicsResult) {
                    $engineFsPhysicsFactoryLogs.Add(('{0}:client:{1}' -f
                        $log.Name,$physicsResult))
                }
                if($soundscapeDetailCount -eq 1 -and
                   $physicsEventCount -eq 3 -and
                   $physicsStage -eq 3 -and
                   $surfacePropsEventCount -eq 3 -and
                   $surfacePropsStage -eq 3 -and
                   $null -ne $surfacePropsResult) {
                    $engineFsSurfacePropsLogs.Add(('{0}:client:{1}' -f
                        $log.Name,$surfacePropsResult))
                }
                if($soundscapeDetailCount -eq 1 -and
                   $surfacePropsStage -eq 3 -and
                   $surfacePropsEventCount -eq 3 -and
                   $registrationEventCount -eq 3 -and
                   $registrationStage -eq 3 -and
                   $null -ne $registrationResult) {
                    $engineFsRegistrationLogs.Add(('{0}:client:{1}' -f
                        $log.Name,$registrationResult))
                }
                if($soundscapeDetailCount -eq 1 -and
                   $materialStage -eq 3 -and
                   $null -ne $materialResult) {
                    $engineFsMaterialLogs.Add(('{0}:client:{1}' -f
                        $log.Name,$materialResult))
                }
                if($soundscapeDetailCount -eq 1 -and
                   $registrationStage -eq 3 -and
                   $registrationEventCount -eq 3 -and
                   $readbackStage -eq 3 -and
                   $readbackEventCount -eq 3 -and
                   $null -ne $readbackResult) {
                    $engineFsReadbackLogs.Add(('{0}:client:{1}' -f
                        $log.Name,$readbackResult))
                }
                $soundscapeStage=4
            }
        }
        if ($role -eq 'server' -and $phase -eq 'init-entered') {
            $serverInitEnteredInLog=$true
        }
        if ($role -eq 'server' -and $phase -clike 'server-physics-*') {
            # The actual 2006 server physics triple is a separate, later
            # opt-in stage; accept only a single ordered 3-event sequence.
            $serverPhysicsEvents++
            if ($serverInitEnteredInLog -and $serverPhysicsStage -eq 0 -and
                $phase -eq 'server-physics-entered' -and
                $version -eq 'original-server-second-factory:read-only') {
                $serverPhysicsStage=1
            } elseif ($serverPhysicsStage -eq 1 -and
                $phase -eq 'server-physics-result' -and
                $version -cmatch '^(original-server-physics-three-available:3:3:1|original-server-physics-missing-first:1:0:0|original-server-physics-missing-second:2:1:0|original-server-physics-missing-third:3:2:0|original-server-physics-missing-factory:0:0:0)$') {
                $serverPhysicsResult=$version
                $serverPhysicsStage=2
            } elseif ($serverPhysicsStage -eq 2 -and
                $phase -eq 'server-physics-terminal-stop' -and
                $version -eq 'no-manifest-coupling-no-server-init') {
                $serverPhysicsStage=3
            }
        }
        if ($role -eq 'server' -and $phase -clike 'server-surfaceprops-*') {
            # A separate ordered three-event read-only check, only after
            # a COMPLETE original-server physics triple in this very log.
            $serverSurfacePropsEvents++
            if ($serverInitEnteredInLog -and
                $serverPhysicsStage -eq 3 -and $serverPhysicsEvents -eq 3 -and
                $serverPhysicsResult -ceq 'original-server-physics-three-available:3:3:1') {
                if ($serverSurfacePropsStage -eq 0 -and
                    $phase -eq 'server-surfaceprops-entered' -and
                    $version -eq 'VFileSystem012:server-original:rb-dual-scope') {
                    $serverSurfacePropsStage=1
                } elseif ($serverSurfacePropsStage -eq 1 -and
                    $phase -eq 'server-surfaceprops-result' -and
                    $version -cmatch '^(original-server-surfaceprops-(two-sources-verified|invalid-filesystem|resource-refused)):(\d{1,3}):(\d{1,3}):(\d{1,3}):(\d{1,3}):(\d{1,3})$') {
                    $type=$Matches[2]
                    $attempted=[int]$Matches[3];$read=[int]$Matches[4]
                    $definitions=[int]$Matches[5];$bases=[int]$Matches[6]
                    $repeated=[int]$Matches[7]
                    $valid=$false
                    if ($type -ceq 'two-sources-verified') {
                        $valid=$attempted -eq 2 -and $read -eq 2 -and
                            $definitions -eq 93 -and $bases -eq 71 -and
                            $repeated -eq 0
                    } elseif ($type -ceq 'invalid-filesystem') {
                        $valid=$attempted -eq 0 -and $read -eq 0 -and
                            $definitions -eq 0 -and $bases -eq 0 -and
                            $repeated -eq 0
                    } else {
                        $valid=$attempted -le 2 -and $read -le $attempted -and
                            $definitions -le 93 -and $bases -le 71 -and
                            $repeated -le 93
                    }
                    if ($valid) {
                        $serverSurfacePropsResult=$version
                        $serverSurfacePropsStage=2
                    }
                } elseif ($serverSurfacePropsStage -eq 2 -and
                    $phase -eq 'server-surfaceprops-terminal-stop' -and
                    $version -eq 'read-only-manifest-no-server-init') {
                    $serverSurfacePropsStage=3
                }
            }
        }
        if ($role -eq 'server' -and $version -eq 'VSoundEmitter001') {
            if ($phase -eq 'sound-connect-entered') { $soundEntered = $true; $soundResult = $null }
            if ($soundEntered -and ($phase -eq 'sound-connect-succeeded' -or $phase -eq 'sound-connect-rejected')) {
                $soundResult = $phase
                $soundCallbacks.Add(("{0}:{1}:{2}" -f $log.Name,$role,$phase))
            }
            if ($soundEntered -and $null -ne $soundResult -and $phase -eq 'sound-probe-terminal-stop') {
                $soundTerminalLogs.Add(("{0}:{1}" -f $log.Name,$soundResult))
            }
        }
        if ($role -eq 'server' -and $soundResult -eq 'sound-connect-succeeded') {
            if ($phase -eq 'math-tables-initialized' -and $version -eq 'server-scalar') {
                $eventStage = 1
                $eventResult = $null
            } elseif ($eventStage -eq 1 -and $phase -eq 'factory-state-cached' -and $version -eq 'server') {
                $eventStage = 2
            } elseif ($eventStage -eq 2 -and $phase -eq 'event-load-entered' -and $version -eq 'resource/gameevents.res') {
                $eventStage = 3
            } elseif ($eventStage -eq 3 -and $phase -eq 'event-load-returned' -and
                $version -cmatch '^resource/gameevents\.res:(-?[0-9]+)$') {
                $count = 0
                if ([int]::TryParse($Matches[1],[ref]$count)) {
                    $eventResult = $count
                    $eventStage = 4
                }
            } elseif ($eventStage -eq 4 -and $phase -eq 'event-probe-terminal-stop' -and $version -eq 'resource/gameevents.res') {
                $eventTerminalLogs.Add(("{0}:{1}" -f $log.Name,$eventResult))
                $eventStage = 5
            }
        }
        if ($role -eq 'server' -and $eventStage -eq 4 -and $eventResult -gt 0) {
            if ($cvarStage -eq 0 -and $phase -eq 'cvar-name-available' -and $version -eq 'dd_log_events') {
                $cvarStage = 1
            } elseif ($cvarStage -eq 1 -and $phase -eq 'cvar-register-entered' -and $version -eq 'dd_log_events') {
                $cvarStage = 2
            } elseif ($cvarStage -eq 2 -and $phase -eq 'cvar-register-returned' -and $version -eq 'dd_log_events') {
                $cvarStage = 3
            } elseif ($cvarStage -eq 3 -and $phase -eq 'cvar-registration-verified' -and $version -eq 'dd_log_events') {
                $cvarStage = 4
            } elseif ($phase -eq 'cvar-probe-result') {
                if (($version -eq 'verified' -and $cvarStage -eq 4) -or
                    ($version -cin @('invalid-interface','existing-name','invalid-list') -and $cvarStage -eq 0) -or
                    ($version -eq 'bad-owned-object' -and $cvarStage -in @(1,3)) -or
                    ($version -cin @('lookup-mismatch','unexpected-relink','override-present') -and $cvarStage -eq 3)) {
                    $cvarResult = $version
                    $cvarStage = 5
                }
            } elseif ($cvarStage -eq 5 -and $phase -eq 'cvar-probe-terminal-stop' -and $version -eq 'dd_log_events') {
                $cvarTerminalLogs.Add(("{0}:{1}" -f $log.Name,$cvarResult))
                $cvarStage = 6
            }
        }
        if ($role -eq 'server' -and $eventStage -eq 4 -and $eventResult -gt 0) {
            if ($phase -eq 'cvar-batch-preflight-complete' -and $version -eq '24' -and $batchStage -eq 0) {
                $batchStage = 1
            } elseif ($batchVerified -lt $batchNames.Count -and $version -ceq $batchNames[$batchVerified]) {
                if ($phase -eq 'cvar-batch-register-entered' -and $batchStage -eq 1) { $batchStage = 2 }
                elseif ($phase -eq 'cvar-batch-register-returned' -and $batchStage -eq 2) { $batchStage = 3 }
                elseif ($phase -eq 'cvar-batch-registration-verified' -and $batchStage -eq 3) {
                    $batchVerified++
                    $batchStage = 1
                }
            } elseif ($phase -eq 'cvar-batch-result' -and $version -cmatch '^(verified|invalid-interface|existing-name|invalid-list|bad-owned-object|lookup-mismatch|unexpected-relink|override-present):([0-9]+)/24$') {
                $status = $Matches[1]
                $count = [int]$Matches[2]
                if ($count -eq $batchVerified -and ($status -ne 'verified' -or
                    ($batchStage -eq 1 -and $batchVerified -eq $batchNames.Count))) {
                    $batchResult = $version
                    $batchStage = 4
                }
            } elseif ($phase -eq 'cvar-batch-terminal-stop' -and $version -eq '24' -and $batchStage -eq 4) {
                $batchTerminalLogs.Add(("{0}:{1}" -f $log.Name,$batchResult))
                $batchStage = 5
            }
        }
        if ($phase -eq 'init-entered') { $explicitInitRoles[$role] = $true }
        # Backward-compatible classification of old traces. Only an actual
        # Init/DLLInit body can emit these, unlike Shutdown or factory probes.
        if ($phase -eq 'request' -or $phase -eq 'missing-interface' -or
            $phase -eq 'missing-factory' -or $phase -eq 'invalid-init-arguments' -or
            $phase -eq 'sdk2004-preflight-only') { $legacyInitEvents++ }
        if ($phase -eq 'request') { $requests.Add(("{0}:{1}" -f $role,$version)) }
        if ($phase -eq 'missing-interface' -or
            $phase -eq 'missing-factory' -or
            $phase -eq 'invalid-init-arguments') {
            $blocker = ("{0}:{1}:{2}" -f $role,$phase,$version)
            $issues.Add($blocker)
            if ($null -eq $logFirstBlocker) { $logFirstBlocker = $blocker }
        }
    }
    if ($serverPhysicsStage -eq 3 -and $serverPhysicsEvents -eq 3 -and
        $null -ne $serverPhysicsResult) {
        $serverPhysicsLogs.Add(('{0}:server:{1}' -f
            $log.Name,$serverPhysicsResult))
    }
    if ($serverSurfacePropsStage -eq 3 -and
        $serverSurfacePropsEvents -eq 3 -and
        $serverPhysicsStage -eq 3 -and $serverPhysicsEvents -eq 3 -and
        $null -ne $serverSurfacePropsResult) {
        $serverSurfacePropsLogs.Add(('{0}:server:{1}' -f
            $log.Name,$serverSurfacePropsResult))
    }
    if ($null -ne $logFirstBlocker) {
        $blockersByLog.Add(("{0}={1}" -f $log.Name,$logFirstBlocker))
    }
}
Write-Output ("OWNER_INDEPENDENT_TRACE_FILES: {0}" -f $logs.Count)
Write-Output ("OWNER_INDEPENDENT_ROLES_OBSERVED: {0}" -f ((@('client','server') | Where-Object { $seen.ContainsKey($_) }) -join ','))
Write-Output ("OWNER_INDEPENDENT_EXPLICIT_INIT_ROLES: {0}" -f ((@('client','server') | Where-Object { $explicitInitRoles.ContainsKey($_) }) -join ','))
Write-Output ("OWNER_INDEPENDENT_FACTORY_REQUESTS: {0}" -f ($requests -join ','))
Write-Output ("OWNER_INDEPENDENT_MODULE_FACTORY_RESULTS: {0}" -f ($factoryResults -join ','))
if ($predictionFactoryEvents.Count -gt 0) {
    # A bound source ConVar or factory acceptance is only a
    # native interface negotiation observation, NOT engine startup.
    Write-Output ("OWNER_CLIENT_PREDICTION_FACTORY_BOUNDARY: {0}" -f
        ($predictionFactoryEvents -join ','))
}
if ($predictionConVarEvents.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_PREDICTION_CONVAR_REGISTRATION: {0}" -f
        ($predictionConVarEvents -join ','))
}
if ($entityListFactoryEvents.Count -gt 0) {
    # The source-correct empty entity-list interface is only a
    # diagnostic factory boundary. Parent entity registration is
    # fatal, so this is never proof of live gameplay entities.
    Write-Output ("OWNER_CLIENT_ENTITYLIST_EMPTY_FACTORY_BOUNDARY: {0}" -f
        ($entityListFactoryEvents -join ','))
}
if ($unsupportedCallbacks.Count -gt 0) {
    Write-Output ("OWNER_INDEPENDENT_UNSUPPORTED_CALLBACKS: {0}" -f ($unsupportedCallbacks -join ','))
}
if ($soundCallbacks.Count -gt 0) {
    Write-Output ("OWNER_SERVER_SOUND_CONNECT_RESULTS: {0}" -f ($soundCallbacks -join ','))
}
if ($serverPhysicsLogs.Count -gt 0) {
    Write-Output ("OWNER_SERVER_ORIGINAL_PHYSICS_FACTORY_RESULTS: {0}" -f ($serverPhysicsLogs -join ','))
}
if ($serverSurfacePropsLogs.Count -gt 0) {
    Write-Output ("OWNER_SERVER_ORIGINAL_SURFACEPROPS_READ_RESULTS: {0}" -f
        ($serverSurfacePropsLogs -join ','))
}
if($env:SIN_INDEPENDENT_VERDICT_DEBUG -eq '1') {
    Write-Output ("OWNER_CLIENT_SOUND_SYSTEM_DEBUG: {0}" -f ($soundInitSeenEvents -join ' | '))
}
if ($soundSystemInitLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SOUND_SYSTEM_INIT_RESULTS: {0}" -f ($soundSystemInitLogs -join ','))
}
if ($statsAssetProbeLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_STATS_ASSET_PROBE_RESULTS: {0}" -f ($statsAssetProbeLogs -join ','))
}
if ($engineFsPresenceLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_ENGINE_FS_PRESENCE_RESULTS: {0}" -f ($engineFsPresenceLogs -join ','))
}
if ($engineFsReadLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_ENGINE_FS_READ_RESULTS: {0}" -f ($engineFsReadLogs -join ','))
}
if ($engineFsStagedLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_ENGINE_FS_STAGED_RESULTS: {0}" -f ($engineFsStagedLogs -join ','))
}
if ($engineFsHooksLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_ENGINE_FS_HOOK_RESULTS: {0}" -f ($engineFsHooksLogs -join ','))
}
if ($engineFsSoundscapeLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SOUNDSCAPE_CATALOG_RESULTS: {0}" -f ($engineFsSoundscapeLogs -join ','))
}
if ($engineFsSoundscapeDetailLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SOUNDSCAPE_DETAILS: {0}" -f ($engineFsSoundscapeDetailLogs -join ','))
}
if ($engineFsSoundscapeCvarLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SOUNDSCAPE_CVAR_LOOKUP_RESULTS: {0}" -f ($engineFsSoundscapeCvarLogs -join ','))
}
if ($engineFsPhysicsFactoryLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_PHYSICS_FACTORY_RESULTS: {0}" -f ($engineFsPhysicsFactoryLogs -join ','))
}
if ($engineFsSurfacePropsLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SURFACEPROPS_RESULTS: {0}" -f ($engineFsSurfacePropsLogs -join ','))
}
if ($engineFsRegistrationLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SURFACEPROPS_REGISTRATION_RESULTS: {0}" -f ($engineFsRegistrationLogs -join ','))
}
if ($engineFsMaterialLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SURFACEPHYSICS_MATERIAL_RESULTS: {0}" -f
        ($engineFsMaterialLogs -join ','))
}
if ($engineFsReadbackLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SURFACEPROPS_READBACK_RESULTS: {0}" -f ($engineFsReadbackLogs -join ','))
}
if ($matSurfaceLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_MAT_SURFACE_RESULTS: {0}" -f ($matSurfaceLogs -join ','))
}
if ($vguiControlsLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_VGUI_CONTROLS_BOOTSTRAP_RESULTS: {0}" -f ($vguiControlsLogs -join ','))
}
if ($engineVGuiSlot1Logs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_ENGINE_VGUI_SLOT1_RESULTS: {0}" -f ($engineVGuiSlot1Logs -join ','))
}
if ($vguiLayoutLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_VGUI_VTABLE_RESULTS: {0}" -f ($vguiLayoutLogs -join ','))
}
if ($vguiLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_VGUI_RESULTS: {0}" -f ($vguiLogs -join ','))
}
if ($ownedStartupLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_OWNED_STARTUP_RESULTS: {0}" -f ($ownedStartupLogs -join ','))
}
if ($clientCvarTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_CVAR_PROBE_RESULTS: {0}" -f ($clientCvarTerminalLogs -join ','))
}
if ($gaussianTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_GAUSSIAN_PROBE_RESULTS: {0}" -f ($gaussianTerminalLogs -join ','))
}
if ($clientScreenTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SCREEN_PROBE_RESULTS: {0}" -f ($clientScreenTerminalLogs -join ','))
}
if ($clientMaterialTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_MATERIAL_PROBE_RESULTS: {0}" -f ($clientMaterialTerminalLogs -join ','))
}
if ($clientSoundTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_CLIENT_SOUND_CONNECT_RESULTS: {0}" -f ($clientSoundTerminalLogs -join ','))
}
if ($eventTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_SERVER_EVENT_LOAD_RESULTS: {0}" -f ($eventTerminalLogs -join ','))
}
if ($cvarTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_SERVER_CVAR_PROBE_RESULTS: {0}" -f ($cvarTerminalLogs -join ','))
}
if ($batchTerminalLogs.Count -gt 0) {
    Write-Output ("OWNER_SERVER_CVAR_BATCH_RESULTS: {0}" -f ($batchTerminalLogs -join ','))
}
if ($issues.Count -gt 0) {
    Write-Output ("OWNER_INDEPENDENT_BLOCKERS_BY_LOG: {0}" -f ($blockersByLog -join ','))
    if ($blockersByLog.Count -eq 1) {
        Write-Output ("OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: {0}" -f $issues[0])
    } else {
        Write-Output "OWNER_INDEPENDENT_FIRST_RECORDED_BLOCKER: AMBIGUOUS_ACROSS_PROCESSES"
    }
    Write-Output "OWNER_INDEPENDENT_VERDICT: ENGINE_ENTERED_INDEPENDENT_INIT_BUT_REJECTED"
} elseif ($unsupportedCallbacks.Count -gt 0) {
    # A terminal callback can occur before the first Init call. It proves
    # virtual entry and diagnostic termination, not that Init was entered.
    Write-Output "OWNER_INDEPENDENT_VERDICT: UNSUPPORTED_CALLBACK_DIAGNOSTIC_STOP_NO_BOOT_PROOF"
} elseif ($batchTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CVAR_BATCH_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($cvarTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CVAR_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($eventTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: EVENT_LOAD_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($serverPhysicsLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: SERVER_PHYSICS_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($engineFsSoundscapeLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_SOUNDSCAPE_CATALOG_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($engineFsHooksLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_HOOKS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($engineFsStagedLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_STAGED_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($engineFsReadLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_READ_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($engineFsPresenceLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_FS_PRESENCE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($statsAssetProbeLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_STATS_ASSET_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($soundSystemInitLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_SOUND_SYSTEM_INIT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($matSurfaceLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_MAT_SURFACE_QUERY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($vguiControlsLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_VGUI_CONTROLS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($engineVGuiSlot1Logs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_ENGINE_VGUI_SLOT1_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($vguiLayoutLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_VGUI_VTABLE_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($vguiLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_VGUI_FACTORY_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($ownedStartupLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_OWNED_STARTUP_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($clientCvarTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_CVAR_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($gaussianTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_GAUSSIAN_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($clientScreenTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_SCREEN_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($clientMaterialTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_MATERIAL_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($clientSoundTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: CLIENT_SOUND_CONNECT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($soundTerminalLogs.Count -gt 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: SOUND_CONNECT_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF"
} elseif ($explicitInitRoles.Count -eq 0 -and $legacyInitEvents -eq 0 -and $factoryResults.Count -gt 0) {
    # The engine reached CreateInterface, but may have probed optional
    # names. No claim that a rejection is the fatal retail startup cause.
    Write-Output "OWNER_INDEPENDENT_VERDICT: GAME_MODULE_FACTORY_ONLY_NO_INIT_EVIDENCE"
} elseif ($explicitInitRoles.Count -eq 0 -and $legacyInitEvents -eq 0) {
    Write-Output "OWNER_INDEPENDENT_VERDICT: NO_INDEPENDENT_INIT_EVIDENCE"
} else {
    Write-Output "OWNER_INDEPENDENT_VERDICT: DIAGNOSTIC_ONLY_NO_RETAIL_BOOT_PROOF"
}
# Factory-only observations cannot establish retail Init or boot success.
# PID-sorted files cannot establish cross-process event chronology.
# Intentionally no SUCCESS verdict: Phase1686 may let the owner-only
# VClient011 diagnostic return 1 to enter the next original engine
# factory request, but unsupported prediction/gameplay callbacks still
# terminate and no playable retail engine contract is established.
