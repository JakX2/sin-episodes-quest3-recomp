# 2006 SiN `server.dll`: animated bone attachment matrix (Phase 110)

**Owner-side original provenance:** PE32 `server.dll`, SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`,
preferred image base `0x22000000`. Original copyrighted binary, assets and
decompiled original source are not committed.

## Method and class linkage

Original `0x220ED470` is the Phase 109 attachment pose wrapper. Its
virtual call at byte vtable offset **`+0x2C4`** resolves to **`0x220ED530`**
in all eight MSVC RTTI-identified inherited vtables:

- `CBaseAnimating`
- `CBaseAnimatingOverlay`
- `CBaseFlex`
- `CBaseCombatCharacter`
- `CBasePlayer`
- `CHL2_Player`
- `CAI_BaseNPC`
- `CNPC_Jessica`

Each also exposes **`+0x2B0`** as `0x220ED1B0`, the native
bone-transform accessor. These bindings are checked directly against
the original DLL's complete-object locators and type descriptors. They
are x86 **byte offsets**, not ARM64 slot numbers or safe cross-module ABIs.

Original `0x220ED530` behavior inferred from verified instructions:

1. Query studio model resource via `0x22139A10`; ensure that the
   original cache/object passes its studio type check. Reject otherwise.
2. Query attachment count `0x2211B810`. Index 0 and indices above
   that count fail. Actual attachment indexes are **1-based**.
3. Subtract one and access the corresponding studio attachment record
   through `0x2211BFA0`. Its record stride is **92 bytes**. The record
   exposes flags at `+6`, the bone reference at `+8` and a local
   3×4 transformation matrix beginning at `+0xC`.
4. Resolve bone reference via `0x2211C000`, and obtain the current
   bone-world matrix through virtual `+0x2B0`.
5. If `flags & 0x100 == 0`, compose **boneWorld × attachmentLocal**
   through `0x22269520`. Return success.
6. If `flags & 0x100 != 0`, extract attachment-local translation
   column 3 through `0x22268FD0`; transform that point through bone
   world space via `0x22268B20`; initialize identity orientation
   via `0x22269F60`; write translated world position via
   `0x22269000`. Return success.

Consequently, the world-aligned branch follows the animated bone in
**position** without inheriting its rotation. Treating it as ordinary
matrix composition would rotate attached objects and scripted throws
incorrectly.

## Byte-level witnesses

`research/tools/audit_sin_original_studio_attachment.py` is a
read-only verifier that accepts an owner-supplied original PE32 DLL.
On the original provided specimen, it passes:

| Check | Quantity |
| --- | ---: |
| Complete SHA-256 function ranges | 8 |
| Exact original relative CALL targets | 15 |
| Instruction byte anchors | 15 |
| MSVC RTTI-backed inherited vtables | 8 |

The original function `0x220ED530..0x220ED62F` has SHA-256
`db55c501578dbbcdc70256310ae140ba26b90fc42fff5abb8c07b0f976fb28ef`.
No original executable content is packaged in the repository.
The synthetic audit test mutates fabricated PE headers and ensures
bad call opcodes, cross-section reads and non-original hashes are rejected.

## Independent C++17 reconstruction

- `include/sin_compat/original_studio_attachment.hpp` introduces
  `IOriginalStudioAttachmentHost`, `OriginalStudioAttachmentRecord`,
  `resolve_original_studio_attachment` and the
  `OriginalStudioNativeAttachmentAdapter`.
- `src/original_studio_attachment.cpp` reconstructs one-based index
  validation, bone-local matrix composition, world-aligned attachment
  matrices, and host-mediated studio/bone failures. The adapter converts
  a successful result to Phase 109's pose representation.
- Tests cover composed and world-aligned transforms, rotated bones,
  translated attachments, metadata validity, missing studio/count/record/
  bone, nonfinite matrices, float overflow, and proper fallback to
  parent world matrix or no valid parent.
- Integration exercises the existing Phase 109 parent-attachment
  selector and Phase 108 world-refresh host. This is not a direct
  call into the 2006 DLL, nor into a modern desktop HL2 executable.

## Crucial limits

The model cache and `CStudioHdr` layout, actual model-animation
`SetupBones`, bone-mask selection, engine-owned EHANDLE generation
validation, per-frame cache invalidation and vtable side effects
remain outside this feature. They require separately verified native
host adapters. The portable model deliberately protects against NaNs,
overflow and callback exceptions. Floating-point/angle conversion
may differ at the last bits from the original x87 instructions.

**Not yet playable.** No full rebuilt `server.dll`/`client.dll`,
SourceVR gameplay runtime or standalone Quest 3 Android app is claimed.
