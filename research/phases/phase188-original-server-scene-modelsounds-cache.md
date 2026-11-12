# Phase 188: Original server scene/model-sounds cache initializers and reload paths

12 November 2026. Stacked on Phase187 draft PR #240, not merged. This pass directly inspects the owner-provided original 2006 Windows PE32 `server.dll` and `client.dll`. The original binaries and complete disassembly/function bodies remain private and are not committed. Windows PC gameplay first, Android second, Quest 3 third.

## Original binary provenance

- Original `server.dll`: 6,049,792 bytes, whole-file SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, PE32 image base `0x22000000`.
- Original `client.dll`: 3,592,192 bytes, whole-file SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
- Original prior Phase187 `CServerGameDLL` startup calls two small singleton thunks: `0x221AD8A0` passes `ECX=0x22503EE0` and jumps to `0x221AD3E0`; `0x22149B60` passes `ECX=0x224F0388` and jumps to `0x221498C0`. Both are mandatory after AutoGameSystem initialization.

### Four newly reviewed native original x86 function regions

| Role | Original VA | Length | Complete original code SHA256 |
|---|---|---:|---|
| `scene.cache` initializer | `0x221AD3E0` | 576 | `7b9db352473a8fc2d5343473b673ac52c89a8c98320847b0b4dd451dcf8e4e69` |
| `modelsounds.cache` initializer | `0x221498C0` | 576 | `94b55dbf7e21d7b45362d277505a8a7c02537667c3c4c4afbeeded9c40ec2218` |
| scene retry/reload wrapper | `0x221AD620` | 80 | `94ca224968818a97ef138555040b7c14075dd21dc1f31b273675e2ba32974cf9` |
| model-sounds retry/reload wrapper | `0x22149B00` | 80 | `1f2ea46fd39f2c9e8708e1e58a8a72ac5c9aed0c447f43cef04b7e6750e328c1` |

**Four distinct original function entrypoints, 1,312 complete original code bytes**. The two 576-byte initializers are **identical after zeroing only the seven `CALL rel32` displacement DWORDs at local offsets `0x106`, `0x13F`, `0x15F`, `0x172`, `0x17E`, `0x194`, `0x1C0`**. Their normalized whole-region SHA256 is `935ba6392cdec614fcc9ef5ca1745a48977d7f0b9e16356901188c8e5fe9f484`. One important callback differs: original scene `0x221ACFD0` vs model-sounds `0x22149770` at local call offset `0x17E`. The other six direct targets are identical in function purpose/address after adjusting displacements.

The two 80-byte reload wrappers are likewise identical after zeroing only the five direct call displacements at local offsets `0x13`, `0x1B`, `0x30`, `0x3B`, `0x43`; normalized SHA256 `3be911fa12f24228389cb3711dc8aba024c133b21b79716088074128aebc9ac3`. Each reload wrapper has two `JMP rel32` tail edges into its own initializer at offsets `+0x23` and `+0x4B`.

## Exact PE-initialized repository configurations

| Object | Original native object VA | Original `+0x34` path pointer | `+0x38` expected repository version |
|---|---|---|---:|
| Scene data cache | `0x22503EE0` | `0x2242ED68` -> `scene.cache` | **3** |
| Model-sounds cache | `0x224F0388` | `0x2240BC9C` -> `modelsounds.cache` | **2** |

Both original methods consult global filesystem interface storage `0x225721A4`, and invoke a vtable method at byte offset `+8` to open each respective file with original literals `rb` at `0x223F0B4C` and `MOD` at `0x223F0C20`. Other observed filesystem virtual byte offsets include `+0` for reading four-byte header fields, `+0xC` to close, and `+0x44` for invalidation after detecting an unacceptable cache header. **These names are semantic descriptions of call sites, not a proof of the entire original IFileSystem C++ ABI.**

### Confirmed initialization decisions

1. Original object byte `+0x3D` is an `initialized` flag. If already set, return true immediately. Otherwise **set it to 1 before checking the configured filename**. A missing/empty filename logs `CUtlCachedFileData:  Can't Init, no repository file specified.` and returns **false**, without clearing the flag. A subsequent direct call would therefore return true by the fast path. This behavior is an original quirk, not an endorsement of it.
2. If the path is nonempty, invoke the optional function pointer at `+0x40` if present, otherwise use `0`. Save the opaque result in object DWORD `+0x44`. The storage is used as a metadata check; no file-hashing implementation was recovered here.
3. Open the file with `rb`/`MOD`. If open returns null, set object byte `+0x3C` and return **true**, treating a missing cache as requiring regeneration, not a mandatory startup failure.
4. A successfully opened cache must contain the **four-byte cache-system version `2`**, followed by the configured per-repository version (**3** for scenes, **2** for model sounds), then the metadata checksum. A nonzero newly calculated metadata checksum is checked for equality against the saved checksum. If new metadata is zero, the saved value is not considered fatal.
5. If these header fields are accepted, read a fourth signed 32-bit record count. Only a **positive** count enters the native record-processing loop. The loop calls many still-unported helpers (`0x2228DBC0`, `0x2228D7D0`, `0x2228D6A0`, `0x2228D810`, one repository-specific callback, `0x2213ACB0`, `0x2207EC10`) and a record object's virtual method. Do **not** claim to know the serialized record format or meaning of those vtable slots from this pass.
6. If the cache-system version, repository version, or nonzero metadata checksum mismatches, log one of three original warnings (at `0x2240BA50`, `0x2240BA90`, `0x2240BAC4`), then close the file, invoke filesystem virtual `+0x44` with path and `MOD`, set invalid/missing flag `+0x3C`, and return **true**. A valid file also closes before returning true. Native does not validate short reads from the filesystem method in the captured routine.

### Confirmed reload/retry decisions

The native 80-byte reload routine checks the initialized flag `+0x3D`. If already initialized, it clears that flag and branches on `+0x3C`:

- If the cache-invalid/missing flag is set, invoke a repository-specific cleanup callback, then a per-repository data reset callback, and tail-jump to initialize.
- Otherwise, if byte `+0x4E` is set, call a repository-specific condition checker and, when it returns false, invoke a repository-specific rebuild callback. In either case clear/reset its data collection before tail-jumping to initialize.
- If it wasn't initialized, it simply tail-jumps to initialize without those preliminary callbacks.

These outgoing branch targets are **not** reconstructed cache format parsers in this phase. No rollback of already-run callbacks is invented. Object bytes are modeled separately from native heap and vector layout.

## New code and tests

- `include/sin_compat/original_server_cache_repositories.hpp`: portable C++17 original address/field/version metadata; high-level observed header and reload-branch ordering. All engine filesystem operations and per-record processing are **synthetic host callbacks**, not engine-linked replacement DLL symbols. Host adds a maximum-record-count guard absent from the native implementation.
- `tests/test_original_server_cache_repositories.cpp`: both distinct filenames/versions, valid headers/positive count, missing file, three precise invalidation reasons, zero-metadata exemption, negative count, object flag quirks, reload callbacks, and host-only limit check. Optimized GCC and Clang with `-Werror` pass.
- `research/tools/audit_sin_original_server_cache_repositories.py`: owner-original dual DLL SHA256 gate, chained prior Phase187 audit, four entire function region hashes, **98 sparse opcode witnesses across two pairs**, **24 direct relative call edges**, **four reload tail jumps**, static object path/version pointers, eight exact strings, and whole-region *normalized twin comparison and SHA256*.
- `research/tests/test_sin_original_server_cache_repositories.py`: **13 original-free Python tests**, including independent corruption checks for all 98 opcode witnesses, all 24 calls, four tail jumps, both singleton filename pointers/version DWORDs and diagnostic strings; synthetic fixture cannot satisfy whole original function hashes.
- Added root CTest and real 32-bit MSVC Win32 synthetic contract CTest. The presence of a test is not proof of passing CI; check hosted results independently.

## Outstanding work

The original per-record decoder and the concrete filesystem, metadata callback, cache-index structures and repository system implementations are still outside this model. The two original initializers themselves are fully **SHA-verified**, but this is **partial behavioral reconstruction**, not a full semantic translation of their 576-byte bodies. The exact Source engine function signatures, global initialization lifetime and real original executable loading of replacement `server.dll` / `client.dll` are still unproven. The project remains Windows PC first, then Android, then Quest 3.