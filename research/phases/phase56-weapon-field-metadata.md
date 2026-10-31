# Phase 56: Named reload state and server/client field-layout comparison

**Scope:** static reverse engineering of the specific 2006 SiN Episodes Windows x86 DLLs identified by the SHA-256 values in [`../binary-provenance.json`](../binary-provenance.json). This report does **not** establish a working engine integration, actual per-weapon script values, or behavior in a running game.

## What is newly verified

The original DLLs contain typed, 52-byte field descriptor records containing a type identifier, a pointer to an embedded field-name string and an object offset. Their entries provide a stronger identification of the previously anonymous reload fields:

| Field name | Descriptor type | Server offset | Client offset | Count server / client |
|---|---:|---:|---:|---:|
| `m_bReloadsSingly` | 6 (boolean) | `0x508` | `0x7FC` | 2 / 1 |
| `m_bInReload` | 6 (boolean) | `0x4AC` | `0x7A0` | 2 / 1 |
| `m_iClip1` | 5 (integer) | `0x4EC` | `0x7E0` | 2 / 1 |
| `m_iClip2` | 5 (integer) | `0x4F0` | `0x7E4` | 2 / 1 |

Server descriptor virtual addresses (`server.dll` base `0x22000000`):

| Name | Descriptor addresses |
|---|---|
| `m_bReloadsSingly` | `0x2251FD78`, `0x225204CC` |
| `m_bInReload` | `0x2251FB3C`, `0x22520088` |
| `m_iClip1` | `0x2251FA6C`, `0x225201F4` |
| `m_iClip2` | `0x2251FAA0`, `0x22520228` |

Client descriptor virtual addresses (`client.dll` base `0x24000000`):

| Name | Descriptor address |
|---|---|
| `m_bReloadsSingly` | `0x242F6810` |
| `m_bInReload` | `0x242F65D4` |
| `m_iClip1` | `0x242F6504` |
| `m_iClip2` | `0x242F6538` |

For all four fields, the **client offset is exactly `server offset + 0x2F4`** for this build. This does *not* imply identical whole-object layouts, a universal offset adjustment, automatic networking, or matching ABI across ports. Implementations should use explicit, typed engine interfaces instead of memcpy of these structures.

### Connection to previous phases

- Phase 54/55 identified `weapon+0x508` as a conditional one-round reload branch and showed its initialization to zero in the inspected firearm constructors.
- The actual field name **`m_bReloadsSingly`** confirms the name/meaning of that flag in the metadata, while `m_bInReload` and clip-count names similarly match previous code analysis.
- The repeated server descriptor records and the single client records are factual; we have **not yet proven which registration paths consume each table**. Their presence alone is insufficient to prove save/restore, prediction or replication behavior.
- Neither the metadata nor the earlier constructor work proves that the Magnum, assault rifle or Scattergun **ever enables** single-round mode during actual gameplay. That is still an open question.

## How to reproduce

Provide your legally obtained, **unmodified original** DLLs, then run:

```sh
python3 research/tools/verify_weapon_field_metadata.py /path/to/sin/server.dll /path/to/sin/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
```

The verifier uses only the Python standard library. It verifies both SHA-256 hashes, parses the PE32 image base/sections, locates the embedded field-name strings, finds the typed pointer-bearing metadata descriptors, and requires the exact offsets and descriptor counts. It fails on unexpected binaries. CI runs the synthetic PE-parser tests without shipping game binaries. No DLL bytes, object-code fragments or proprietary assets are committed.

## Next questions

1. Identify the functions/registration structures pointing at the *two* server descriptor groups and the client group to determine how the fields are serialized, predicted or networked.
2. Perform a register-alias-aware cross-reference audit of runtime writes to `m_bReloadsSingly`, avoiding false positives from other classes that also have a field at offset `0x508`.
3. Obtain user-owned weapon script data/models where permitted, and validate per-gun ammo capacities and reload sequence events.
4. Integrate these fields into a tested SourceVR adapter **only** when the exact engine branch and lifecycle ABI are known.
