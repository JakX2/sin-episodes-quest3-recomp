# Gameplay Phase609: original Assault two serial-handle guards and state reset

## Original binary provenance

Direct retail SiN Episodes: Emergence 2006 source inspection using owner-connected Drive SE1/bin.zip. SHA256 of unmodified server.dll: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. SHA256 of unmodified client.dll: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

The original Assault eligibility virtual+0x18 method 0x220CEAF0 was completely SHA-authenticated in Phase607 (255 bytes). This phase authenticates its four native helper methods directly in the original 2006 DLL, corrects the missing-target Boolean meaning, and models the first original early-exit branch in separate host-only C++.

## Four whole native original methods, 204 bytes

| Original function role | Entry VA | Whole bytes | Whole original SHA256 |
| --- | --- | ---: | --- |
| First serial entity pointer resolver | 0x220CE210 | 42 | 3e1f38e2f06c1fd7b15f8268e7eecf5e4e21067cfa73e45348e71183740c9fa6 |
| First missing-target predicate | 0x220CE240 | 60 | 55c24daf3da453809e6e0692d604c1a7008273d6a08b2ec9f38c4bbc094c8f0a |
| Second serial entity pointer resolver | 0x220CE2F0 | 42 | 3e1f38e2f06c1fd7b15f8268e7eecf5e4e21067cfa73e45348e71183740c9fa6 |
| Second missing-target predicate | 0x220CE350 | 60 | 55c24daf3da453809e6e0692d604c1a7008273d6a08b2ec9f38c4bbc094c8f0a |

These are four separate original address ranges but **two distinct binary code bodies**: both 42-byte functions are exactly identical, and both 60-byte functions are exactly identical, confirmed directly by original whole-method byte comparison. All four real whole method SHA256s and genuine post-method 4 or 6 INT3 bytes are checked. Do not infer a common original C++ function name or shared linkage solely from binary-identical code.

## Exact handle checking behavior

Each helper reads a 32-bit encoded entity handle and checks invalid sentinel 0xFFFFFFFF, original entity table global pointer 0x224F5620, low 12-bit entity index and upper serial shift 12 against a 16-byte entry with serial at entry+4. The pointer resolver returns the engine entity pointer when serial matches, otherwise NULL. The missing-target predicate returns TRUE for an invalid handle, stale serial or NULL entity pointer; FALSE for a serial-matched valid nonnull entity.

**These Boolean functions test missing targets, not present targets.** Reversing their sign would incorrectly reject valid Assault behavior targets.

## Source-backed original Assault early-exit path

The authenticated 255-byte native Assault +0x18 eligibility method calls:
- missing-target helper 0x220CE240 at native 0x220CEB3A for the behavior handle at +0x14;
- missing-target helper 0x220CE350 at native 0x220CEB4A for the second behavior handle at +0x18.

When EITHER helper returns true, control branches to original 0x220CEBE0 and zeroes raw behavior DWORD+0x1C, BYTE+0x24 and BYTE+0x25, then returns AL=0. If both handles are present, processing continues into the complex Assault time/state/condition flow. Later two native calls at 0x220CEB73 and 0x220CEB7E target the original handle resolver helpers 0x220CE210 and 0x220CE2F0 to access engine entity pointers. All four genuine original E8 rel32 edges are verified.

Other Assault prerequisites and success cases remain unresolved: owner virtual+0x450, condition 0x3B checks, original owner virtual+0x154, target entity+0x3B9, x87 time comparison, byte+0x26 and native condition 0x18 interactions. Do not claim a completely reimplemented Assault behavior.

## Verifier and bounded portable model

- Original-SHA-gated research/tools/audit_sin_original_assault_handle_guards_phase609.py audits whole original server/client SHA256, existing Assault vtable+0x18 and 255-byte predicate whole SHA, four helper whole original method SHA/boundary, both pairs' byte-identity, four E8 native call edges and 26 sparse opcode witnesses.
- The original-free synthetic auditor mutates each byte of each of four authentic E8 encodings and rejects 20 corrupt branches and mismatched original-DLL SHA.
- include/sin_compat/original_grunt_assault_handle_guards_phase609.hpp uses explicit serial-checked entity host callback and faithfully checks the two handles in original short-circuit order; if either is missing, it zeroes raw state at +0x1C/+0x24/+0x25.
- tests/test_original_grunt_assault_handle_guards_phase609.cpp verifies valid pair preserves fields, both invalid/stale scenarios reset them, first failure short-circuits second and serial/index extraction is correct. No original binary is linked or required in CTests.

Native direct-source auditing against original bytes has verified the individual hashes and call edges. Reproducible private audit:

    python research/tools/audit_sin_original_assault_handle_guards_phase609.py --self-test
    python research/tools/audit_sin_original_assault_handle_guards_phase609.py --bin-zip /private/SE1/bin.zip

**Phase609: four new whole native methods and 204 authenticated original bytes. Phases601–609 total: 38 separate native function addresses and 11,004 original code bytes.** This does not mean 11,004 bytes of portable C++ source or a game-engine-compatible Windows server.dll. Full Windows/Android/Quest boot remains separate; generic HL2 physics/surfaces/models/props work remains explicitly excluded.
