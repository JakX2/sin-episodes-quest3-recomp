# Phase 1702: 1,082 original source member offsets for the 2006 PE32 client receive schema

## Runtime objective and strict limit

The only owner-proven original SiN Episodes Windows engine execution remains the independent replacement \`client.dll\` successfully accepting 21 source interfaces, sound Connect, prediction, entity list, centerprint and ClientLeaf before deliberately terminating at \`VClient011.slot5\` (diagnostic \`0x53494E45\`). The original commercial DLL was restored. This phase **does not unblock retail slot5**, create valid native RecvProxy callbacks, implement networking wire format, or run the game on Windows/Android/VR.

The 211-table/1,110-static-property source inventory in Phases1696–1700 lacked the actual original **member offsets** for most native 60-byte RecvProp descriptors. A host-owned image with correct type numbers but zero-filled offset fields can pass a graph walk while remaining structurally unlike the game.

## Original 2006 DLL source authentication

The **private** original user-owned \`client.dll\` is read-only and whole-SHA256 pinned:

\`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d\`.

Using actual original native x86 constructor callsites, this audit identifies the direct immediate offset argument to **1,082 canonical receive-property constructor calls**. For standard types 0, 1, 2, 3 and ordinary type5 this corresponds to the original native RecvProp \`+44\` member-offset DWORD. This is not a packet offset or an assumption from a modern HL2 SDK. The original source address and table/property order were checked against 214 table constructor calls, 211 distinct static tables and all 1,110 original property destinations. Canonical offsets range from 0 to 4,676 bytes; zero can be a legitimate recovered offset.

The **remaining 28** properties take separate constructors and are **explicitly unknown in this direct-immediate offset inventory**:

- **13** original native type4 array descriptors, which pass array element counts/widths through their specialized constructor.
- **14** type5 generated-array outer descriptors, whose authenticated parent bases and generated-child relative offsets are already modeled separately from Phases1697/1699.
- **1** specialized \`DT_OverlayVars.m_AnimOverlay\` descriptor with unresolved native callback/ownership semantics.

The source static native type histogram remains exactly **414 / 300 / 164 / 12 / 13 / 207** (types 0 through 5).

## Compact immutable offset manifest

\`include/sin_compat/original_client_native_member_offsets.hpp\` stores exactly 1,110 two-byte little-endian entries in authentic original source order: first the 191 linked-list ClientClass tables, then 20 original VA-sorted auxiliary tables. The special 28 descriptors retain the sentinel \`0xFFFF\`, **not** an executable property offset.

The original-free stream is 2,220 bytes and SHA256-pinned to:

\`155043d70cc995d37f5812bb818d257224627c9fbbbff8628e384fab7a59dd04\`.

The public header's concatenated hex was checked independently against the source-exported private manifest. Its computed 64-bit FNV-1a is \`0xbb1def4950b6b973\` on both sides. Compile-time checks require the exact byte length, hexadecimal formatting, 1,082 known offsets, 28 unresolved markers and all 28 exceptions to reside on the expected type4/type5 native constructor families.

## Host-image behavior and tests

The existing \`original_client_native32_recvtable_image.hpp\` now writes the 1,082 source-authenticated member-offset DWORDs to each corresponding property descriptor \`+44\` during \`Image::Build()\`. The existing Phase1701 \`AuditVisitedSourceTable()\` checks every reached known field against immutable original source metadata. A **valid but source-incorrect offset** now causes refusal.

The remaining 28 special descriptors are not supplied with guessed member offsets. For the 14 source-generated tables, existing verified parent-base and child-relative element logic still populates those respective native fields. For type4 and specialized overlay semantics, the new inventory does **not** invent offsets, names, flags, creator callbacks or RecvProxies.

The already-wired native C++ test checks all 1,082 reconstructed static offsets and retains the original tree shape: **191 class roots -> 224 tables / 1,796 property descriptors**; exhaustive source inventory **225 tables / 1,801 descriptors**. It additionally mutates a known, in-range offset and verifies guarded refusal and restoration. The existing original engine's unreachable auxiliary \`DT_Animationlayer\` remains excluded from the class-root walk.

The new read-only Python audit \`research/tools/audit_sin_original_client_member_offsets.py\` has two modes:

\`\`\`sh
# Public original-free CI: verifies the committed source stream hash.
python research/tools/audit_sin_original_client_member_offsets.py --self-test

# Local owner-only: rederive all 1,082 offsets from the original DLL.
python research/tools/audit_sin_original_client_member_offsets.py \
  --client /private/unchanged/client.dll
\`\`\`

Source-private mode refuses a mismatched original DLL SHA, mismatched table count/order, missing canonical constructor offset, and any difference between derived offsets and the committed source stream. CI contains only original-free tests, strict C++17 and sanitizers, MSVC Win32 x86 and Android NDK/Bionic smoke. No owner's binary bytes are uploaded to GitHub.

**Next native ABI gap:** exact flags, proxy callbacks, type4 array/overlay native layout, source table name ownership, class creators and lifetime. Once the engine can safely traverse and retain *genuine executable* native objects, retest the disposable Windows replacement. Retain the \`VClient011.slot5\` guard until then. User-deferred HL2-equivalent physics, models/props, surfaces and rendering remain excluded.
