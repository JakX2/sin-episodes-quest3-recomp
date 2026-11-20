# Owner-only recovery for TEST COPIES created by Start-SiNFileSystemTest.
# Never edits a retail installation. Every candidate must be a DIRECT child
# of Windows TEMP with exact name, original 2006 SHA backup and marker.
# Does not kill processes or delete an unverified backup. Public CI uses
# -PlanOnly plus synthetic objects and no actual game.
[CmdletBinding()]
param(
    [string]$CopyRoot,
    [switch]$ScanTemp,
    [switch]$CleanupRecoveredCopy,
    [switch]$PlanOnly
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$clientHash='6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d'
$serverHash='568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650'
$temp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
function Digest([string]$Path) {
    # Older Windows PowerShell runners can lack Get-FileHash. Use the same
    # .NET SHA-256 routine as the original authenticated owner stager.
    $hasher=[Security.Cryptography.SHA256]::Create()
    $stream=[IO.File]::OpenRead($Path)
    try {
        return [BitConverter]::ToString($hasher.ComputeHash($stream)).Replace('-','').ToLowerInvariant()
    } finally {
        $stream.Dispose()
        $hasher.Dispose()
    }
}
function Plain([string]$Path,[bool]$IsDirectory) {
    if (-not (Test-Path -LiteralPath $Path -PathType $(if($IsDirectory){'Container'}else{'Leaf'}))) {
        throw ('Missing recovery path: '+$Path)
    }
    $item=Get-Item -LiteralPath $Path -Force
    if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw ('Refusing symlink/junction recovery path: '+$Path)
    }
}
function Check-Root([string]$Path) {
    $candidate=[IO.Path]::GetFullPath($Path).TrimEnd('\')
    $parent=[IO.Path]::GetDirectoryName($candidate).TrimEnd('\')
    $name=[IO.Path]::GetFileName($candidate)
    if (-not [string]::Equals($parent,$temp,[StringComparison]::OrdinalIgnoreCase) -or
        $name -cnotmatch '^sin-independent-fs-[0-9a-f]{10}$') {
        throw 'Recovery allowed only in exact Windows TEMP sin-independent-fs-<10 hex> test copies'
    }
    Plain $candidate $true
    Plain (Join-Path $candidate 'game') $true
    Plain (Join-Path $candidate 'game\SE1') $true
    Plain (Join-Path $candidate 'game\SE1\bin') $true
    $marker=Join-Path $candidate 'game\SIN_INDEPENDENT_DISPOSABLE_TEST_COPY.txt'
    Plain $marker $false
    if (([IO.File]::ReadAllText($marker)).Trim() -cne
            'SIN_INDEPENDENT_DISPOSABLE_TEST_COPY_V1') {
        throw 'Disposable-copy marker does not match; do not touch'
    }
    Plain (Join-Path $candidate 'game\SinEpisodes.exe') $false
    Plain (Join-Path $candidate 'game\SE1\GameInfo.txt') $false
    return $candidate
}
function Recover-One([string]$Path) {
    $root=Check-Root $Path
    $bin=Join-Path $root 'game\SE1\bin'
    $live=Join-Path $bin 'client.dll'
    $saved=Join-Path $bin 'sin_independent_saved_client.dll'
    $server=Join-Path $bin 'server.dll'
    Plain $server $false
    if ((Digest $server) -cne $serverHash) {
        throw 'Server DLL is not the SHA-pinned original; refuse recovery'
    }
    if (Test-Path -LiteralPath (Join-Path $bin 'sin_independent_saved_server.dll')) {
        throw 'Unexpected server backup; refuse mixed-role recovery'
    }
    $backupPresent=Test-Path -LiteralPath $saved
    if ($backupPresent) {
        Plain $saved $false
        if ((Digest $saved) -cne $clientHash) {
            throw 'Saved client original SHA256 mismatch; refuse any removal'
        }
    }
    if (Test-Path -LiteralPath $live) {Plain $live $false}
    $liveOriginal=(Test-Path -LiteralPath $live -PathType Leaf) -and
         ((Digest $live) -ceq $clientHash)
    if ($backupPresent -and $liveOriginal) {
        throw 'Both original client and original backup exist: ambiguous, preserve'
    }
    if (-not $backupPresent -and -not $liveOriginal) {
        throw 'No trusted original client DLL or original backup available'
    }
    if ($PlanOnly) {
        Write-Output ('RECOVERY_PLAN_ONLY: '+$root)
        Write-Output ('RECOVERY_ORIGINAL_BACKUP: '+$(if($backupPresent){'SHA_VERIFIED'}else{'ALREADY_RESTORED'}))
        return
    }
    if ($backupPresent) {
        $finished=$false
        $last='none'
        for($attempt=1; $attempt -le 18; ++$attempt) {
            try {
                if ((Digest $saved) -cne $clientHash) {
                    throw 'Original backup SHA changed during recovery'
                }
                if (Test-Path -LiteralPath $live) {
                    Remove-Item -LiteralPath $live -Force -ErrorAction Stop
                }
                Move-Item -LiteralPath $saved -Destination $live -ErrorAction Stop
                if ((Digest $live) -cne $clientHash) {
                    throw 'Restored client SHA verification failed'
                }
                $finished=$true
                break
            } catch {
                $last=$_.Exception.Message
                if (-not (Test-Path -LiteralPath $saved -PathType Leaf)) {
                    throw 'Original backup no longer exists; keep test copy for inspection'
                }
                if ($attempt -eq 18) {break}
                Write-Output ('RECOVERY_RETRY: attempt='+$attempt+'/18')
                Start-Sleep -Milliseconds 500
            }
        }
        if (-not $finished) {
            throw ('Locked test DLL: original SHA-pinned backup preserved. '+$last)
        }
    }
    if (-not (Test-Path -LiteralPath $live -PathType Leaf) -or
        (Digest $live) -cne $clientHash -or
        (Digest $server) -cne $serverHash -or
        (Test-Path -LiteralPath $saved)) {
        throw 'Post-recovery original DLL verification failed: preserve test copy'
    }
    Write-Output ('RECOVERY_VERIFIED_ORIGINAL_DLLS: '+$root)
    if ($CleanupRecoveredCopy) {
        Remove-Item -LiteralPath $root -Recurse -Force -ErrorAction Stop
        if (Test-Path -LiteralPath $root) {
            throw 'Temporary copy deletion did not complete'
        }
        Write-Output ('RECOVERY_VERIFIED_TEMPORARY_COPY_REMOVED: '+$root)
    } else {
        Write-Output ('RECOVERY_VERIFIED_TEMPORARY_COPY_RETAINED: '+$root)
    }
}
if ($PlanOnly -and [string]::IsNullOrWhiteSpace($CopyRoot) -and -not $ScanTemp) {
    Write-Output 'RECOVERY_PLAN_ONLY_NO_GAME_FILES_TOUCHED'
    Write-Output 'RECOVERY_SCOPE: direct children of Windows TEMP, exact sin-independent-fs-<10 hex> names'
    Write-Output 'RECOVERY_REQUIREMENTS: disposable marker, SHA-pinned saved original client and server DLL'
    Write-Output 'RECOVERY_ACTION: bounded retries; delete temporary clone only if both original DLLs restored'
    return
}
if ($ScanTemp -and -not [string]::IsNullOrWhiteSpace($CopyRoot)) {
    throw 'Use either -ScanTemp or -CopyRoot, not both'
}
$targets=@()
if($ScanTemp) {
    $targets=@(Get-ChildItem -LiteralPath $temp -Directory -Filter 'sin-independent-fs-*' |
        Where-Object { $_.Name -cmatch '^sin-independent-fs-[0-9a-f]{10}$' } |
        Sort-Object Name | ForEach-Object { $_.FullName })
} elseif (-not [string]::IsNullOrWhiteSpace($CopyRoot)) {
    $targets=@($CopyRoot)
} else {
    throw 'Specify -ScanTemp or a trusted -CopyRoot'
}
if ($targets.Count -eq 0) {
    Write-Output 'RECOVERY_NO_RETAINED_TEST_COPIES'
    return
}
$blocked=0
foreach($target in $targets) {
    try {
        Recover-One $target
    } catch {
        ++$blocked
        Write-Output ('RECOVERY_BLOCKED: '+$target+' reason='+$_.Exception.Message)
    }
}
if ($blocked) {
    throw ($blocked.ToString()+' candidate test copy/copies could not be verified; no unsafe forced removal')
}
