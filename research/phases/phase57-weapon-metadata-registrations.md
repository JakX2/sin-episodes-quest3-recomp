# Phase 57: SiN weapon field-group registrations and runtime-built descriptors

**Evidence:** static analysis of the original 2006 x86 SiN Episodes `server.dll` and `client.dll` identified by the SHA-256 digests in [`../binary-provenance.json`](../binary-provenance.json). The Python verifier checks these original files locally when the researcher supplies them. **No proprietary binaries or instruction dumps are distributed.**

## Newly confirmed

The four named reload/clip fields recovered in Phase 56 belong to **two different server descriptor groups**, while the client contains a matching group of 29 descriptors. The original machine code sets descriptor-array pointers and counts directly:

| Group (research label) | Descriptor array VA | Registration stub | Declared count |
|---|---|---|---:|
| `server_matched` | `0x2251F864` | `0x222AC4F0` | 29 (`0x1D`) |
| `client_matched` | `0x242F62FC` | `0x24090EB0` | 29 (`0x1D`) |
| `server_extended` | `0x2251FFEC` | `0x222B0E07` | 42 (`0x2A`) |

All **29** static entries in the mirrored server/client arrays have identical field names, type IDs and array ordering. For **28** entries, client object offset minus server object offset is `0x2F4`. The one exception is entry 0, `m_nNextThinkTick`: server `+0x74`, client `+0x84`, difference `0x10`. This exception is important: **do not apply one universal offset transform to entire weapon objects**.

The 42-entry server array has **32 statically described fields** followed by **ten 52-byte slots that are zero-initialized in the on-disk PE**. The initializer code between `0x222B0940` and `0x222B0E07` contains direct writes to locations inside *every one* of those ten trailing slots. This makes the declared 42-entry length consistent with the 32 static records plus ten runtime-initialized records, rather than 42 independent static descriptors. It does not yet identify the purposes of the final ten entries or their runtime values.

The extended server group includes fields not present in the mirrored client/server group, for example `m_reloadState` at `+0x4A8`, `m_overridePrimaryAmmoCount` at `+0x4D4`, `m_overrideSecondaryAmmoCount` at `+0x4D8`, and `m_flUnlockTime` at `+0x514`. `m_bReloadsSingly` occurs in both groups at `+0x508`. Its presence in metadata **does not prove that normal gameplay enables it**.

### Evidence locations

- Server matched group: count store at `0x222AC4F0`, pointer store at `0x222AC4FA`.
- Client matched group: count store at `0x24090EB0`, pointer store at `0x24090EBA`.
- Server extended group: count store at `0x222B0E07`, pointer store at `0x222B0E11`.
- Server extended runtime-initializer range: `0x222B0940` through (excluding) `0x222B0E07`.
- First runtime slot: `0x2252066C`; last runtime slot: `0x22520840`, with stride `0x34`.

These VAs refer to the original files' preferred image bases, not Android addresses.

## What remains uncertain

The registration structures are distinct, but **we have not classified the downstream consumers**. The parallel 29-entry lists are *consistent* with some form of paired client/server weapon metadata; the 42-entry server group is *consistent* with broader server state metadata. They must **not** be categorically labeled prediction, replication, or save/restore tables until their consumers have been resolved. The ten runtime-filled slots are likewise not yet classified. This is not a decompilation of the full registration system.

This phase also does not establish which specific firearms turn on single-round reloading, nor does it produce a SourceVR-compatible build.

## Reproduction

With the unmodified, legally obtained original `server.dll` and `client.dll`:

```sh
python3 research/tools/verify_weapon_registration_groups.py /path/to/sin/server.dll /path/to/sin/client.dll
python3 -m unittest discover -s research/tests -p 'test_*.py' -v
```

The verifier uses only Python 3's standard library and the Phase 56 metadata reader. It checks DLL hashes, named descriptor identities, all three count/pointer initialization instruction patterns, the complete 29-entry mirror including the nonuniform offset exception, 32 static plus ten file-zeroed extended entries, and code references to all ten runtime slots. The test suite uses synthetic fixtures; no game bytes are needed in CI.

**Test record (31 October 2026):** Real-binary verifier passed on both supplied original DLLs. Eleven synthetic Python tests passed (five Phase 56 plus six Phase 57). The standalone C++ compatibility prototype and CTest remain separate engine-independent tests.

## Next research target

Find consumers of the three returned registration structures, including indirect/vtable or registration-manager calls. Distinguish prediction from datamap/save/restore using their actual callers and type flags. Then perform alias-aware writes analysis for the object `+0x508` field and seek independent runtime or weapon-script evidence for one-round reload activation.
