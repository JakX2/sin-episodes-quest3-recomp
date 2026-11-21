# Gameplay Phase583: original NPC numeric request mapping and action-route admission

**Scope:** native 2006 SiN Episodes: Emergence original `server.dll`, following original Grunt animation event `0x8D` / `0x91` into the shared numeric request mapper and its action-transition caller. The aim is recovering exact original gamecode, **not** reimplementing Half-Life 2-equivalent physics, surface properties, model/prop rendering, Windows factory boot, Android or Quest 3.

## Commercial DLL identity, complete method boundaries and original hashes

The source auditor requires the untouched original pair SHA256:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- PE32 preferred image base `0x22000000`.

| Full original x86 function VA | Authenticated bytes | Whole-body original SHA256 |
|---|---:|---|
| `0x22049A60` | **229 bytes**, ending `0x22049B44` | `4ff325f5f5722b910e681e00b6b0466894baad8a1f92df3228a63b53b0e2f893` |
| `0x22049B50` | **202 bytes**, ending `0x22049C19` | `3dada046704eacb555a0aceff7ea9b3c067a0790835d83b00b90bad4059fb302` |
| **Total** | **431 distinct original bytes**, two newly authenticated complete functions | Both followed by original `INT3` padding |

Native method extents do not overlap previous Phase580–582 methods. The earlier original signal setter `0x22049C20` directly calls `0x22049A60` from `0x22049CCB`. This incoming edge is independently authenticated, but the earlier method is **not** added to these byte counts.

## Original mapper `0x22049A60`, actual out-pointer order

Its original x86 calling convention is a `thiscall`-like owner in `ECX` plus **four stack argument words** (native `ret 0x10`). These are a requested raw numeric action code followed by three **caller-supplied output/context pointers**. The original `0x22049C20` supplies:

- requested value = current NPC raw `+0x880`
- 1st pointer = owner raw `+0x884`, the **resulting numeric action/schedule identifier**
- 2nd pointer = owner raw `+0x888`, the **translated intermediate identifier**
- 3rd pointer = owner raw `+0x88C`, an original input/context word passed into `0x22049870`

The whole original function performs these concrete writes and calls:

1. Initializes `*output_0x884 = -1`.
2. Calls `0x22049870(requested_numeric_code, context_0x88C)`. Writes the returned value to **`*output_0x888`**.
3. If requested numeric code is **`0x0F`**, calls `0x2204FB20(owner)` and stores that result to `*output_0x884`, then returns through the common epilogue. Thus the native `0x0F` special case bypasses the ordinary mapping lookup.
4. Otherwise calls original `0x220EEBD0(translated_identifier)` and stores result in `*output_0x884`. On result != `-1`, it returns.
5. On result == `-1`, it evaluates the original global logging throttle state at `0x22548124`, `0x22548120`, `0x2254811C`, performs an x87 `fcomp`/`test ah,0x41` check against a native constant, and may log the missing mapping. The symbolic meaning of these global values is **not proven**. The standalone model exposes an abstract diagnostic hook, not an exact imitation of this optional native log.
6. If `*output_0x888 == 10`, it **rewrites the mapped identifier to `6`**, calls `0x220EEBD0(6)` and stores the result. If the final output identifier remains `-1`, it writes **zero** to `*output_0x884`.

This behavior is source-authenticated; it should not be labeled with Half-Life 2 schedule names without the original SiN registration table. The numeric raw values `0x0F`, `10`, `6`, `0` are recovered literals, not confirmed source enums.

The method has five exact original `E8 rel32` outgoing targets:

| Original callsite | Callee |
|---|---|
| `0x22049A7A` | `0x22049870` |
| `0x22049A8D` | `0x2204FB20` |
| `0x22049A98` | `0x220EEBD0` |
| `0x22049AE9` | `0x220E6CE0` |
| `0x22049B2C` | `0x220EEBD0` |

## Original route transition helper `0x22049B50`

This method receives the next raw requested numeric code and accepts a one-word stack argument (original `ret 4`). Its own source-proven control flow:

1. Calls `0x220EB8A0(owner)`. When false, it does not request a transition.
2. Compares existing owner raw **`+0x878`** with the incoming requested code. Equal codes do not trigger another request.
3. A special rejection happens when the new code is nonzero, the original `+0x878` equals **2**, and owner `+0x880` is **not** `0x9A`. This stops the action attempt. Raw request zero explicitly bypasses this special rejection.
4. Optionally emits an original diagnostic based on global logging state and owner byte `+0xA9 & 0x10`, still opaque in the host model.
5. Sets owner raw **`+0x880`** to the new numeric request.
6. Calls **`0x22049A60`** with pointers to original owner `+0x884`, `+0x888`, `+0x88C`.
7. Calls original **`0x220417D0`**, presenting the request plus the three resulting raw values in order. The downstream scheduler/action transition behavior itself remains opaque.

This is evidence of a distinct **requested code** (`+0x880`), **current code** (`+0x878`), **mapped code** (`+0x888`) and final **identifier** (`+0x884`) in original native memory. It does not by itself prove their original developer variable names or corresponding named schedule constants.

Original `0x22049B50` E8 calls:

| Original callsite | Native target |
|---|---|
| `0x22049B53` | `0x220EB8A0` |
| `0x22049BB2` and `0x22049BBC` | `0x22022A90` (two separate debug-related callsites) |
| `0x22049BF6` | `0x22049A60` |
| `0x22049C0E` | `0x220417D0` |

These two functions are real dependencies of the Grunt event numeric-signal helper from Phase582, which itself is called by the Grunt-specific animation switch from Phase581 for event `0x8D` with literal signal `0x19`, and event `0x91` with literal `0x1E`. This authenticates a **call chain from Grunt event ID to generic NPC numeric request mapping** without inventing action names.

## Authentication, C++ reference and next proof boundary

`research/tools/audit_sin_original_grunt_signal_route_phase583.py`:

- Authenticates both owner-private DLLs against full-file SHA256 before any PE reads: **PASS**.
- Authenticates both complete function body digests and exact authentic `INT3` boundaries: **PASS**.
- Checks **10 new original E8 direct calls**, **one earlier authentic Phase582 incoming call** and **30 exact original opcode/field witnesses**: **PASS**.
- Synthetic self-test mutates each byte of every authenticated five-byte relative-call encoding and refuses non-owner/fake source binaries: **PASS**.

`include/sin_compat/original_grunt_signal_route_phase583.hpp` is an independently authored C++17 host source reference; `tests/test_original_grunt_signal_route_phase583.cpp` verifies mapping, unresolved and `10 -> 6` fallback, request `0x0F`, unavailable routing, repeated/current action skip, special current code `2`/prior request `0x9A`, zero-request bypass, and original transition callback ordering. Strict GCC and Clang warnings-as-errors, GCC ASan/UBSan, standalone CMake/CTest **2/2 PASS**.

The host source makes native downstream APIs (`0x22049870`, `0x220EEBD0`, `0x220EB8A0`, `0x2204FB20`, `0x220417D0`) **opaque callbacks**, and its diagnostic callback intentionally does *not* claim native logging frequency or x87 timing parity. It does not recreate engine schedules, original source declarations, physics, renderer, game DLL loading, or a playable Android/Quest 3 port.

Reproduce:

```bash
python3 research/tools/audit_sin_original_grunt_signal_route_phase583.py --self-test
python3 research/tools/audit_sin_original_grunt_signal_route_phase583.py --bin-zip /private/SE1/bin.zip
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Best next target is the SiN-original code registration connecting the raw 0x0F / 0x19 / 0x1E / 0x9A numeric identifiers to the actual NPC action definitions, or the branch-specific Grunt event 0x8F XYZ update. Do not map them to HL2 names without original evidence.