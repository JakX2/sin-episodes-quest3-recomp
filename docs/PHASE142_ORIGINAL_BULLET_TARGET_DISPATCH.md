# Phase 142: original bullet target filtering and TraceAttack routing

**Evidence:** the preserved Phase 42 SHA-pinned static-analysis report for the owner's 2006 SiN server.dll (SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650). Phase 42 records exact instruction bytes, the target virtual calls, and RTTI-checked class vtables. This phase reuses that evidence; it does not claim a fresh disassembly of the DLL.

## Recovered target-hit order

The original player FireBullets path calls the hit target's PassesDamageFilter virtual at byte offset +0xD8. A false result skips the target's TraceAttack virtual at +0xDC; a true result dispatches it. The target's class-specific vtable entries were checked for seven original classes:

| Original class | PassesDamageFilter | TraceAttack | Reconstructed route |
|---|---:|---:|---|
| CBaseEntity | 0x2213C880 | 0x2214D2C0 | base filter, base trace |
| CBaseAnimating | 0x2213C880 | 0x2214D2C0 | base filter, base trace |
| CBaseCombatCharacter | 0x2213C880 | 0x2214D2C0 | base filter, base trace |
| CBasePlayer | 0x2213C880 | 0x22246BD0 | base filter, player trace |
| CHL2_Player | 0x2239E430 | 0x22246BD0 | player override, player trace |
| CPhysicsProp | 0x2213C880 | 0x222FF890 | base filter, copy-then-base trace |
| CPropContainer | 0x2213C880 | 0x222FF890 | base filter, copy-then-base trace |

The base filter compares the complete 32-bit damage-type value for exact equality with 0x00400000 and 0x00010000, rejecting either value. These are not bit-mask tests. Otherwise it resolves the target's damage-filter handle from the original +0xA4 field; a live filter receives its own PassesDamageFilter call at +0xD8, while an absent/unresolved filter follows the allow path. CHL2_Player has an earlier exact 0x00400000 rejection before delegating to the base behavior.

This distinction matters for DUAG: its registered damage bits are 0x00400002. That combined value is not equal to 0x00400000, so it is not rejected by the two exact-value checks; it proceeds to the optional map damage filter and then TraceAttack if allowed.

## Independently authored implementation

Added sin_original_bullet_target_dispatch:

- Reproduces the exact-value rejections and the optional damage-filter callback order.
- Preserves the CHL2_Player override as a separate route instead of collapsing it into a generic mask check.
- Selects the verified base/player/physics-prop TraceAttack route for the seven classes above.
- Uses generation-aware host entity references. The host resolves its native filter handle and owns every engine call.
- Forwards an optional opaque host damage-info payload without reading, retaining, or assuming its layout.
- Fails closed on host lookup/callback failures and on unknown target classes.

No original x86 object offsets, vtable slots, or function addresses are called on ARM64.

## Explicit limits

This is the target filter and TraceAttack dispatch boundary, not a complete hitscan weapon. It does not reconstruct bullet spread, penetration, the later impact-effect gates, MultiDamage/health application, armor, knockback, or all CTakeDamageInfo fields. The SourceVR native game-module loader remains unsupported for custom client.dll/server.dll game code, and this component is not a SourceVR module or a Quest 3 gameplay test.

## Verification boundary

The focused C++17 regression covers exact-type rejection, the combined DUAG type, player override ordering, optional/stale filters, filter rejection, all three TraceAttack routes, and host failures. Standard C++ CI compiles and runs it. No new synthetic binary audit, placeholder .so, APK, or emulator smoke test was added.

The owner-held binsin.zip and SourceVR archive were located in Library, but raw-byte materialization was denied for both in this run. Therefore no new original PE/ELF bytes were read or disassembled here; the implementation is grounded in the previously preserved Phase 42 report, not a fresh binary scan.


## Native host and mod-loader gate (checked 9 November 2026)

The current public SourceVRPort README says its custom-mod path is for content-only additions and explicitly says mods that include their own client.dll or server.dll game code are not supported ([SourceVR README](https://github.com/tinsarfal/SourceVR/blob/main/README.md)). Its public repository tree currently exposes only the README and release-validation notes, with no engine source tree, public native game-module SDK, or LICENSE file ([SourceVR repository](https://github.com/tinsarfal/SourceVR)). Its private native binaries therefore cannot be treated as a documented extension API or assumed redistributable.

The public Source SDK 2013 license grants use to develop a modified Valve game running on Valve's Source 1 engine; it does not itself grant a standalone SiN game-engine license ([Valve Source SDK 2013 LICENSE](https://github.com/ValveSoftware/source-sdk-2013/blob/master/LICENSE)). A separate written permission or a genuinely independent engine implementation is needed before choosing that route.

**Conclusion:** no supported, publicly documented, legally established native host/mod loader for SiN game-code DLLs is currently identified. SourceVR content mounting is not a game-code loader, and the Source SDK 2013 grant is not a substitute for a SiN/standalone engine license. Do not build the release path around undocumented binary injection or a guessed loader symbol.
