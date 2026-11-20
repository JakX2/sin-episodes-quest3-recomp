# Phase 480: audited CClientShadowMgr Init top-level side effects

## Private original-binary verification

The user's original SE1/bin.zip was retrieved read-only from connected
Drive and both original DLLs were checked against previously established
SHA pins. This specific study uses only the exact owner original
client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

The original CClientShadowMgr::Init VA 0x24118880 has a 146-byte
method-body SHA256
32dbbce9d4da9dce01522425c0531510640a5841a662c01abcec443df3d0d438.

Private read-only checks matched the entire original method body and
ten distinct original side-effect / branch / return instruction sites.

## Verified ordered top-level behavior

1. Set original CClientShadowMgr byte at this+0xCD to zero.
2. Construct/configure original member at this+0x14 through
   helper 0x24167810, passing observed opaque constants
   0x24297D14 and 0x2427D384.
3. Invoke manager virtual byte slot +0x60 with three float32 bits
   0x3DCCCCCD, 0x3DCCCCCD, 0xBF800000
   (approximately 0.1f, 0.1f, -1.0f).
4. Invoke virtual byte slot +0x6C with float32 0x42480000 (50.0f).
5. Invoke virtual byte slot +0x70 with float32 0x3BA3D70A
   (approximately 0.005f).
6. Set original manager dword at this+0xD8 to 8.
7. If the external original object at global 0x243581C0 has a
   nonzero field at +0x2C, call helper 0x24118610.
8. Invoke another object's virtual byte slot +0x1D0 with the
   original callback VA 0x24115E70.
9. Return true unconditionally after the above calls.

**Neither the virtual callees nor the conditional helper have been
reimplemented in this phase.** The original globals, renderer
managers, resources, callback registry and actual object ownership
are unresolved.

## Independent implementation and tests

client_shadow_mgr_known_init_sequence.h models only the two
definitively recovered original object fields and the exact
call/order/constants through injected, independently authored
callbacks. It refuses missing callback dependencies BEFORE writing
any owned state, a safety policy of the new port and not a claim
that the original engine performs equivalent preflight.

Its Win32 test exercises both branches of the original external
shadow flag, all exact bit patterns, callback ordering and the
final mode, plus missing hook refusal with no partial state writes.
No original callback pointer is cast or invoked.

audit_sin_original_shadow_mgr_known_init.py contains the full
original SHA-pinned read-only auditor and an independent fabricated
synthetic marker test for public CI. Original DLLs never enter CI
artifacts and raw original disassembly is never committed.

## Position in overall game startup

Previous client_original_gamesystem_sequence.h tracks 12
registered AutoGameSystems with eight previously mapped Init bodies
and four still marked incomplete. This work narrows the remaining
CClientShadowMgr top-level behavior but **DOES NOT** mark it fully
implemented, because every actual renderer side effect remains
in callback placeholders. The previous four-pending status is
intentionally retained.

C_StatsSystem's owned-value model is farther along but still lacks
HUD integration. C_SoundscapeSystem and CPerfVisualBenchmark also
remain unresolved. Actual VClient011::Init must not return success
until system construction, callbacks, assets and shutdown are real.

Real retail-engine proven milestone remains 0x53494E74, and no
real game packet, menu, map or gameplay is claimed by this phase.
