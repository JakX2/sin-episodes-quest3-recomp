# Phase 182: Original 2006 client surface-file read, typed KeyValues and shared physics dispatch

**12 November 2026.** Continues Phase181 draft PR #234. Uses owner-provided original *SiN Episodes: Emergence* Windows PE32 `client.dll` and `server.dll` privately. Public code excludes original game binaries, complete original machine-code functions, game assets and scripts. Goal remains engine-compatible **Windows x86 game DLLs first**, then Android and Quest 3.

## Native evidence and complete function regions

Original client: 3,592,192 bytes, SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`, PE32 base `0x24000000`.

Original server: 6,049,792 bytes, SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`, PE32 base `0x22000000`.

Five original native function regions independently verified by their **entire function-region SHA256**:

| Original VA | Length | Exact original SHA256 | Native function role |
|---|---:|---|---|
| `0x241667C0` | 232 | `ff0eb63ceb387d73abca9cd91928809eac5a902ee04fc3eea2098c22325a5124` | Convert a KeyValues field into a string via 5-way native type switch, or return default |
| `0x24191EC0` | 136 | `f5e1bbb19b7d8305690aa65992653d244ee0e8aaaf105e057fb760a889c218b5` | Open a surface-property data file, read, close, NUL-terminate and hand bytes to the physics surface-property interface |
| `0x24192050` | 32 | `b558e0cbc0c489b26369d3ea1e38d16e5668952877b892e698051b5979a742e8` | Dispatch a method at virtual byte offset +4 on global interface only if both arguments are nonnull |
| `0x24192070` | 32 | `4123fed65cbc55dbc746751ac69c5ed77b6fe95e597eb7f37971373b37dc1f4e` | Same guards but virtual byte offset +8 |
| `0x24192090` | 22 | `00ecc6870ea5a88df35279c6bba7ed2c9b626b3600bec486303451b64ceede86` | Forward two raw input pointers via global-interface virtual byte offset +0xC, without the two null guards |

**454 exact original executable function bytes verified**. The native `0x241667C0` code region ends just before an embedded **separate 20-byte five-DWORD jump table** at `0x241668A8`, SHA256 `dbcd6e68c213b9cc929530b59a5a5104e681584c1a2f8c4c95fcab84702291c4`. The table is explicitly and independently audited; it does **not** inflate the count of original function bytes. The original region SHA digests are **not** reconstructed source-code equivalence proofs.

Phase182 owner-only audit verifies the native function boundaries, **48 sparse non-contiguous x86 instruction witnesses**, **8 original indirect virtual call opcodes**, **5 exact internal CALL rel32 edges**, both incoming direct CALL sites from the previously verified Phase181 manifest loader, the five table destination addresses, four original strings and raw `0x242FD18C` manifest global reference. `audit_sin_original_client_surface_file.py` chains the complete Phase181 original-client/server provenance audit when called using `audit()`.

## Exact KeyValues `GetString` native behavior, 0x241667C0

The function receives a KeyValues `this` in ECX, a string field name and caller-provided default. It calls original `0x241656E0` with `(fieldName, 0)`, which returns an original KeyValues value node or null. Null, type 0, and unsigned types outside the range **1..5** return the caller's default value. Type lives in the native DWORD `node+0x10`.

The five verified jump-table targets, in tag order 1..5, are:

| Type | Original jump target | Observed source action |
|---:|---|---|
| 1 | `0x2416688F` | Return raw DWORD from `node+0x04`, original existing string address |
| 2 | `0x2416682B` | Decimal conversion using `%d` and signed integer from `node+0x0C`, call `0x24165AE0` to install new string, return `node+0x04` |
| 3 | `0x241667F4` | x87 float32 `node+0x0C` formatted with `%f`, same original setter, return `node+0x04` |
| 4 | `0x2416682B` | Same decimal conversion as type 2 |
| 5 | `0x2416685D` | Convert native wide-string pointer `node+0x08` with Win32 WideCharToMultiByte, original **CP_UTF8 (65001)** and **512-byte buffer**; only if conversion returns nonzero, assign string and return `node+0x04`; otherwise return caller default |

Literal native formatting strings `%d` and `%f` and the jump-table entries are independently checked against original client `.rdata` and `.text` respectively. Original numeric-to-string setter `0x24165AE0` is **not** reconstructed. In the C++17 host model it is a callback, as are the original key lookup and UTF-8 conversion.

## Original surface data file read, 0x24191EC0

The Phase181 manifest loop calls this function at `0x24192839` after it obtains the surface file path from a KeyValues entry. Native arguments are (filename, surface props interface, filesystem interface). It does **not** open the manifest again: it opens each **referenced surface file**.

1. Call the supplied filesystem virtual method at **vtable byte offset +8** with `(filename, "rb", 0)`. Native mode literal is `rb` (`0x24289C0C`).
2. On null/zero handle, print `Unable to load surface prop file '%s' (referenced by manifest file '%s')\n`, using the original manifest path pointer global `0x242FD18C`, then return; no physics parse call.
3. Otherwise call filesystem **virtual +0x1C** to obtain file byte count, calculate temporary stack allocation of that size plus padding, and dispatch virtual **+0x00** to read that byte count. Native does **not** test the returned read length.
4. Call filesystem **virtual +0x0C** to close the handle. **Close precedes parsing**.
5. Write `\0` at buffer[size], then call original surface-props interface **virtual +4** with `(filename, buffer)`. Native does not check that callback's result.

The original uses an unchecked stack allocation and assumes its engine pointers and file-size operations are valid. Our reference uses an explicitly bounded heap-backed buffer to avoid stack overflow on invalid synthetic fixtures. This **safety departure is documented, not attributed to the original**.

## Three native shared surface-properties forwarders

The original client global DWORD `0x243743AC` owns the surface-properties vtable used by native wrappers `0x24192050`, `0x24192070`, and `0x24192090`. The first two check each argument for null before using the global interface, and dispatch vtable bytes **+4** and **+8**, respectively. The third wrapper uses method **+0xC** with no argument null checks. Their actual higher-level operation names remain **unknown**, so the reconstruction uses neutral `method4`, `method8`, `method12` callbacks. Do not mistake virtual byte offsets for virtual method *indices*.

## Added code, tests and limitations

- `include/sin_compat/original_client_surface_file.hpp`: exact five-DWORD x86 value record prefix and static addresses; portable tagged-value conversion, explicit setter and filesystem callbacks, bounded read/close/parse order, three virtual-dispatch branches.
- `tests/test_original_client_surface_file.cpp`: missing fields, out-of-range tags, existing string, signed decimal and float formatting, UTF-8 success/failure, missing/opened/empty/oversized files, call/close order, and null-guarded versus unconditional virtual forwarding. GCC and Clang C++17 `-O2 -Wall -Wextra -Wpedantic -Werror` pass locally.
- `research/tools/audit_sin_original_client_surface_file.py`: exact native original-file/checksum validation, all five complete original function hashes, and sparse opcode/call/string/table witnesses. No complete native code bytes committed.
- `research/tests/test_sin_original_client_surface_file.py`: 11 original-free tests, with independently corrupted opcode/vtable/CALL/string/jump-table witnesses, synthetic hash rejection and invalid PE base rejection.
- Root CMake/CTest and actual Windows Win32 MSVC CI test integration in `pc/windows_x86_proxy/CMakeLists.txt`.

**Not completed:** original KeyValues `0x241656E0` lookup, `0x24165AE0` setter, Win32 filesystem virtual ABI, actual surface-property parser, physical game initialization, game executable with replacement DLLs, Android game or Quest 3. The manifest file itself and game assets remain private; this work models the *binary program* reading those assets, not their contents.

Next useful targets: original KeyValues lookup and string setter, actual physics surface props callback object/vtable, and server startup sequence. Prioritize a real original-Windows-game-hosted test over indefinitely collecting isolated helpers.