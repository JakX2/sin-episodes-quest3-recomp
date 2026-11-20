<#
One-click original-game filesystem diagnostic.
NEVER edits the selected installed game. It creates a separate disposable
copy, uses packaged independent DLLs and the existing SHA-gated reversible
launcher, then emits text reports as a ZIP. Normal game boot is not claimed.
#>
[CmdletBinding()]
param(
    [Parameter(Position=0)][string]$GameRoot,
    [switch]$PlanOnly,
    [switch]$StagedStats,
    [switch]$HookProof,
    [ValidateRange(10,300)][int]$TimeoutSeconds=90
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$clientHash='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
$serverHash='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
$gate=Join-Path $PSScriptRoot 'Invoke-OwnerFileSystemRetailGate.ps1'
if ($HookProof -and -not $StagedStats) {
    throw 'HookProof requires StagedStats; do not skip original staged resource prerequisites'
}
$bin=Join-Path $PSScriptRoot 'bin'

function Digest([string]$Path) {
    $hasher=[Security.Cryptography.SHA256]::Create()
    $stream=[IO.File]::OpenRead($Path)
    try {
        return [BitConverter]::ToString($hasher.ComputeHash($stream)).Replace('-','').ToLowerInvariant()
    } finally {
        $stream.Dispose()
        $hasher.Dispose()
    }
}
function Require-Plain([string]$Path,[bool]$Directory) {
    if ($Directory) {
        if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
            throw ("Missing directory: "+$Path)
        }
    } elseif (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw ("Missing file: "+$Path)
    }
    $item=Get-Item -LiteralPath $Path -Force
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw ("Refusing linked/reparse path: "+$Path)
    }
}
function Require-UnlinkedAncestors([string]$Path) {
    $here=[IO.Path]::GetFullPath($Path)
    while ($true) {
        Require-Plain $here $true
        $parent=[IO.Directory]::GetParent($here)
        if ($null -eq $parent) {break}
        $here=$parent.FullName
    }
}
function Select-Game {
    Add-Type -AssemblyName System.Windows.Forms
    $dialog=New-Object System.Windows.Forms.FolderBrowserDialog
    $dialog.Description='Choose the ORIGINAL SiN Episodes folder containing SinEpisodes.exe. This folder is READ ONLY.'
    $dialog.ShowNewFolderButton=$false
    try {
        if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
            return $dialog.SelectedPath
        }
    } finally {$dialog.Dispose()}
    throw 'Folder selection cancelled; no game files changed'
}
function Authenticate-Source([string]$Path) {
    $root=[IO.Path]::GetFullPath($Path).TrimEnd('\')
    if ($root -eq [IO.Path]::GetPathRoot($root).TrimEnd('\')) {
        throw 'Choose the game folder, not an entire drive'
    }
    Require-UnlinkedAncestors $root
    $exe=Join-Path $root 'SinEpisodes.exe'
    $gameInfo=Join-Path $root 'SE1\GameInfo.txt'
    $client=Join-Path $root 'SE1\bin\client.dll'
    $server=Join-Path $root 'SE1\bin\server.dll'
    foreach($file in @($exe,$gameInfo,$client,$server)) {
        Require-Plain $file $false
    }
    if ((Digest $client) -cne $clientHash -or
        (Digest $server) -cne $serverHash) {
        throw 'Original client.dll/server.dll SHA256 mismatch. The test supports only authenticated 2006 retail files.'
    }
    if (Test-Path -LiteralPath (Join-Path $root 'SE1\bin\sin_independent_saved_client.dll')) {
        throw 'Original game contains unresolved independent backup; test refused'
    }
    $pkg=[IO.Path]::GetFullPath($PSScriptRoot).TrimEnd('\')
    if ($pkg.StartsWith(($root+[IO.Path]::DirectorySeparatorChar),
                        [StringComparison]::OrdinalIgnoreCase) -or
        [string]::Equals($pkg,$root,[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Extract the standalone ZIP outside the game directory'
    }
    return $root
}
function Scan-Source([string]$Root) {
    $sum=[long]0
    $stack=New-Object 'System.Collections.Generic.Stack[string]'
    $stack.Push($Root)
    while($stack.Count -gt 0) {
        $folder=$stack.Pop()
        foreach($entry in [IO.Directory]::EnumerateFileSystemEntries($folder)) {
            $attributes=[IO.File]::GetAttributes($entry)
            if (($attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw ('Refusing symlink or junction source content: '+$entry)
            }
            if (($attributes -band [IO.FileAttributes]::Directory) -ne 0) {
                $stack.Push($entry)
            } else {
                $sum+=([IO.FileInfo]::new($entry)).Length
            }
        }
    }
    return $sum
}
function Verify-OriginalPair([string]$Root) {
    $client=Join-Path $Root 'SE1\bin\client.dll'
    $server=Join-Path $Root 'SE1\bin\server.dll'
    if (-not (Test-Path -LiteralPath $client -PathType Leaf) -or
        -not (Test-Path -LiteralPath $server -PathType Leaf)) {
        return $false
    }
    return (Digest $client) -ceq $clientHash -and
           (Digest $server) -ceq $serverHash
}
function Preserve-SessionLogs([string]$Transcript,[string]$Folder) {
    if (-not (Test-Path -LiteralPath $Transcript -PathType Leaf)) {return}
    $sessions=@([IO.File]::ReadAllLines($Transcript) | Where-Object {
        $_ -cmatch '^OWNER_INDEPENDENT_SESSION: [0-9a-f]{32}$'
    })
    if ($sessions.Count -ne 1) {return}
    $m=[regex]::Match($sessions[0],'^OWNER_INDEPENDENT_SESSION: ([0-9a-f]{32})$')
    if (-not $m.Success) {return}
    $id=$m.Groups[1].Value
    foreach($log in @(Get-ChildItem -LiteralPath ([IO.Path]::GetTempPath()) -Filter ('sin-independent-'+$id+'-*.log') -File)) {
        if ($log.Name -cmatch ('^sin-independent-'+$id+'-[0-9]+\.log$')) {
            Copy-Item -LiteralPath $log.FullName -Destination $Folder
        }
    }
}
if ($PlanOnly) {
    Write-Output 'SIN_ONE_CLICK_PLAN_ONLY_NO_GAME_COPY_OR_LAUNCH'
    Write-Output 'SELECT: original installed game folder'
    Write-Output 'COPY: fresh disposable entire game copy under Windows Temp'
    Write-Output 'BINARIES: independent client/server DLLs shipped in package bin'
    Write-Output 'TEST: presence first, then read only on success and confirmed restoration'
    if ($StagedStats) {
        Write-Output 'THIRD TEST: rb/GAME definitions and rt/null layout, independent 66-ID binding'
        if ($HookProof) {
            Write-Output 'HOOK PROOF: three independently owned CUserMessages hook IDs 27:28:5, not dispatch'
        }
    }
    Write-Output 'RESULTS: new text-only ZIP in extracted package folder'
    Write-Output 'CLEANUP: keep disposable copy if restoration is uncertain'
    return
}

$exitCode=2
$reports=$null
$temporary=$null
$disposable=$null
$launchStarted=$false
$presenceOk=$false
$readOk=$false
$stagedOk=$false
$stagedStarted=$false
$problems=New-Object 'System.Collections.Generic.List[string]'
$copyStatus='NOT_STARTED'
$cleanup='NOT_STARTED'
$id=[Guid]::NewGuid().ToString('N').Substring(0,10)
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
Write-Output 'SiN Episodes standalone filesystem test. No manual DLL copying required.'
try {
    if ([string]::IsNullOrWhiteSpace($GameRoot)) {
        $GameRoot=Select-Game
    }
    $source=Authenticate-Source $GameRoot
    Require-Plain $bin $true
    Require-Plain (Join-Path $bin 'client.dll') $false
    Require-Plain (Join-Path $bin 'server.dll') $false
    Require-Plain $gate $false
    $reports=Join-Path $PSScriptRoot ('Results\'+$stamp+'-'+$id)
    if (Test-Path -LiteralPath $reports) {
        throw 'Refusing to overwrite an existing results folder'
    }
    [void](New-Item -ItemType Directory -Path $reports)
    $bytes=Scan-Source $source
    $tempBase=[IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    $drive=[IO.DriveInfo]::new([IO.Path]::GetPathRoot($tempBase))
    if ($drive.AvailableFreeSpace -lt $bytes+268435456L) {
        throw 'Not enough free disk space for safe disposable game copy'
    }
    $temporary=Join-Path $tempBase ('sin-independent-fs-'+$id)
    if (Test-Path -LiteralPath $temporary) {
        throw 'Disposable folder already exists; refusing reuse'
    }
    [void](New-Item -ItemType Directory -Path $temporary)
    $disposable=Join-Path $temporary 'game'
    [void](New-Item -ItemType Directory -Path $disposable)
    Write-Output 'Making an isolated game copy. The installed game is not modified.'
    $copyLog=Join-Path $reports 'copy-summary.txt'
    & robocopy.exe $source $disposable /E /XJ /R:0 /W:0 /NFL /NDL /NJH /NJS /NP |
        Out-File -LiteralPath $copyLog -Encoding UTF8
    $copyExit=$LASTEXITCODE
    if ($copyExit -gt 7) {
        throw ('Disposable game copy failed, robocopy status '+$copyExit)
    }
    if (-not (Verify-OriginalPair $disposable)) {
        throw 'Disposable copied DLLs do not match authenticated originals'
    }
    $copyStatus='COPY_VERIFIED_ORIGINALS'
    [IO.File]::WriteAllText(
        (Join-Path $disposable 'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY.txt'),
        'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY_V1'+[Environment]::NewLine)
    $presence=Join-Path $reports 'presence.txt'
    $presenceRun=Join-Path $reports 'presence-run.txt'
    $launchStarted=$true
    Write-Output 'Checking whether the original engine sees both statistics files.'
    try {
        $options=@{DisposableGameRoot=$disposable;IndependentBin=$bin;
            Phase='Presence';TranscriptPath=$presence;
            TimeoutSeconds=$TimeoutSeconds;DisposableTestCopyConfirmed=$true}
        & $gate @options *>&1 | Tee-Object -FilePath $presenceRun | Out-Host
    } catch {
        $problems.Add('presence: '+$_.Exception.Message)
    }
    if (Test-Path -LiteralPath $presenceRun) {
        $presenceOk=([IO.File]::ReadAllText($presenceRun)).Contains(
            'OWNER_ENGINE_FS_GATE_RESULT: PRESENCE_DIAGNOSTIC_ONLY_NO_GAME_BOOT')
    }
    Preserve-SessionLogs $presence $reports
    if ($presenceOk) {
        $read=Join-Path $reports 'read.txt'
        $readRun=Join-Path $reports 'read-run.txt'
        Write-Output 'Presence confirmed. Testing bounded read-only file access.'
        try {
            $options=@{DisposableGameRoot=$disposable;IndependentBin=$bin;
                Phase='Read';PriorPresenceTranscript=$presence;
                TranscriptPath=$read;TimeoutSeconds=$TimeoutSeconds;
                DisposableTestCopyConfirmed=$true}
            & $gate @options *>&1 | Tee-Object -FilePath $readRun | Out-Host
        } catch {
            $problems.Add('read: '+$_.Exception.Message)
        }
        if (Test-Path -LiteralPath $readRun) {
            $readOk=([IO.File]::ReadAllText($readRun)).Contains(
                'OWNER_ENGINE_FS_GATE_RESULT: READ_DIAGNOSTIC_ONLY_NO_GAME_BOOT')
        }
        Preserve-SessionLogs $read $reports
    } else {
        Write-Output 'File presence was not confirmed. The read phase is skipped.'
    }
    if ($StagedStats -and $readOk) {
        $stagedStarted=$true
        $staged=Join-Path $reports 'staged.txt'
        $stagedRun=Join-Path $reports 'staged-run.txt'
        Write-Output 'Checking original rb/GAME and rt/null modes, independent 66-stat catalog and optional hook proof.'
        try {
            $options=@{DisposableGameRoot=$disposable;IndependentBin=$bin;
                Phase=$(if($HookProof){'Hooks'}else{'Staged'});PriorReadTranscript=$read;TranscriptPath=$staged;
                TimeoutSeconds=$TimeoutSeconds;DisposableTestCopyConfirmed=$true}
            & $gate @options *>&1 | Tee-Object -FilePath $stagedRun | Out-Host
        } catch {
            $problems.Add('staged: '+$_.Exception.Message)
        }
        if (Test-Path -LiteralPath $stagedRun -PathType Leaf) {
            $stagedOk=([IO.File]::ReadAllText($stagedRun)).Contains(
                $(if($HookProof){'OWNER_ENGINE_FS_GATE_RESULT: HOOKS_DIAGNOSTIC_ONLY_NO_GAME_BOOT'}else{'OWNER_ENGINE_FS_GATE_RESULT: STAGED_DIAGNOSTIC_ONLY_NO_GAME_BOOT'}))
        }
        Preserve-SessionLogs $staged $reports
    } elseif ($StagedStats) {
        Write-Output 'Prior read was not verified; original-mode staged phase skipped.'
    }
    $exitCode=if($StagedStats){
        if($stagedOk){0}else{2}
    }else{
        if($readOk){0}else{2}
    }
} catch {
    $problems.Add('setup: '+$_.Exception.Message)
    Write-Output ('Test blocked: '+$_.Exception.Message)
    $exitCode=3
} finally {
    # A failed restored state must NOT be silently deleted, even from a clone.
    if ($null -ne $temporary -and (Test-Path -LiteralPath $temporary -PathType Container)) {
        $restored=$true
        if ($launchStarted) {
            $restored=$false
            $last=if($stagedStarted){'staged.txt'}elseif($presenceOk){'read.txt'}else{'presence.txt'}
            if ($null -ne $reports) {
                $transcript=Join-Path $reports $last
                if (Test-Path -LiteralPath $transcript -PathType Leaf) {
                    $restored=([IO.File]::ReadAllText($transcript)).Contains(
                        'OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE')
                }
            }
        }
        # The 2026-11-20 real owner read proved VFileSystem012 Read but
        # immediate rollback hit UnauthorizedAccessException on the TEMP
        # copy's client.dll. Attempt bounded SHA-authenticated recovery
        # before preserving the clone and asking the owner to do cleanup.
        if (-not $restored -and $launchStarted) {
            $repair=Join-Path $PSScriptRoot 'Recover-SiNRetainedTestCopy.ps1'
            try {
                $recoveryOutput=@(& $repair -CopyRoot $temporary)
                foreach($line in $recoveryOutput) {Write-Output $line}
                $restored=@($recoveryOutput | Where-Object {
                    $_ -like 'RECOVERY_VERIFIED_ORIGINAL_DLLS:*'
                }).Count -eq 1
                if ($restored) {
                    Write-Output 'ONECLICK_AUTO_RECOVERY: VERIFIED_ORIGINALS'
                    # Preserve the real filesystem read milestone when
                    # its initial gate result was masked by an exclusively
                    # post-exit restoration refusal.
                    if (-not $readOk -and $presenceOk -and -not $stagedStarted) {
                        $sourceTranscript=Join-Path $reports 'read.txt'
                        if (Test-Path -LiteralPath $sourceTranscript -PathType Leaf) {
                            $evidence=@([IO.File]::ReadAllLines($sourceTranscript))
                            $exitSeen=@($evidence | Where-Object {
                                $_ -ceq 'OWNER_ENGINE_EXIT_HEX: 0x53494E6B'
                            }).Count -eq 1
                            $readSeen=@($evidence | Where-Object {
                                $_ -cmatch '^OWNER_CLIENT_ENGINE_FS_READ_RESULTS: sin-independent-[0-9a-f]{32}-[0-9]+\.log:client:both-present:parsed-and-verified:62:8:156:66$'
                            }).Count -eq 1
                            if ($exitSeen -and $readSeen) {
                                $readOk=$true
                                $exitCode=0
                                Write-Output 'ONECLICK_RECOVERED_READ_DIAGNOSTIC: ORIGINAL_ENGINE_FS_COUNTS_VERIFIED'
                            }
                        }
                    }
                }
            } catch {
                $problems.Add('automatic restoration retry: '+$_.Exception.Message)
            }
        }
        # Staged test may have completed in the original Windows engine but
        # a post-exit file lock prevented the gate from confirming cleanup.
        # Do NOT claim success unless the exact new-phase session and terminal
        # evidence match, and both original retail DLLs are now restored.
        if ($StagedStats -and $stagedStarted -and -not $stagedOk -and $restored) {
            $stagedTranscript=Join-Path $reports 'staged.txt'
            if (Test-Path -LiteralPath $stagedTranscript -PathType Leaf) {
                $evidence=@([IO.File]::ReadAllLines($stagedTranscript))
                $sessions=@($evidence | Where-Object {
                    $_ -cmatch '^OWNER_INDEPENDENT_SESSION: [0-9a-f]{32}$'
                })
                if ($sessions.Count -eq 1) {
                    $match=[regex]::Match($sessions[0],
                        '^OWNER_INDEPENDENT_SESSION: ([0-9a-f]{32})$')
                    $sid=$match.Groups[1].Value
                    $pattern=('^OWNER_CLIENT_ENGINE_FS_STAGED_RESULTS: sin-independent-'+
                        $sid+'-[0-9]+\.log:client:owned-staged-verified-not-engine-init:66:62:8:156:66:7:7$')
                    $expectedExit=if($HookProof){'OWNER_ENGINE_EXIT_HEX: 0x53494E74'}else{'OWNER_ENGINE_EXIT_HEX: 0x53494E70'}
                    $hooksEvidence= (-not $HookProof) -or (
                        @($evidence | Where-Object {
                            $_ -ceq 'OWNER_CLIENT_ENGINE_FS_HOOK_PROBE: ENABLED_TERMINAL_DIAGNOSTIC'
                        }).Count -eq 1 -and
                        @($evidence | Where-Object {
                            $_ -cmatch ('^OWNER_CLIENT_ENGINE_FS_HOOK_RESULTS: sin-independent-'+$sid+'-[0-9]+\.log:client:three-owned-hooks-bound-not-dispatched:27:28:5:3$')
                        }).Count -eq 1)
                    if (@($evidence | Where-Object {
                            $_ -ceq $expectedExit
                        }).Count -eq 1 -and $hooksEvidence -and
                        @($evidence | Where-Object {
                            $_ -cmatch $pattern
                        }).Count -eq 1 -and
                        @($evidence | Where-Object {
                            $_ -ceq 'OWNER_CLIENT_ENGINE_FS_STAGED_PROBE: ENABLED_TERMINAL_DIAGNOSTIC'
                        }).Count -eq 1) {
                        $stagedOk=$true
                        $exitCode=0
                        Write-Output $(if($HookProof){'ONECLICK_RECOVERED_HOOK_DIAGNOSTIC: OWNED_27_28_5_VERIFIED'}else{'ONECLICK_RECOVERED_STAGED_DIAGNOSTIC: ORIGINAL_MODES_AND_INDEPENDENT_CATALOG_VERIFIED'})
                    }
                }
            }
        }
        $backup1=Join-Path $disposable 'SE1\bin\sin_independent_saved_client.dll'
        $backup2=Join-Path $disposable 'SE1\bin\sin_independent_saved_server.dll'
        if (-not $restored -or (Test-Path -LiteralPath $backup1) -or
            (Test-Path -LiteralPath $backup2) -or
            -not (Verify-OriginalPair $disposable)) {
            $cleanup='RETAINED_FOR_MANUAL_RECOVERY: '+$temporary
            $problems.Add('Temporary copy retained because DLL restoration is not verified')
        } else {
            try {
                Remove-Item -LiteralPath $temporary -Recurse -Force
                $cleanup='VERIFIED_AND_REMOVED_DISPOSABLE_COPY'
            } catch {
                $cleanup='TEMPORARY_COPY_CLEANUP_REFUSED: '+$temporary
                $problems.Add('Temporary copy cleanup failed: '+$_.Exception.Message)
            }
        }
    } else {
        $cleanup='NO_TEMPORARY_COPY'
    }
    if ($null -ne $reports) {
        $summary=Join-Path $reports 'SUMMARY.txt'
        $rows=@(
            'SiN Episodes 2006 independent filesystem diagnostic'
            ('Presence: '+$(if($presenceOk){'VERIFIED_DIAGNOSTIC'}else{'NOT_VERIFIED'}))
            ('Read: '+$(if($readOk){'VERIFIED_DIAGNOSTIC'}else{'NOT_VERIFIED_OR_SKIPPED'}))
            ('Original-mode staged 66-stat binding: '+$(if($stagedOk){'VERIFIED_DIAGNOSTIC'}elseif($StagedStats){'NOT_VERIFIED_OR_SKIPPED'}else{'NOT_REQUESTED'}))
            ('Independent Stats CUserMessages hooks 27:28:5: '+$(if($HookProof -and $stagedOk){'VERIFIED_DIAGNOSTIC'}elseif($HookProof){'NOT_VERIFIED_OR_SKIPPED'}else{'NOT_REQUESTED'}))
            ('Copied original files: '+$copyStatus)
            ('Temporary game cleanup: '+$cleanup)
            'No complete normal VClient011::Init or playable map boot is claimed.'
        )
        foreach($problem in $problems) {$rows+=('Problem: '+$problem)}
        [IO.File]::WriteAllLines($summary,[string[]]$rows)
        $zip=Join-Path $PSScriptRoot ('SiN_FileSystem_Test_Results_'+$stamp+'-'+$id+'.zip')
        try {
            Compress-Archive -LiteralPath $reports -DestinationPath $zip -CompressionLevel Optimal
            Write-Output ('RESULTS_ZIP: '+$zip)
        } catch {
            Write-Output ('Unable to create results ZIP. Results folder: '+$reports)
        }
        Write-Output ('RESULTS_FOLDER: '+$reports)
    }
}
if ($HookProof -and $stagedOk) {
    Write-Output 'FINISHED: original-mode staged Stats plus three owned user-message hooks verified. NO actual dispatch or game boot.'
} elseif ($StagedStats -and $stagedOk) {
    Write-Output 'FINISHED: original-mode staged stats and owned 66-ID binding verified. NOT an engine Init or playable game boot.'
} elseif (-not $StagedStats -and $readOk) {
    Write-Output 'FINISHED: both retail filesystem diagnostics passed, NOT a playable game boot.'
} else {
    Write-Output 'FINISHED: test incomplete or refused. Share RESULTS_ZIP for analysis.'
}
exit $exitCode
