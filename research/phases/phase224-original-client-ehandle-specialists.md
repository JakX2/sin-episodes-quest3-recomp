# Phase 224: original client type-13 EHandle direct-call chain

14 November 2026. **Stacked DRAFT on Phase223 PR #278.** Directly verified the user's original SiN Episodes: Emergence 2006 Windows PE32 client/server game DLLs from private Drive bin.zip.

- client.dll (3,592,192 bytes), original SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- server.dll (6,049,792 bytes), original SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

No original DLL, complete proprietary original method body, raw native disassembly, maps or other game assets are committed.

## Four complete newly SHA-reviewed original native methods

| Original client.dll entry | Complete x86 bytes | Whole original native SHA256 | Phase212 direct CALL | RET pop / INT3 pad |
| --- | ---: | --- | --- | --- |
| 0x24199060 | **160** | 2a5a05257fce45a49037e02d10d537b429f617c8f174bbacc8f471b1fecf343d | 0x24199A93 | 12 / 0 |
| 0x24197E90 | **49** | fb6a1f47681fbd9b2ab63d77985fe211a4925308b032a83a1306260598fa7794 | 0x24199AA2 | 16 / 15 |
| 0x241994C0 | **288** | df09716ab9b3746c9e7dfb9b44e8b910d26462de937bfaa8805580cd6a386fa5 | 0x24199AB1 | 16 / 0 |
| 0x241995E0 | **175** | ac19b58ac8891a70cdb75e226bf717f1a1b99e8e8f4b62f405393a9c9d90ccb2 | 0x24199AC0 | 16 / 1 |

**Four complete original SHA256-reviewed native functions, 672 native x86 bytes.** Compare method ends immediately before next method at 0x24199100, and diagnostics ends immediately before preview at 0x241995E0. There is zero INT3 padding in these two boundaries. The remaining padding and neighboring method entry witnesses are independently checked.

Original Phase212 router type selector maps **type 13 to case 11**, case VA 0x24199A8E. Its direct original CALL order is: resolved-pointer comparison 0x24199060, conditional forward DWORD copy 0x24197E90, diagnostics 0x241994C0, and optional preview 0x241995E0.

## Original EHandle pointer comparison, not handle integer comparison

Native 160-byte comparator returns status 0 immediately if BYTE(this+0x14) is zero; descriptor flags bit 0x04 forces status 1; signed DWORD record count <=0 also returns status 1. Otherwise each DWORD EHandle is resolved through the original process-global entity-list pointer at original .data VA 0x242FA734:

- EHandle 0xFFFFFFFF resolves to null.
- Slot index = handle & 0xFFF.
- Slot address = entity list base + 4 + slot index * 16.
- Slot serial DWORD at slot+4 must equal handle >> 12; otherwise null.
- If serial matches, the resolved pointer is DWORD at slot+0.
- The comparator compares the **resolved pointer values**, not the input EHandle DWORDs. Two distinct handles can compare equal if they both resolve to the same pointer, including the null pointer.

The native global layout is directly observed, but actual original entity-list lifetime, class, vtable and engine pointer ownership are not claimed. C++ reference requests slots from an explicit host callback instead.

## Forward copy, diagnostics and preview

The original 49-byte copy helper skips if BYTE(this+0x2C)==0, comparison result equals 1, or the **signed** DWORD count is <=0. Otherwise it copies complete DWORDs FORWARD, with cascading behavior for overlapping buffers. The host reference reuses the Phase213 forward-copy implementation with safe buffer and overflow checks.

The original 288-byte diagnostics helper is enabled only if BYTE(this+0x14)!=0. On comparison result 0 it resolves the first predicted and net handles and calls original native formatter 0x24197910 with authentic literal at 0x242A4230:

    EHandles differ (net) 0x%p (pred) 0x%p\n

Then all enabled paths format the first predicted handle, AFTER the copy helper has already run. If resolution is null, original formatter 0x241979C0 uses native literal at 0x242A420C:

    EHandle (NULL)

If nonnull, native method 0x240C5590 reads an entity name; when the name is empty, original code calls 0x24248F05 then 0x24249896 to obtain a fallback. Formatter 0x241979C0 then uses native literal at 0x242A421C:

    EHandle (0x%p->%s)

Native 175-byte preview checks DWORD(this+0x34)==DWORD(this+0x18), INDEPENDENT of enabled flag this+0x14. It resolves the first predicted handle, follows the same name/fallback/NULL cases and calls original formatter 0x24197FE0.

The verifier checks all **11 exact native nested CALL instructions** to original name/formatter methods, but their implementation BODIES remain opaque. The C++ reference exposes symbol/name callbacks, does NOT claim actual Source entity object ownership, and represents pointers as 32-bit identity tokens for testing.

The diagnostic runs AFTER the native copy, so a status 0 mismatch may report net and predicted as equal pointers once the prediction has already been updated. The independent host model preserves compare -> copy -> diagnostics -> preview sequencing.

## Verification and test coverage

- research/tools/audit_sin_original_client_ehandle_specialists.py: requires both complete original retail DLL fingerprints, checks four complete method SHA256s, **81** sparse complete native opcode witnesses, four exact Phase212 router CALLs, **11** nested native engine/formatter CALLs, all three original format strings, original return arities, INT3 pads, zero-padding boundaries, native neighbor entrypoints, global .data location and slot index/serial constants.
- research/tests/test_sin_original_client_ehandle_specialists.py: synthetic-only mutation of opcode witnesses, direct and nested CALLs, return arity, padding, original string literals, base and section; synthetic bytes never count as original whole-method SHA verification.
- include/sin_compat/original_client_snapshot_ehandle.hpp: clean-room portable C++17 entity-pointer resolver with 12-bit index and upper serial, conditional forward DWORD copy, name/NULL traces, conditional preview and original direct-call composition.
- tests/test_original_client_snapshot_ehandle.cpp: stale/missing/null handles, same entity pointer through different handles, early comparison mismatch, count/flag bypasses, forward overlap, fallback entity-name callbacks, null preview, and post-copy tracing.
- Root Linux CTest and native MSVC Windows x86 CTest registered. Host tests do NOT prove an original retail game engine loading reconstructed client.dll/server.dll.

Owner-only original audit:

    python3 research/tools/audit_sin_original_client_ehandle_specialists.py --archive /private/bin.zip

Original-free mutation tests:

    python3 -m unittest discover -s research/tests -p 'test_sin_original_client_ehandle_specialists.py' -v

## Checkpoint and next goal

Individual complete-method SHA-reviewed original function ledger **270 → 274** (**128 server, 146 client**). The **strictly scoped** Phase212 36-distinct-direct-target router graph is now fully reviewed, moving **four → zero** opaque direct CALLEE destinations. This is NOT total client/server DLL recovery, not a global decompilation percentage, and not a playable Windows binary. Nested formatter functions and many other original engine/game functions remain outside this graph.

Next highest-value target is the genuine Windows engine/game client/server module boundary, including native exported factories, class/interface ownership, and a verified retail Windows startup attempt. Android ARM64 and Quest 3 follow an actual working PC game.
