# Phase 109: original 2006 parent-attachment matrix selector

Source: owner's unchanged SiN Episodes: Emergence Windows x86
`server.dll`, SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
This research contains no original binary or raw disassembly. Names here
describe observed compiled behavior; they are not recovered Ritual C++ symbols.

## Direct binary evidence

| Original function | Virtual address range | Size |
| --- | --- | ---: |
| Parent-to-world selector | `0x2213D960..0x2213DA25` | 198 bytes |
| Attachment position/angles query | `0x220ED470..0x220ED527` | 184 bytes |
| Identity matrix3x4 writer | `0x22269F60..0x22269F7E` | 31 bytes |
| Angles plus origin to matrix3x4 | `0x2226CC20..0x2226CC49` | 42 bytes |

The SHA-pinned `research/tools/audit_sin_original_parent_attachment.py`
checks **four exact complete-function hashes, nine relative call
targets and sixteen specific opcode anchors**. Its original-free regression
test uses artificial PE32 metadata and explicitly rejects altered headers.

## Original 0x2213D960 control flow

1. Read child's parent handle `+0xF0`. Invalid sentinel, mismatching
   generation serial or null entity table entry takes the no-parent path,
   writing an identity matrix via `0x22269F60` to caller output.
   The global handle table pointer resides at `0x224F5620`; the
   original compares the low 12-bit slot index and upper generation.
2. On valid parent, if child's attachment index `+0xEC` is **nonzero**,
   call parent virtual byte offset `+0xC4` to obtain a usable animating
   interface. Only when nonnull, call `0x220ED470`, supplying the
   attachment selector and two destination vectors.
3. If the attachment getter returns **true**, call `0x2226CC20` with
   the retrieved angles and origin and return that attachment matrix.
4. If the getter returns **false**, or there is no animating interface
   or selected attachment, check the parent's flags `+0xB8` for bit
   `0x800`. Refresh the dirty parent with `0x2213FA60`, and return
   a pointer to its cached world matrix at `+0x268`.

### Why failure handling matters

`0x220ED470` itself calls the animating interface's virtual byte slot
`+0x2C4` to retrieve an attachment transform. If successful it extracts
angles with `0x22268A60`, origin with `0x22268FD0`, and returns true.
If unsuccessful, it may refresh the parent's world transform and write
fallback world origin and angles to its output vectors, **but returns
false**. The outer selector **does not use those fallback vectors to
build an attachment matrix**. It instead selects the parent's cached
world matrix directly.

This detail is especially relevant for animated weapons and entities
attached to skeletal bones: failed attachment lookups must not invent
an attachment orientation or discard the correct parent transform.

## Independent C++17 reconstruction

- `original_parent_attachment.hpp/.cpp`: `resolve_original_parent_matrix`
  with identity, attachment and cached-parent outcomes, independent
  entity IDs and safe value copying.
- `OriginalParentMatrixWorldAdapter`: bridge to Phase 108's entity
  snapshots and world-transform refresh behavior.
- `OriginalAttachmentWorldTransformAdapter`: supplies the exact
  attachment-getter **boolean success gate** to the Phase 108
  `IOriginalWorldTransformHost` callback, retaining original-style
  parent-matrix fallback on failure.
- New C++ tests cover stale handles, missing parents, dirty caches,
  animated and nonanimated parents, failed and valid attachments,
  90-degree rotation, nested world transforms, invalid inputs and
  exception boundaries. The verifier also has synthetic PE32 tests.

Owner-side binary audit (do not commit game files):

```sh
python3 research/tools/audit_sin_original_parent_attachment.py \
  --server /owner/private/SiN/server.dll
```

Portable repo tests:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Limits and native ABI work

Original x86 function returns either a **borrowed pointer** to parent
matrix cache or a pointer to a scratch matrix. The portable model
returns a checked, owned copy, intentionally avoiding unsafe Windows
PE32 pointer assumptions. The original does not establish numerical
guards/exception recovery; the portable code fails closed for invalid
host observations.

True generation-aware EHANDLE lookup, SourceVR's real animation
interfaces at vtable byte `+0xC4` and `+0x2C4`, matrix output lifetime,
dirty-cache propagation, engine-owned model/attachment data, and
ARM64 native callbacks are still absent. A successful desktop CTest or
Android NDK compilation **does not** demonstrate actual game entities,
campaign startup, or a playable standalone Quest 3 build.

No original proprietary DLL, maps, models, assets or disassembly included.
