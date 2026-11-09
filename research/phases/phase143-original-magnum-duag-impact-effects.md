# Phase 143: original CWeaponMagnum DUAG impact effects

**Evidence provenance:** preserved Phase 39 read-only verifier and clean-room reference contract for the owner's original 2006 server.dll, SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 (PE32/x86, image base 0x22000000). The current run could not materialize the owner-held ZIP, so it did not freshly disassemble the DLL. No original code bytes are committed.

## Preserved original evidence

- Original ammo registration gives DUAG damage bits 0x00400002 (0x00000002 bullet bit plus 0x00400000 special bit).
- Original CWeaponMagnum vtable slot 102 points to 0x22316910, the impact-effect override examined by Phase 39.
- At 0x22316927, the original instruction is test ebx, 0x00400000: the DUAG-specific gate is a bit test, not full-value equality.
- At 0x22316933, the original instruction is test byte ptr [esi+0x42], 0x04: raw bit 2 in the trace-like record's byte at +0x42 suppresses the two additional decals. The field's symbolic name is unresolved and remains raw in the port contract.
- The original resource strings are DUAGDecal1 and DUAGDecal2, referenced at the two original push sites 0x223169C5 and 0x223169D5.
- The preserved Phase 39 clean-room contract records the inherited/base impact effect before the SiN-specific gate. This implementation carries that ordering forward; the current run did not freshly re-check the base callsite against raw DLL bytes.

The preserved research/tools/phase39_verify.py checks the exact server DLL fingerprint, 20 instruction anchors, DUAG/MAG/BUCK strings, the two decal strings, four SniperBullet datamap descriptors and relevant vtable targets. It does not establish wall-penetration thickness, retained damage, or target-count formulas.

## Independently authored implementation

Added sin_original_magnum_impact_effects:

1. Invoke the host's inherited/base impact effect first.
2. Test damage_type & 0x00400000.
3. If the bit is absent, return after the base effect.
4. Test raw trace_flags_42 & 0x04.
5. If set, suppress both extra decals.
6. Otherwise dispatch DUAGDecal1, then DUAGDecal2.

The host owns native trace/material resolution and actual renderer/effect calls. The portable library reads only the verified 32-bit damage type and raw trace byte; the opaque host payload is borrowed and never dereferenced or retained. No original x86 vtable slot, object offset, function address, or engine ABI is invoked on ARM64.

## Verification and limits

The focused C++17 regression covers base-first ordering, non-DUAG ammo, DUAG's combined 0x00400002 value, bit-test rather than equality semantics, the 0x04 trace gate, unrelated trace bits, exact decal names, and opaque payload forwarding.

This adds one weapon impact-effect branch. It does not implement material penetration, wall thickness, damage retention, bullet spread, generic Source impact effects, a real decal renderer, or a complete client/server game module. It is not SourceVR execution or Quest 3 gameplay.

No proprietary DLL, APK, original ELF, BSP, model, script, or game asset is included. Keep the change stacked and draft; do not merge without explicit user instruction.
