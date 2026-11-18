# Phase 347: original client console declaration inventory and accessor

Retail Phase 344 owner result: screen query returned 1920:1080;
Gaussian attachment verified; exit 0x53494E53; restoration verified.
The exact successful sequence is now identified in the classifier test
as observed, rather than synthetic. Alternate refusal traces remain synthetic.

Direct SHA-authenticated inspection of the owner original client.dll
(6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d)
recovers the six original constructor wrapper call counts:

| Wrapper RVA | Arguments | Count |
| --- | ---: | ---: |
| 0x121640 | 3 | 240 |
| 0x1216C0 | 4 | 129 |
| 0x121700 | 8 | 9 |
| 0x121750 | 5 | 4 |
| 0x1217A0 | 9 | 2 |
| 0x1215E0 | 5 (command) | 149 |

The 533 immediate constructor sites comprise 384 variable records
and 149 command records, including 82 +/- input commands. All object
RVAs are distinct. Case-insensitive names total 532: mat_fullbright
has two original declarations (default 0, flags 0x4000), at object
RVAs 0x33DCE8 and 0x358860. The metadata digest (canonical sorted
JSON) is ec58b826b784b5a296fdb5465ccd594aa456b1fd57d028f9833c85656fb75c4b.
There are 71 declarations with replicated flag 0x2000 and nine
with unregistered flag 1. There are 81 numeric, flags-zero,
three-argument candidates with no exact NUL-terminated name in
the SHA-verified server DLL. This is not proof that no engine,
plugin, or dynamically created declaration has the same name.

The original 9/18/13-entry client console vtables are at RVAs
0x2981A4, 0x2981CC and 0x298218. Their sizes and slot roles agree
with the already reconstructed native server console ABI. The
client startup calls its manager at 0xFE393 -> 0x120C70, passing
the accessor at 0x2FA4AC. The manager stores the accessor, caches
the pending next pointer and invokes virtual Init slot 8. The
accessor vtable is at 0x2968D4, slot zero at 0xFE000. Unlike the
server accessor, the client sets FCVAR_CLIENTDLL (0x8) through
AddFlags, clears next with 0x120E00 and calls ICvar registration
slot zero. It does not perform the server's replicated default
or command-line override logic. CreateBase excludes bit-1
declarations from the pending list. The earlier owner SDK
public/tier1/convar.h and tier1/convar.cpp corroborate the common
ABI and pending manager behavior.

Inspect-OwnerOriginalClientConsole.py reproduces the inventory
locally, including duplicate names and input command syntax.
client_console_registration_core.h models original prepend order,
exclusions, engine relinking, client flag, and no writes to an
engine root. Native fixture exercises two same-name variable
records, command, excluded and already registered entries, no
server override query, malformed lists, reentry and relink
preserving the engine root's value. The owned adapter is not
yet exposed to the retail engine. No original code bodies or
DLLs committed. This is not the complete original client registry,
particle system, VGUI, or full game startup.
