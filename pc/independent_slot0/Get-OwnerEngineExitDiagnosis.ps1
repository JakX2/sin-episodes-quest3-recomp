# Offline classification of a real Windows process exit status, never boot proof.
# The launcher records the exact unsigned NTSTATUS bit pattern, including when
# .NET represents high-bit Windows exit codes as negative signed Int32 values.
[CmdletBinding()]
param([Parameter(Mandatory=$true)][long]$ExitCode)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($ExitCode -lt -2147483648 -or $ExitCode -gt 4294967295) {
    throw 'ExitCode outside Windows 32-bit process status range'
}
$bits = [uint32]($ExitCode -band 0xFFFFFFFFL)
$hex = ('0x{0:X8}' -f $bits)
$meaning = switch ($hex) {
    '0x00000000' { 'PROCESS_EXITED_ZERO_NOT_BOOT_PROOF'; break }
    '0xC0000135' { 'STATUS_DLL_NOT_FOUND'; break }
    '0xC000007B' { 'STATUS_INVALID_IMAGE_FORMAT'; break }
    '0xC0000142' { 'STATUS_DLL_INIT_FAILED'; break }
    '0xC0000139' { 'STATUS_ENTRYPOINT_NOT_FOUND'; break }
    '0xC0000005' { 'STATUS_ACCESS_VIOLATION'; break }
    '0xC0000409' { 'STATUS_STACK_BUFFER_OVERRUN_OR_FAIL_FAST'; break }
    default { 'UNCLASSIFIED_EXIT_STATUS' }
}
Write-Output ('OWNER_ENGINE_EXIT_HEX: {0}' -f $hex)
Write-Output ('OWNER_ENGINE_EXIT_DIAGNOSIS: {0}' -f $meaning)
# These are generic Windows statuses, NOT proof of which DLL caused exit.
