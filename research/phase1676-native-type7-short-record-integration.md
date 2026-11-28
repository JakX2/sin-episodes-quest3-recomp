# Phase 1676: original-free native type-7 short-record integration

## What is new

This phase **does not claim newly recovered original machine-code bytes**. It connects a *previously authenticated, independently reconstructed* original type-7 signed-short pipeline to the current native MSVC x86 PostNetwork dispatcher and its native signed-short preview.

The former Phase218 research and code already establish the four retail type-7 call sites, in source order, inside the original 1,036-byte typed dispatcher at 0x241996F0:

| Retail call site | Destination | Native method role | Original method bytes | x86 return |
|---|---|---|---:|---|
| 0x241999F4 | 0x24197D50 | 16-bit WORD comparison | 84 | RET12 |
| 0x24199A05 | 0x241989E0 | gated WORD correction copy | 48 | RET16 |
| 0x24199A10 | 0x24197B10 | signed WORD differences and status logs | 79 | RET16 |
| 0x24199A1F | 0x24198380 | selected-record signed-short preview | 33 | RET16 |

All four complete native SHA256 values and original CALL edges are already pinned by `research/tools/audit_sin_original_client_short_specialists.py`. The last 33-byte method is also separately covered by Phase1675's full original SHA/relocation/literal auditor. Neither the original game engine nor opaque logger 0x24197FE0 is reproduced here.

## New independent Win32 composition

`pc/independent_slot0/test_original_postnetwork_type7_integrated.cpp` uses:

- The **actual MSVC x86** Phase1672 four-argument nonvirtual `__thiscall Dispatch` with RET16, real 0x34-byte records, flag filtering, and current-record pointer at `this+0x18`.
- The already-tested, platform-independent `sin_compat::original_client_snapshot_short::run_type7_direct_calls` as a safe host model of the original compare/copy/diagnostic ordering. This callback is a synthetic substitute for the retail case implementation, **not an x86 binary translation of the original specialist body**.
- The **actual MSVC x86** Phase1675 four-argument nonvirtual `__thiscall FormatShortIfSelected` RET16 method, called *after* the corrected-data diagnostics, preserving original `this+0x34 == this+0x18` selection behavior.
- Explicit host-owned byte buffers, bounds checks, a trusted signed-short pointer and injected diagnostic callbacks. Original commercial game memory is never loaded.

The test covers two type-7 records, a mismatch followed by a two-WORD correction, an already-equal record, signed `-32768` and `-1` preview, diagnostics observing post-correction bytes, correction-disabled behavior, comparison-disabled but correction-enabled behavior, descriptor bit `0x04` forced-match, early `0x0200` flag skip, selected-record isolation and ESP preservation across actual native Win32 calls.

## Verification / scope

Adds **one** original-free MSVC x86 CTest on top of Phase1675's verified **298/298** baseline. Phase1676's target is **299**; do not report 299 passed until the exact phase head and Windows test log have completed successfully.

No original DLL bytes are bundled in GitHub or run in CI. The added test does **not** prove original Windows Source engine boot, production prediction interface registration, gameplay parity, Android or Quest3 boot. It adds no physics, materials, surfaces, props, model loaders, or rendering work.

Source evidence: `research/tools/audit_sin_original_client_short_specialists.py`, `research/tools/audit_sin_original_client_snapshot_record_router.py`, `research/tools/audit_sin_original_postnetwork_type_dispatch_13case.py`, and `research/tools/audit_sin_original_postnetwork_scalar_formatters.py`. Safe portable behavior: `include/sin_compat/original_client_snapshot_short.hpp`.
