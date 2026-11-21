# Phase 549: Original `VClient011` level lifecycle, client shutdown and prediction command

## Provenance and scope

These findings come from the owner's **unchanged original 2006 SiN Episodes: Emergence** PE32 `client.dll` and `server.dll`. Their whole-file SHA-256 identifiers are respectively:

- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

Phase538 authenticated original `VClient011::Init` at virtual **+0x00**, native VA `0x240FE0E0`, from the original vtable at `0x24296C6C`. Phase549 recovers **the next four original vtable entries**, clarifying shutdown and map-level callback order, plus one directly called original client-owned resource cleanup. These five **distinct complete original methods total 646 native code bytes**, not counting any earlier function.

These descriptive role labels follow their original vtable order, argument patterns and original effects. Exact Ritual C++ member names, all helper ownership semantics and the original static ABI declaration are **not recovered**. Particularly, map pre/post and shutdown labels are vtable-lifecycle **interpretations**, not source-line symbols. This report is a **source-level decompilation of the verified observable control flow** and does not imply a playable original game module.

## Original vtable entries and complete-method SHA-256

| Vtable byte offset | Complete original client method | Length | Whole-method SHA-256 | Recovered role |
|---|---|---:|---|---|
| `+0x04` | `0x240FF3D0` | 204 | `4cab24e5b8920392fdcf69020e709da740e06f2963f01ca2b374a6c3755f92d1` | Whole-client shutdown and reverse client-owned callback sequence |
| `+0x08` | `0x240FDB40` | 192 | `28a4824a1a03c8b393e0551448dc062856eeff486cd11391de282e4f09a55511` | Level pre-entity startup, original map state and `cl_predict` command |
| `+0x0C` | `0x240FDC00` | 21 | `dce3dcce7372b6a641fe7c1bb20a17c1982b3c922ec798c6bcad85b3b4fd34b2` | Level post-entity tail-forwarded initialization |
| `+0x10` | `0x240FF4A0` | 202 | `e33da70aa7dd66188da6a46dbca45f6cb80ec06afeaea5005a88d714ee43672a` | Guarded level shutdown and reverse client-owned callback sequence |
| N/A, direct call from +0x04 | `0x240FC900` | 27 | `71721cdd2db6d9210f6b335faca52209973d792a0aa0b802c0c7306241303e88` | Destroy an optional client global via native vptr+0 and clear its pointer |

Important binary boundary: the 192-byte pre-entity method ends exactly at `0x240FDC00`, where the next 21-byte post-entity method starts. **There is no `INT3` padding between them**. The audit checks the exact vtable successor rather than claiming a padded boundary. The 204-byte whole-shutdown method contains `0xCC` as one byte of an embedded **rel32 CALL displacement**; a naive first-`0xCC`-byte scanner falsely truncates that method. The full digest and executable vtable pointers are used instead.

## Exact level-active state gate (`0x2433F680`)

Both `+0x08` and `+0x10` read the **same** original byte global `0x2433F680`:

- `0x240FDB40`: if byte is nonzero, jump directly to native `ret 4` at `0x240FDBFD`. Otherwise set byte to **1 at `0x240FDB53`**, then perform the rest of level startup and make the prediction-console decision. This is a single-attempt gate in the visible code, not a transactional or retry-safe initializer.
- `0x240FF4A0`: if byte is zero, jump directly to the ordinary return at `0x240FF568`. Otherwise **clear it to 0 at `0x240FF4B3`**, *before* calling engine/HUD/system cleanup dependencies.

Pseudocode of the **verified guards only**:

```cpp
// +0x08, original 0x240FDB40 (one map-name argument, x86 ret 4)
if (native_global_byte_2433F680 != 0)
    return;
native_global_byte_2433F680 = 1;
// ... original engine and map/HUD startup operations ...

// +0x10, original 0x240FF4A0
if (native_global_byte_2433F680 == 0)
    return;
native_global_byte_2433F680 = 0;
// ... original shutdown operations ...
```

These are **two separate lifecycle operations**. Neither proves that all later callouts always succeed or that nested engine callbacks cannot re-enter them.

## `+0x08`: map-name input, dependency ordering and prediction command

Original method `0x240FDB40` performs the following ordered actions on its first entry:

1. Tests global byte `0x2433F680` and sets it to 1, as above.
2. Reads original global object `0x2431CD18` and calls its virtual `+0x04`.
3. Reads the method's **single 4-byte stack argument** into `esi`, passes it to a different global object's virtual `+0x08` at `0x24320BB4`. This argument is plausibly a map name, but the original **function signature and pointer type remain unproved**.
4. Calls native helper `0x24053AB0`; calls a virtual `+0x0C` on `0x242FA958` and virtual `+0x04` on `0x24377060`; calls another virtual `+0x08` on `0x242F588C`.
5. Passes the input argument to original `0x241566D0` (a replaceable global string buffer helper, verified separately but not newly counted) and calls original `0x24128C50`.
6. Reads original `0x242FA458` and compares **dword at +0x14 against 1**; when greater than 1, calls original global `0x2433F684` virtual byte `+0x148` and tests the Boolean result. This global address was independently identified in Phase538 as the request output for `VEngineClient012`. The numeric meaning of +0x14 is **not verified**.
7. Calls the `VEngineClient012` object virtual `+0x1C` with exactly one of two original console literals, then calls `0x24143240` on object `0x243623D0` with the same original input argument.

**Critical original branch mapping**, independently verified from the string bytes in the original `.rdata` section:

| Condition | Original command literal address | Original string |
|---|---|---|
| `*(int32_t*)(global_242FA458 + 0x14) <= 1` | `0x242968B0` | `cl_predict 0` |
| `> 1` AND `VEngineClient012` virtual `+0x148` returns nonzero | `0x242968B0` | `cl_predict 0` |
| `> 1` AND `VEngineClient012` virtual `+0x148` returns zero | `0x242968C0` | `cl_predict 1` |

The positive-return condition branches to `0x240FDBE1`, which uses **`cl_predict 0`**, not `cl_predict 1`. The zero-return path issues **`cl_predict 1`** at the earlier `0x240FDBC2` branch. This behavior is counterintuitive without the original engine context; do **not** rename native +0x14 to "player count", "split-screen count" or +0x148 to "prediction allowed" without source evidence.

```cpp
// Source-bounded branch description. No original Source class types assumed.
const char* cmd =
    ((int32_t(original_unknown_field_0x14) > 1) &&
     !virtual_engine_0x148_return())
        ? "cl_predict 1"
        : "cl_predict 0";
engineClient->virtual_0x1c(cmd);
```

The native actual code short-circuits and **does not call** virtual `+0x148` when the field is <=1. The above explanatory expression must respect that. Original command byte sequences are verified at rdata `0x242968B0` (`cl_predict 0`) and `0x242968C0` (`cl_predict 1`).

## `+0x0C`: post-entity helper tail call

The **21-byte** native function at `0x240FDC00` is unusually compact:

1. Direct calls to original `0x24156730`, then original `0x24194F90`.
2. Loads object global pointer from `0x2431DBEC`, reads its vtable, then performs a native **indirect `JMP` at virtual byte +0x14** (`0x240FDC12`).

The last operation is a tail jump, **not** a call followed by a local `ret`; the forwarded virtual's return behavior belongs to the callee. This function has no separate function epilogue.

## `+0x10`: level shutdown order and client-owned callback array

When the shared active-state byte is set, the **202-byte** method performs:

1. Clears active flag early; calls `0x240C4570` with a literal zero argument, then `0x24156740` and `0x241941A0`.
2. Calls global object `0x24320BB4` virtual `+0x0C`, and original `0x241061E0` on object `0x242FA738`.
3. Traverses a **global singly linked list** rooted at `0x2433F6F8`. Each node's vtable slot zero is called using that node as `this`, then the next pointer at node `+0x04` is read. The head may be null and terminates the loop. The exact object interface is not established.
4. Calls helper `0x24156750`, global `0x242F588C` virtual `+0x0C`, global `0x24301268` virtual `+0x08`, helper `0x2417F750` on `0x24373E30`, helper `0x241432C0` on `0x243623D0`, global `0x2431DBEC` virtual `+0x14`, and global `0x2431DE3C` virtual `+0x20`.
5. Reads a client-owned callback-array count at **`this+0x10`**. For indices `count-1` down to zero, obtains `this+0x04` array element and invokes each original object's virtual **byte `+0x34`**, then writes **`this+0x10 = 0`**. This is the same reverse-registered-game-system callback pattern seen in the whole-client shutdown method.

Notably, the original code does not free the callback-array allocation here; it clears the **logical count**, not the data pointer at `this+0x04`. **This must not be silently equated to Phase539's separate AutoGameSystem array rooted at global `0x242FBFA8`**. The original CHLClient object's runtime address and the full callback-element types are not established. The exact meaning of virtual +0x34 is unresolved.

## `+0x04`: whole-client shutdown and directly called owned-global destruction

The **204-byte** client vtable +0x04 method at `0x240FF3D0` immediately calls `0x240FC900`, a separately authenticated **27-byte** method that:

- Checks global object pointer at `0x2433F3F4` for null.
- If non-null, invokes the object's native vtable slot **+0x00** with literal argument **1** (a deleting-destructor-style convention is plausible, but the original callee is not decompiled).
- Stores **0 to the global** unconditionally afterward.

The parent then performs three separate getter calls (`0x241DAB30`, `0x2418D1C0`, `0x241ACF50`) and passes each resulting pointer through **the same global object's native virtual `+0x28`** (global at `0x242FF470`) in their original call order. It follows these with helper calls and distinct virtual dispatches on other globals.

Later, it loops this object's callback array from `this+0x10` down to zero, calls virtual `+0x34` on each, and sets the system count to 0. Its final ordered native helper calls are `0x241566C0`, `0x24143E80`, `0x241EE0C0`, `0x240FC100`; it zeros original pointer global `0x2433F6D0` and returns.

**Do not conflate the reverse callback loops with Phase539's AutoGameSystem registry.** These two CHLClient methods both dispatch virtual +0x34 through their own `this+0x04` array; the separate global game-system list uses a different hardcoded address. Source/alias equivalence has **not** been demonstrated. These callbacks are separately reachable, so they may be executed during both full-client and map shutdown based on caller state. Their concrete meaning remains unresolved.

## Original-free C++17 reference and verification

`include/sin_compat/original_client_level_lifecycle.hpp` captures only authenticated branch decisions: the shared active-state byte gate, exact `cl_predict` string choice, and reverse-index CHLClient-owned callback dispatch order with count reset. The reference never tries to call original 2006 vtable addresses or instantiate original client objects. `tests/test_original_client_level_lifecycle.cpp` covers inactive/active map transitions, all three branch conditions for the original prediction command, zero and nonzero system counts and reversed dispatch order.

`research/tools/audit_sin_original_client_level_lifecycle.py --bin-zip /owner/original/bin.zip` checks both original full-DLL SHA identities, **five complete original function SHA256s (646 code bytes), four original vtable pointers, original `CHLClient` RTTI, 26 rel32 CALLs, two native near conditional jumps, 41 opcode witnesses and both literal command strings**. `--self-test` mutates synthetic rel32 instruction bytes and rejects incorrect source hashes. The tool prints no proprietary original binary bytes.

### Next original decompilation dependencies

- The actual owner and element objects of the `CHLClient+0x04` callback array, especially the targets of virtual `+0x34`; do not equate them to the twelve registered AutoGameSystems without proof.
- The engine client `+0x148` virtual result and original global `0x242FA458+0x14` source (not implemented, just documented) to understand the unusual `cl_predict` branch.
- The linked objects under global `0x2433F6F8` and the owner of `0x2433F3F4`.
- Real client server lifecycle integration remains out of scope until game-specific source-level decompilation is more complete. All Phase549 proof is static original PE32 evidence, not retail-engine test confirmation.