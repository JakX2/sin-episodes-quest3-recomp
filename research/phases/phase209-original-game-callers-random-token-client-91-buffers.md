# Phase 209: original game-code callers of recovered allocator wrappers

13 November 2026. **Draft stacked on Phase208 PR #262.** The user-supplied original SiN Episodes: Emergence 2006 Windows PE32 `server.dll` and `client.dll` were inspected privately, with whole-file SHA256s matching:

- Server (6,049,792 bytes, original base `0x22000000`): `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
- Client (3,592,192 bytes, original base `0x24000000`): `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

No commercial DLLs, extracted game data, raw native function bodies or complete disassemblies are committed. This phase moves **from allocator-wrapper discovery into two actual callers** in the original game code. The x86 original function SHA digests and outgoing edges were verified against owner-local binaries and the function inventory checked for duplicates.

## Two NEW complete SHA-verified original functions

| Original module | Native entry | Exact x86 length | Complete original-body SHA256 |
| --- | --- | ---: | --- |
| Server object callback, 3-byte random token | `0x22358870` | 52 | `b44c5112406cb01608fcd932d088a9106822257a12c2dcb67cab11968275cd8d` |
| Client first-use 91-buffer initializer | `0x240C8720` | 156 | `5af411946aa3f82304cf82bfcd2bae6b509549b6cd9a5b2347d995396da75787` |

**208 newly SHA-verified complete original x86 bytes, two unique entrypoints**. Both function ends have `ret` followed by original `int3` padding (12 server, 4 client). The research catalog **231 → 233 (128 server, 105 client)**; this is original-method research inventory, not a measure of game completion.

## Original server object callback: exact dataflow

Entry `0x22358870`, 52 bytes. `ECX` holds a game object pointer, saved to `EDI`. The original function:

1. Pushes the byte count **3**, then directly calls the *previously recovered* original allocator alias at **`0x2226E820`**. Exact native callsite **`0x22358876`**.
2. Stores the resulting opaque pointer in `ESI` and writes ASCII underscore `'_' = 0x5F` to its byte zero.
3. Pushes `3`, then `0`, and invokes the import **`vstdlib.dll!RandomInt`**, read from the **original server IAT slot `0x223D21AC`**. It adds ASCII `'A'=0x41` to the **low AL byte** of the result.
4. Writes that result as byte one and stores a NUL at byte two. Normal normal `RandomInt(0,3)` values produce exactly the four token strings **`_A`, `_B`, `_C`, `_D`**.
5. Passes the allocated token pointer to the original object's **vtable method byte offset `+0x5B0`** and returns. The original caller performs **no deallocation** before returning. Whether the callee takes ownership remains unknown.

The function entry VA is referenced by **seven distinct pointer DWORDs in the original server `.rdata` raw bytes**: `0x2248CACC`, `0x2248EDF4`, `0x22495A5C`, `0x22499C04`, `0x2249B05C`, `0x224A4724`, `0x224A5004`. These are exact byte references consistent with method tables; seven different class identities are **not** claimed. No direct `CALL rel32` to this method was observed in the original complete `.text` disassembly; indirect/vtable dispatch is likely but not yet executed in the retail engine.

## Original client first-use buffer initializer: exact dataflow

Entry **`0x240C8720`**, 156 bytes. Exact original direct incoming `CALL rel32` at **`0x240CAF5A`**. `ECX` is the object pointer, copied to `EBX`.

- The method first checks pointer at object byte offset **`+0x454`**. If nonzero, it returns without allocating anything.
- Otherwise it invokes the object's virtual method **byte offset `+0x40`**, then checks the returned descriptor. If the descriptor exists and the descriptor byte at **`+0x11` equals zero**, it calls original helper **`0x240C57B0`** using the object pointer and descriptor as its argument. This helper's full semantics/class identity are not recovered here.
- It invokes the `+0x40` virtual getter **a SECOND time**, then reads the descriptor's **signed 32-bit field at byte `+0x14`**, clamping any value at most 4 to **4 bytes minimum**.
- It directly calls original allocation alias **`0x2416FD80`**, stores the returned pointer at object **`+0x454`**, then zero-fills the allocated buffer with `REP STOSD` and trailing `REP STOSB`.
- It then loops **0x5A = 90** times, allocating and zeroing another buffer of identical size per iteration, storing those pointer DWORDs sequentially at object offsets **`+0x2EC` through `+0x450`**. The first allocation callsite is `0x240C875F`, and the loop allocation callsite `0x240C8791`; both target `0x2416FD80`. The two virtual descriptor queries are separate operations, not one cached pointer.

Total **91 dynamically allocated, individually zero-filled buffers** on initial use, guarded by the first pointer. Original native code **does not check** the SECOND descriptor getter for null or allocator results before dereferencing/clearing. The host-neutral model rejects missing descriptors, failed allocations, or unreasonably large buffers to avoid invalid memory access. These host-only guards are NOT claimed to reproduce the original's fault behavior.

## Independently authored implementation and tests

- `include/sin_compat/original_allocator_game_callers.hpp` models both callers with opaque 32-bit host-controlled tokens, while reusing **Phase208 allocation alias B**, the previously verified per-DLL primary lazy cache and original virtual method byte offsets. The client data structure is a **364-byte contiguous slice of the original object** holding 90 pointer DWORDs and the sentinel; it is not claimed to describe the complete object.
- `tests/test_original_allocator_game_callers.cpp` tests all four server token outcomes, import call ordering, NUL termination, no premature free, tagged/untagged allocator context, null-allocation refusal, 91 distinct client buffers, preinit and no-preinit paths, two independent descriptor queries, signed-size minimum of 4, first-use guard/no repeat, buffer zeroing and pointer publication **BEFORE** zero-fill.
- `research/tools/audit_sin_original_allocator_game_callers.py` validates both complete DLL SHA256s, complete original native function hashes, **41 sparse original x86 instruction witnesses, five exact native direct CALL rel32 edges, the original named `RandomInt` import and two imported `tier0.dll!g_pMemAlloc` bindings**, original `.text` and method alignment boundaries, and **all seven exact original `.rdata` pointer references**.
- `research/tests/test_sin_original_allocator_game_callers.py` contains 11 synthetic-only negative-test groups: corrupt every witness, call destination, method boundary, import binding and method-pointer reference; wrong original image base or section, fake whole-body and DLL hashes, and extra/unlisted `.rdata` references.
- Root Linux/portable CTest and real MSVC Windows x86 CTest registered. **Hosted CI needs to be checked on the exact branch SHA before claiming the branch validated.**

## What remains unknown

Neither original object's source-level class name nor the server's virtual `+0x5B0` method semantics, nor the client object's virtual `+0x40` method / `0x240C57B0` callback semantics are proven. Native allocator object ownership still requires tracing the original engine's actual process behavior. These are game-code reconstruction models, **not** working gameplay or replacements for original game DLLs.

**Next:** inspect the exact caller at client `0x240CAF5A` and the seven server `.rdata` tables, associate them with concrete object/vtable identities when evidence permits, and prioritize the original Windows executable's DLLInit and factory/loader handshake. Windows PC first, Android then Quest 3.
