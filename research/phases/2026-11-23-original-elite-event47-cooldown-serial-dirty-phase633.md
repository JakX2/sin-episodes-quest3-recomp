# Gameplay Phase633: Original Elite event47 clock, serial and dirty-state helpers

## Scope: three new COMPLETE original-source methods, 131 executable bytes

The original unmodified 2006 *SiN Episodes: Emergence* owner-supplied server.dll has SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Original client.dll SHA256 is `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Phase632 recovered the **complete secondary Elite animation-event handler** `0x223556A0`, and proved source callsites for its event `0x47` branch. That reference necessarily treated original helpers as opaque host callbacks. Phase633 reverse-engineers three of those source methods in their **entirety** and composes them with the existing Phase632 event model. These are not renamed generic HL2 callbacks or inferred Source game class methods.

| Original method | Complete original bytes | Original SHA256 | Direct Phase632 native callsite(s) |
| --- | ---: | --- | --- |
| `0x220326D0` clock vs Elite deadline | 30 | `e855a4c4805f0994d12d66af60bb42b6d9d1bea9f7aa9d7a83aca2b39943e1cc` | `0x223556C5` |
| `0x222AA640` Elite serial-target getter | 46 | `012fdc0caa1ad7c83f1107ab5e442bf2ce9eae7d87c10e11649998b88aafd5fb` | `0x223556D4`, `0x223556E3`, `0x223556FD` |
| `0x2203F8E0` resolved-object DWORD setter | 55 | `355e523bed2535ee30adbe6477d3fdcba0e57d07aa9dcfc1b370028d41b81371` | `0x2235570A` |

**Total new independently recovered 2006 source:** 131 executable bytes, three complete functions, five authenticated Phase632-to-helper direct call edges, 27 original opcode witnesses. All full hashes and opcode/edge witnesses were verified against the owner's original local `server.dll`. The original method bodies are neither published nor linked into CI.

### Original predicate `0x220326D0`: engine clock reaches Elite `+0x9DC`

The method loads the global game-clock pointer **`0x22572190`**, reads float at `+0x0C`, performs x87 `FCOMP` against original Elite float `this+0x9DC`, then examines **C0** in the x87 status word. The predicate returns true if current clock is **greater than or equal to** `+0x9DC`; a lesser or unordered floating comparison returns false. There is no additional arbitrary delay from Phase632. The independently authored model reads both float values through host callbacks and preserves the original individual load sequence, ordinary IEEE f32 comparisons and negative/NaN cases. Exotic x87 control-word/exception behavior remains outside the portable model.

### Original helper `0x222AA640`: *different Elite serial handle* at `+0x78C`

The source first loads the encoded DWORD from **Elite `this+0x78C`**, not the `+0xFDC` serial field used by the separate weapon-linked `0x43` path. If `0xFFFFFFFF`, return null without accessing the global table. Otherwise, it loads the serial table through **`0x224F5620`**, indexes `handle&0xFFF` into 16-byte entries, compares the saved upper serial bits `handle>>12` against the entry serial stored at offset `+0x04`, and returns the stored native pointer at entry offset `+0x00` if matching. A mismatch returns null.

**Crucial consequence:** event `0x47` calls this exact helper up to **three separate times**, meaning each call rereads Elite `+0x78C`, separately accesses the global table, and can resolve a different object. The model does not cache the first or second result for later uses. A valid serial entry may still contain a null pointer; the Phase632 reference explicitly refuses the possible second/third object null dereferences.

### Original helper `0x2203F8E0`: target DWORD `+0x4EC` and two dirty paths

The Phase632 secondary event calls this method with native **`ECX = thirdResolvedObject + 0x4EC`** and a **pointer to a DWORD** returned by the second resolved object's original dynamic vtable `+0x404`. The helper reads the existing DWORD; if it is unchanged, it immediately returns with **zero dirty-state effects**. If different:

- Read byte `thirdTarget+0x1F0`. If nonzero, write byte `thirdTarget+0x200=1`, then write the new DWORD to `thirdTarget+0x4EC`.
- Otherwise read native owner/edict-dirty pointer at `thirdTarget+0x16C`. If nonzero, OR bit 0 into the pointed DWORD; finally write the new DWORD to `thirdTarget+0x4EC`. If the pointer is null, simply write the DWORD.

These are exact relative addresses from `ECX=target+0x4EC`, not fabricated entity-method names. The model publishes each native write in **source order** through host-controlled callbacks, supports partial side effects on later host failure and performs no invented rollback. This reused Source-style dirty-state mechanism is investigated **only because it is on the proven original SiN Elite event47 path**. No broad HL2 props, rendering, physics, surfaces, or unrelated setter work was added.

## Implemented C++17 source references and composition

- `include/sin_compat/original_elite_event47_helpers_phase633.hpp`: three source-bounded typed host interfaces and references `Cooldown`, `SerialLookup`, `Field4ECSetter`.
- `src/original_elite_event47_helpers_phase633.cpp`: complete original instruction-level semantic decisions as host-neutral C++17 methods, including early returns, shift/mask serial table derivation, two dirty-state alternatives, original callback ordering and reentrancy/exception refusal.
- `tests/test_original_elite_event47_helpers_phase633.cpp`: tests f32 boundaries/NaNs and host exceptions, mismatched/missing/wrong serial targets, the original sentinel, three independently reread serial handles with different target objects, identical vs changed +0x4EC values, both dirty-state paths, null dirty-owner pointer, all write-failure/partial mutation paths, and method reentrancy.
- The same C++ test **composes all three reconstructed helpers directly with the already-compiled Phase632 real event47 reference**, demonstrating a source-backed clock predicate, three distinct original serial rereads, second target virtual-return value, third target +0x4EC dirty field update, then opaque original numbered arguments 3/4/5 and final action 0. This is a compiled model composition, **not** an actual engine test.
- `research/tools/audit_original_elite_event47_helpers_phase633.py`: requires both SHA256-authenticated original PE32 game DLLs before checking three **complete** source hashes, five rel32 original caller→helper edges and 27 opcode witnesses. `--self-test` mutates entirely fabricated bytes/edges and rejects non-retail game identity.
- Root portable Linux/Android-compatible CMake and native Win32 x86 MSVC builds/CTests.

## Remaining authenticated callback work

The same Phase632 original event47 method calls two further original natives which Phase633 deliberately leaves opaque:

1. **`0x220398A0` (91 bytes)**, called three times with arguments `3`, `4`, `5`. Initial disassembly indicates native encoded ID conversion through virtual `+0x760` and `0x22073700`, then a bit **clear** in the original Elite `+0x7CC` bitset. Its exact full behavior and owner interface should be separately recovered, not replaced with a placeholder bitmask.
2. **`0x2203D2D0` (93 bytes)**, invoked with argument `0` on the event47 success path and conditionally on the event43 path. Initial disassembly shows an unconditional `0x2204E420` call, a virtual `+0x760` getter and `0x22073700` lookup with literal `0x21`, a check against Elite flags at `+0x7CC`, and a conditional write `Elite+0x7A4=4`. This state mutation should be its own source-verified method before integration.

These next candidates represent a further potential **184 source executable bytes** but are *not counted as Phase633 implemented methods*.

### Owner validation

```powershell
python research\tools\audit_original_elite_event47_helpers_phase633.py `
  --server "D:\SiN-Original\server.dll" `
  --client "D:\SiN-Original\client.dll"
```

For real-game evidence use the Phase632 owner-only `-WatchProfile Event` hardware debugger, observing the original second event handler's event-ID classification. The real game has **not** been run through our reconstructed references, and these original helper methods have not been dynamically traced in an owner game process. Public CI compiles synthetic host-level models; it does **not** prove gameplay, a replacement `server.dll`, or Android/Quest in-level boot.
