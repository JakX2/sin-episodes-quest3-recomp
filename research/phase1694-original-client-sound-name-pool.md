# Phase 1694: recover original client audio name / CGameStringPool ownership

## Scope and evidence source

This work uses the user's exact **unmodified** original 2006 PE32 `SE1/bin.zip` from the linked SiN Drive folder. Private read-only verification authenticates full original client SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` and companion server SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

Unlike Phase 1691's 21 engine interface queries or Phase 1692's owned `CGameStringPool`, the target here is a **genuine original game client caller of that pool**. The previously documented VGUI/HUD helper `0x241EDFB0` is **not** counted again: it was already recovered in original Phase 540/541, and its duplicate PR was explicitly closed.

The new source audit is `research/tools/audit_sin_original_client_sound_name_pool.py`. It authenticates three **complete, distinct original methods** (577 bytes total) including their exact next-byte INT3 boundaries:

| Original client VA | Length | Whole-method SHA256 | Native behavior |
| --- | ---: | --- | --- |
| `0x241BF880` | 341 | `020c1810c4b24ab08688d946bf5f784fcd467938aa66c9bf8928d77b9abc1cf5` | Original audio-name setup; extension-token routing, interned owner fields and subsequent parameter initialization |
| `0x241BCC50` | 41 | `2cc1aeb6d3d13d2e5f6f437f360ed3a958d665c3ffb0fae99cb7b7f517c142fc` | Original sound-emitter global virtual calls, first +0x78 then +0x1C |
| `0x241BF5F0` | 195 | `ee6abaccb339549ba4ef800f200a39c408d80adc8ad9b1ef54a882a8fed80b1a` | Initialize per-sound parameter flags, query provider slots, enumerate and copy indexed records |

Their native direct CALL edges, branch destinations, interface-global values and object offsets were inspected against the owner's exact original bytes. This is newly inspected original code, **not** a reconstructed DLL successfully executing that code.

## Filename routing is substring matching, not extension parsing

The original initializer at `0x241BF880` loads its `Q_stristr` import via IAT `0x24276238` (original decorated import `?Q_stristr@@YAPBDPBD0@Z`). It calls that imported function with two exact original C strings:

1. `.wav` at original client `0x242A6BD4`, checked at native call `0x241BF901`; matching result jumps to original literal/fallback branch `0x241BF949`.
2. `.mp3` at original client `0x242A6BCC`, checked at native call `0x241BF910`; matching result takes the **same** literal/fallback branch.

**Only if neither substring is present** does the original call `0x241BCC50` at `0x241BF920`. That helper obtains original global `0x2433F6E4` (the `VSoundEmitter001` object already recovered in Phase 1691), calls its native vtable at byte offset **+0x78**, then reloads the global and calls byte offset **+0x1C** with the first return and source arguments. A zero byte return takes the fallback path at `0x241BF92A -> 0x241BF949`; a nonzero result takes the distinct source-resolved metadata path.

`Q_stristr` is a *case-insensitive substring* test. A filename such as `ambient.WAV.backup` enters the direct-name branch, even though `.WAV` is not its final suffix. It is not safe to silently substitute host `std::filesystem::path::extension` logic. The portable reconstruction uses deterministic ASCII case folding; non-default original CRT locale details are **not** claimed.

## Original owned string pointer writes

The parent contains three native direct calls to `0x2413D250` (the Phase 1692 original CGameStringPool intern/allocate wrapper):

| Callsite | Original destination | Path |
| --- | --- | --- |
| `0x241BF934` | `this+0x30` | Nonzero sound-emitter name resolution |
| `0x241BF94A` | `this+0x30` | File-token match or resolver refusal |
| `0x241BF967` | `this+0x2C` | Auxiliary C-string, both paths |

The original writes the **returned pooled pointer**, not the original caller-owned C-string address. In the nonzero resolver case, it also sets fields `this+0x44` and `this+0x20` using resolved scratch data. In the literal/failure case `this+0x44` receives float **1.0** and `this+0x20` uses an original caller-supplied argument. These are native layout observations, not proof of higher-level parameter type names. Earlier source `0x2413D250` returns the existing owned string if a casefold-equivalent key exists. Pointers remain valid while the original pool retains ownership and become invalid after original slot5 clear.

The initializer also stores a source vtable-derived value at `this+0x34`, or **-1** when its optional input pointer is null; it stores a further incoming argument at `this+0x38`. Several inline float tests and field resets follow. The function finally calls `0x241BF5F0` from `0x241BF9C5` with `this+0x48` and an argument-provided record object, and exits with original PE32 `ret 0x14`. This is a 20-byte stack cleanup ABI witness, not a fully recovered C++ parameter signature.

## The separately recovered 195-byte parameter helper

Original `0x241BF5F0` sets its own field +4 to integer 1, queries its supplied provider's virtual byte offsets **+4** and **+8**, and ORs the flags with **2** or **4** when the respective Boolean returns nonzero. Its provider's virtual byte offsets **+0x0C** and **+0x10** supply a count and an indexed value, respectively. It validates negative result values and appends entries to an owned array, growing it via original `0x2401B4C0` and shifting with `0x24248B50`. The higher-level identity of the provider, any allocation failure behavior, and the full record type are **unresolved**. The original source includes no explicit general-purpose stable-pointer guarantee for this separate record array.

## Portable subset implementation

`include/sin_compat/original_client_sound_name.hpp` implements only the evidenced routing and **owned string-name outcome** against the *same* Phase 1692 C++17 `Pool` implementation, not against an invented engine or sound vtable. An injected resolver may be called only if neither literal token is found. Both resolver outcomes still intern the filename; optional auxiliary C-string uses the same pool. Invalid or embedded-NUL names refuse conservatively. It never writes original native object fields, calls the original `VSoundEmitter001`, presumes float defaults on Android, or claims original memory-layout parity.

`tests/test_original_client_sound_name.cpp` checks matching `.wav` / `.mp3` anywhere in a filename, uppercase variants, bypass vs invocation of resolver, fallback on refusal, owned first-spelling and pointer stability across pool inserts, auxiliary pool identity, embedded-NUL refusal, and lifetime invalidation after `Clear`.

The identical source regression is registered for native MSVC Win32 x86, C++ reference/Linux, Android ARM64 cross-compilation, and Android Bionic x86_64 emulator execution. The original-source auditor `--self-test` uses invented CALL and short-branch mutations and refuses wrong original DLL SHAs. For independent owner-private verification, run:

```sh
python research/tools/audit_sin_original_client_sound_name_pool.py --bin-zip /private/SE1/bin.zip
```

The archive is never bundled into public GitHub Actions. Its hashes and extracted original bytes are not committed.

## Limitations and next boundaries

This is audio-related, SiN-specific client gameplay behavior, **not** an HL2-equivalent physics, model/prop, surface-properties or renderer task. The next steps are identifying original direct callers of the 341-byte initializer, typed semantics of the input parameter provider to `0x241BF5F0`, and the actual engine-facing emitter interface semantics for slots +0x78 and +0x1C. A source-order sound-name model is not a complete CSoundPatch class, successfully initialized independent `client.dll`, actual audio playback, first rendered frame, APK or Quest 3 execution.

