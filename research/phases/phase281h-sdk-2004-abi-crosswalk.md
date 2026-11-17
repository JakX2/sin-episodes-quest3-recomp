# Phase 281-H: use Source SDK 2004 as the primary ABI reference

## Correction to previous project direction

The old Source SDK has **actual C++ game module source and versioned
engine interface declarations**. We should NOT reconstruct the whole
`client.dll` / `server.dll` architecture by treating every Ghidra result
as an unrelated unknown function.

Archived SDK source: [Source SDK 2004](https://github.com/Source-SDK-Archives/source-sdk-2004/tree/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod).
The archive contains Valve-derived code, which is **not MIT-licensed**.
This repository's new crosswalk records only API member identifiers,
reference paths and candidate address metadata. Do not paste or vendor
Valve's copyrighted source implementations into our MIT tree without
satisfying the Source SDK terms and reviewing distribution rights.

## Verified reference relationships

The 2004 SDK declares the exact older version strings observed in the
SHA256-authenticated **SiN Episodes: Emergence (2006)** binaries:

| SiN interface | SDK 2004 declaration / implementation reference | SDK 2004 virtual slots | Reviewed original PE32 pointer prefix | Interpretation |
| --- | --- | ---: | ---: | --- |
| `VClient011` | [IBaseClientDLL](https://github.com/Source-SDK-Archives/source-sdk-2004/blob/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod/public/cdll_int.h); [CHLClient](https://github.com/Source-SDK-Archives/source-sdk-2004/blob/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod/cl_dll/cdll_client_int.cpp) | 47 | 48 | One extra pointer; boundary or modified interface must be reviewed |
| `ServerGameDLL003` | [IServerGameDLL](https://github.com/Source-SDK-Archives/source-sdk-2004/blob/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod/public/eiface.h); [CServerGameDLL](https://github.com/Source-SDK-Archives/source-sdk-2004/blob/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod/dlls/gameinterface.cpp) | 29 | 31 | Two extra pointers; do not assume SDK slots apply automatically |
| `ServerGameClients003` | [IServerGameClients](https://github.com/Source-SDK-Archives/source-sdk-2004/blob/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod/public/eiface.h) | 15 | 15 | Candidate full slot count match, not ABI equivalence proof |
| `ServerGameEnts001` | [IServerGameEnts](https://github.com/Source-SDK-Archives/source-sdk-2004/blob/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod/public/eiface.h) | 7 including virtual destructor | 7 | Candidate slot count match |
| `GameClientExports001` | [IGameClientExports](https://github.com/Source-SDK-Archives/source-sdk-2004/blob/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod/public/cl_dll/IGameClientExports.h) | 4 including IBaseInterface virtual destructor | 4 | Candidate slot count match |
| **Total** | | **102** | **105** | Three more observed pointers than SDK declarations |

In particular, the original SDK's `IBaseClientDLL` slot-zero candidate
is `Init`, slot-one candidate is `Shutdown`; `IServerGameDLL`'s
slot-zero candidate is `DLLInit` and slot-one candidate is `GameInit`.
**These remain unverified SiN slot-name hypotheses**, even where version
strings agree. The original DLL may modify methods while keeping a
compatible or identical version identifier. Slot prefixes are not proof of
actual vtable end boundaries.

The 2006 Episode One SDK already changed versions to
`VClient013` and `ServerGameDLL005`, while the modern SDK 2013
uses `VClient017` and `ServerGameDLL012`. Copying newer class
definitions into SiN's PE32 replacement would shift layout and callbacks.

## Implementation

`research/tools/build_source_sdk_2004_crosswalk.py` uses version-pinned
SDK 2004 **method names and order only**, never SDK source bodies.
It strictly validates Phase 280's original SHA-authenticated metadata and
joins the 105 target addresses to SDK slot-name hypotheses.

Each output slot contains `sdk_symbol_name_verified_as_sin_method=false`
without exception. The additional client/server pointer positions have
`null` SDK method names, not fabricated names. This tool also rejects
incorrect digests, missing/extra/reordered slots, duplicate targets, and
pointer entry address mismatches. It never reads any original game bytes
or emits original instructions or SDK C++ code.

The integrated owner-original Actions job writes
`source_sdk_2004_interface_crosswalk.json` alongside the existing
metadata-only artifacts, **only after full SHA256 authentication**.

## Next gameplay-first work

1. Use 2004 `IBaseClientDLL` and `IServerGameDLL` declarations as the
   *candidate* virtual ABI blueprint. Verify the real SiN slot 0/1/2
   using the 14 original-method Ghidra private reviews and Windows owner
   live vptr probe before writing callable replacement methods.
2. Start from SDK 2004 factory registration, game-system lifecycle,
   entity registration, client/server networking tables and save/restore
   architecture. Port/rewrite SiN-specific gameplay behaviors in
   appropriately licensed source, rather than recreating Valve engine
   subsystems from scratch.
3. Implement one real Windows 32-bit `VClient011` /
   `ServerGameDLL003` interface handshake and boot it in a backed-up
   owner game copy, recording the first unmet engine factory dependency.

Do **not** equate Ghidra-seeded function boundaries, matching pointer
counts, or a synthetic boot with independent retail-engine execution.
The current independent PC game boot is still unproven.
