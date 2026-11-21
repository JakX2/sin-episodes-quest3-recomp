# Gameplay Phase584: original action-token resolver, lookup and transition ordering

## Scope and source identity

**Directly inspected:** the owner's untouched 2006 SiN Episodes: Emergence `server.dll` and `client.dll` from original `bin.zip`, not Half-Life 2 reference implementation or an engine-generic replacement. Separate work on Windows loaders, rendering, surfaces, props, models, Android and VR remains out of scope. Owner SHA pins:

- `server.dll` `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll` `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Original PE32 preferred image base `0x22000000`.

This phase follows the three opaque Phase583 edges into **three complete original x86 methods**, without identifying unsupported Source SDK schedule names.

| Original VA | Native bytes / inclusive end | Exact SHA256 whole method | Original boundary |
|---|---|---|---|
| `0x22049870` | 483 through `0x22049A52` | `1fd33bf43c063f042614cebee270dc29990aea8cda358ad39e461b72fbe1d315` | next byte `INT3` |
| `0x220EEBD0` | 90 through `0x220EEC29` | `313f85010f1a33727341d3896097c366180f6d1340c81f91baf78989ba7ab0c5` | next byte `INT3` |
| `0x220417D0` | 359 through `0x22041936` | `aa4f92800ea9cb148416f4cb5f135ce1d4acd342b216f178d4f324c67fff5487` | next byte `INT3` |
| **Total** | **932 distinct original bytes**, three SHA-pinned complete method extents | | |

## Original `0x22049870`: request normalization, not a single table lookup

`ECX` is the owner, with exactly two stack parameters (`ret 8`): requested numeric code and an optional pointer receiving the initial result of the owner virtual `+0x33C`. Original code initializes two temporary flag bytes to zero. The first call `virtual+0x33C(request, &flag)` produces an initial transformed code. It then loops up to **five** times, using `virtual+0x338(current)` followed by `virtual+0x33C(alternative, &flag)`. The flag from the most recent transform influences a diagnostic path; it is not a proof of any particular schedule identifier.

The method has distinct early and fallback routes:

1. If the current loop code equals literal **`0x0F`**, return that code immediately, without original availability/schedule lookup.
2. Otherwise call original `0x220EB8A0` owner availability predicate. If true, call `0x220EEBD0` on the last returned transform. If that lookup succeeds (result `!= -1`), return **the transformed code**, not the mapped action returned by the lookup.
3. On unresolved mapping, optionally diagnose using the original flag and global log throttling. The exact logging time/contents are opaque.
4. Check the most recent alternative against `0x2202E600`, then separately check the initial transformed code and original request. These branches can return **different values**, including returning the original request after a successful test of the *initial transformed* code.
5. Final fallback exchanges numeric code `10` with `6`; otherwise returns the original code.

The original virtual getter signatures/semantics and the exact branch behavior of `0x2202E600` have not been independently reconstructed. This host reference reproduces the grounded call/decision order by accepting explicit callback outputs. It does not claim all native machine registers/exceptional inputs are source-equivalent.

## Original `0x220EEBD0`: state-dependent registration query

The method takes **one stack numeric input** and returns with `ret 4`. It snapshots owner dword **`+0x3EC`**; calls original `0x22139A10` for an opaque entity/context; if nonnull, calls global registry interface at `0x225721BC` vtable `+0x24(entity)`. Only classification **`3`** invokes registry vtable `+0x28(entity)` to obtain a mode. The final callee is **`0x220EB1C0(mode, requested, owner+0x3EC)`**, with mode **zero** otherwise.

There are two real native `E8` edges to `0x220EB1C0`, corresponding to the zero-mode versus class-3-mode branches. The original lookup's dynamic/weighted table semantics are still below this boundary. The source report does **not** assign symbolic class or schedule names to raw values `3`, `-1`, `0`.

## Original `0x220417D0`: apply an action while preserving callback order

This method accepts **four stack words** (`ret 0x10`). On entry it writes its **third argument** to owner **`+0x87C`** *unconditionally*. Its **first argument** is the next raw request, second the selected action index and fourth a callback context. A debug-only path depends on original global `0x2254D090` and owner `+0xA9 & 0x10`; the C++ host does not attempt to emulate its string formatting, logging or memory side effects.

For second argument **`> -1`**:

- If the requested action differs from owner selected state at `+0x3EC`, or that selected state is not active (`+0x3E1` zero), the method queries **owner virtual `+0x4E0` twice**, once with the old request `+0x878` and then with the new request. **If both do not approve**, it calls original `0x220405C0(0)` before scheduling. A short-circuit `&&` preserves the native call count.
- It calls `0x220F2300(action)`, `0x220EEDE0(action)` returning an x87 floating value, and then `0x222A5470(context, returned float)`; original execution is not replaced by a guessed animation/physics controller.

For a negative action index, it instead calls **`0x220F2300(0)`** and omits the other two action callbacks.

Both paths then:

1. Call owner virtual `+0x630` with the stored **`+0x87C`** value and an output scratch pointer, pass its result to `0x22140BD0`.
2. Compare prior `+0x878` with the new **first** argument; if different, invoke owner virtual `+0x4E4(new request)`.
3. Write new first argument to **`+0x878`**, *after* the notification virtual.
4. Call the navigation/helper pointer at owner `+0x908`, virtual **`+0x4C`**, even if no requested-code change occurred.

The host model tests this exact ordering and the distinct positive/negative selected-index paths, with a typed `TransitionState`. Native pointer identity and x87 ABI are intentionally abstracted.

## Source proof and testing

- SHA pins cover **three entire original function extents, 932 native bytes**; all are in original `.text` with verified `INT3` next-byte boundaries.
- **25 exact original E8 direct CALL/target pairs**, **three incoming Phase583 direct callsites**, **42 non-overlap asserted opcode/field witnesses**, verified original virtual-table byte offsets and numeric literals.
- Read-only Python `--self-test` rejects each byte mutation of all 28 checked five-byte CALL encodings and a fake DLL identity; `--bin-zip` refuses mismatched originals.
- Independent C++17 host code in `include/sin_compat/original_grunt_action_translation_phase584.hpp`; test in `tests/test_original_grunt_action_translation_phase584.cpp` covers five-step cap, shortcut `0x0F`, `10/6` swap, context lookup classification, mapped callbacks, short-circuit virtual admission, state write order and action absence. GCC/Clang `-Werror`, GCC ASan/UBSan, standalone CMake/CTest **2/2 PASS**.
- Reproduction: `python3 research/tools/audit_sin_original_action_translation_phase584.py --self-test --bin-zip /private/bin.zip`; `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`.

**Known limits:** no original dynamic schedule registration names, exact log throttle, native virtual contracts, full `0x220EB1C0` lookup, x87 exceptional precision, valid game DLL factory, completed gameplay source, or Android/Quest support are claimed. No commercial binary, direct disassembly dump, map, proprietary material or patch is published.