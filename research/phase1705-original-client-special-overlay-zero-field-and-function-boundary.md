# Phase 1705: original special overlay +44 is zero, 621-byte native function boundary

## Source-proven correction

The owner's exact original Windows SiN Episodes (2006) PE32 client.dll is
SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

The original specialized animation overlay builder begins at
`0x241271B0`. The real native function's last instruction is
`RET` at `0x2412741C`, followed by three INT3 padding bytes at
`0x2412741D..0x2412741F`. An **independent function** begins at
`0x24127420` (`PUSH ESI; MOV ESI,ECX`).

Thus the exact original *executable function* is **621 bytes**,
SHA256 `a41703fce2d6cd865d2457b21738fa927ee63cda2ca34b473188d213528df4c6`.
The earlier Phase1698/1699 reported **657-byte** SHA256
`b2e9bf21b248f751855489484f6ac04d4a8809473406e3287701a1ed4188c670`
is reproducible **but spans into the following function**, so should
never have been described as a complete single executable function.
This correction does not invalidate the original array/table type
evidence; it precisely fixes the source function boundary.

### Exact native RecvProp field +44

At `0x241271BA`, the builder initializes a 60-byte local RecvProp
at `ESP+0x1C` (after saved registers). Original instructions
`0x241271DF XOR EBX,EBX` and `0x241271E1 PUSH ECX` adjust the
current stack position; original `0x241271EE MOV [ESP+0x4C],EBX`
writes **zero into local RecvProp +0x2C (44)**. At
`0x24127405 MOV ECX,15` and `0x24127410 REP MOVSD`, the full
60-byte initialized descriptor is copied to the source property.
The source callsite `0x240C36C9 PUSH 0x7A4` supplies **1956 as
an auxiliary parent-object offset** for specialized proxy allocation.
It is **not the native outer RecvProp +44**.

This closes the original byte +44 member field inventory from
**1,109/1,110** to **1,110/1,110 source-accounted static descriptors**.
The original special overlay **type5 child table and specialized native
proxy functions remain unresolved and unexecutable** in the independent
client. Knowing field +44 is zero does not implement ownership, genuine
overlay callbacks, packet processing, or actual game startup.

## Safe model and verification

- The committed `research/manifests/original_client_static_recvprops_phase1704.csv`
  now explicitly records `DT_OverlayVars.m_AnimOverlay` +44 as zero,
  route `special_overlay_zero_initialized`. This replaces the earlier
  deliberately unknown Phase1704 field, not a cosmetic column change.
- New LF CSV SHA256:
  `ee990e1ee8a57ffc095ac2fbd49b29619ac28259df4b683a275215446d34f045`.
  Source table|slot|name SHA256 and FNV64 remain unchanged because no
  property names were changed.
- `research/tools/audit_sin_original_client_overlay_zero_field.py`
  authenticates the entire original *621-byte method*, previous
  overlapping 657-byte digest, exact function boundary, stack-relative
  +44 zero store, full 60-byte descriptor copy and original caller's
  separate `0x7A4` auxiliary argument.
- The host-owned native32 image writes the source-proven zero explicitly,
  validates it on the 191-class-root graph and refuses mutations that
  replace it with `0x7A4`. Its special type5 nested-table child
  pointer remains intentionally NULL and blocked.
- Focused Linux strict C++17 and ASan/UBSan, Windows native CTest and
  Android original-free compilation retain their existing contract.
  The original-owner source audit may be run privately:
  `python research/tools/audit_sin_original_client_overlay_zero_field.py --client /private/client.dll`.
  Original-free public mode is
  `python research/tools/audit_sin_original_client_overlay_zero_field.py --self-test`.

**No retail `VClient011.slot5` execution, genuine native receiver
proxy, Source networking serialization, original game map boot, or
Quest 3 gameplay is established by this metadata correction.**
Original DLLs remain outside GitHub. All game-module safety guards
remain closed. The user's excluded HL2-equivalent physics, models,
props, surfaces and rendering work remains deferred.
