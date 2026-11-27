SiN Episodes: Emergence (2006) original DLL Interface Factory Win32 probe
=======================================================================

This package contains:
  sin_original_interfacereg_owner_probe.exe
  Run-OwnerOriginalInterfaceFactoryProbe.ps1
  OwnerOriginalInterfaceFactoryProbe-README.txt

NO ORIGINAL GAME DLLs OR CONTENT ARE INCLUDED.

Windows 32-bit original factory interface readback:
  - Exact original on-disk client.dll SHA-256:
    6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
  - Exact original on-disk server.dll SHA-256:
    568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
  - Refuses LoadLibraryEx when either SHA differs or a file is missing
  - Uses the original CreateInterface singleton thunks and reads PE32
    interface-registry records without executing any vtable method
  - Verifies original client 55 and server 10 InterfaceReg record counts
  - Verifies original client GameClientExports001 4-slot loaded vtable
  - Verifies original VClientEntityList003 secondary +0x10014 factory identity,
    distinct parent/secondary vptrs and 9+3 code-pointer slots (read-only)
  - Reads original client entity-list initial count/max/flagged/highwater
    DWORDs after DllMain with a bounded 32-bit read-only observer
  - Verifies original VClientPrediction001 singleton vptr and ALL 23
    original prediction virtual targets after DllMain, no method invocation
  - Reads original cl_predict ConVar's m_pParent at +0x18 and its numeric
    m_nValue at +0x2C only when m_pParent==&cl_predict; never follows
    an external alias, and never invokes ConVar or prediction functions
  - Verifies original server ServerGameClients003 15-slot loaded vtable
  - Does NOT launch SinEpisodes.exe, load a map, or test independent game DLLs

IMPORTANT: This tool LOADS the original DLLs inside its own Windows process.
Normal original DllMain/static constructors execute and may have side effects.
Only use original, unmodified game files with their dependencies in a
DISPOSABLE COPY controlled by the original owner. Never copy originals into
public GitHub, CI, or a shared report.

Self-test (requires no game files):
  powershell -NoProfile -ExecutionPolicy Bypass -File ".\Run-OwnerOriginalInterfaceFactoryProbe.ps1" -SelfTest

Owner-original test, from this package directory (edit both paths to match):
  powershell -NoProfile -ExecutionPolicy Bypass -File ".\Run-OwnerOriginalInterfaceFactoryProbe.ps1" -ClientDll "C:\SiN-Test-Copy\bin\client.dll" -ServerDll "C:\SiN-Test-Copy\SE1\bin\server.dll" -OwnerDisposableCopyConfirmed

Expected output includes JSON events:
  original_interfacereg_snapshot
  original_gameclientexports_vtable
  original_client_entitylist_secondary_vtable
  original_client_entitylist_startup_state
  original_client_prediction23_vtable
  original_cl_predict_convar_startup
  original_servergameclients_vtable

The entity-list JSON event reports the 65556-byte secondary interface
adjustment, nine secondary slots, three separate parent slots, and zero
method invocations. Its four-byte inter-table gap is NOT treated as a
virtual. Runtime method semantics remain out of scope.

Output prints status, counts, expected counts and matched vtable slots only.
The Phase1657 startup-state event additionally shows four bounded original
entity-list integer counters, without printing original process pointers.
It does NOT print raw original machine-code bytes, process pointers, or
original game object contents.

The Phase1658 prediction event requires all 23 pointers in the original
prediction object's virtual table to match the SHA-pinned retail image.
It does NOT invoke the original prediction Init, which reads the
cl_predict ConVar's m_pParent pointer and that parent's numeric
m_nValue (+0x2C). The owner diagnostic verifies the self-parent case
using only two bounded read-only DWORDs, and explicitly refuses to
follow external ConVar parent aliases or unknown object memory. Retail DllMain
may still have side effects, so always use a disposable owner copy.

A green result means the unmodified original DLLs' factory and static
registrations match this read-only observer. It does NOT prove independent
replacement DLLs initialize or gameplay works. Windows / Android / Quest 3
startup remains unverified until engine-compatible independent modules boot.
