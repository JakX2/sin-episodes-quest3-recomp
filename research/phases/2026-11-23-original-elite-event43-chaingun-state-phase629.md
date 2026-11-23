# Phase629: Two complete original 2006 weapon-state methods behind Elite animation event 0x43

## Provenance

The owner's unchanged 2006 retail `server.dll` and `client.dll` are the sole native source. Their full SHA256 values are:

- server: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- client: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

Phase593 authenticated the complete Elite animation handler at `0x22355480` (360 bytes). Its original event `0x43` branch resolves a `this+0xFDC` weapon serial handle and calls **`0x22315320` at `0x22355503`** with that resolved entity as ECX. Phase623 authenticated the complete Elite Spawn and the CWeaponChaingun RTTI cast that populates the serial handle. This is a **source-observed call chain**, not guessed equivalence from a later Source SDK.

The full original bodies of **two additional distinct methods** are now SHA-verified:

| Original function | Start VA | Body bytes | Full original SHA256 |
|---|---|---:|---|
| Event43 gate | `0x22315320` | **82** | `0c0dda93a80bfdf697c92723bb3009406ec52c0e94befe7c1875ba8857d9c665` |
| Paired weapon-state updater | `0x22315390` | **173** | `13129eadd1c0023f308921188d4782da0a7f09debeb9dd98831776526bd56628` |

An additional **16-byte dispatch table** at `0x22315374` has SHA256 `64a10f13fed6cb45ff28cb6757d5d6be0701fd0380f41dbdf2b4801ccbbafe69` and branches for states 0, 1, 2 and 3. This is original table **data**, not a third native method. The gate ends before the table. The updater ends before `int3` padding at `0x2231543D`.

The two functions contribute **255 new authenticated original executable bytes if globally distinct**, but this report does not increment older function-count ledgers without an exhaustive cross-phase overlap check.

## Exact x86 state mechanism

The **entity resolved from the Elite's weapon-linked serial handle** uses these fields. The DLL does not prove this referent's exact runtime class:

- `this+0x594`: unsigned 32-bit state, with native branches for 0–3.
- `this+0x590`: first float clock snapshot.
- `this+0x58C`: second float clock snapshot.
- `global 0x22572190`, dereferenced at offset `+0x0C`: float engine time, read independently at each source instruction.
- `0x22477C5C`: immutable original `float 1.0` comparison threshold.

**Gate method `0x22315320`**, invoked by Elite animation event `0x43`:

| State | Original behavior | Returns to event handler |
|---|---|---|
| 0 | virtual `+0x3C4(13, 0)`; set state 1; read engine time **twice** into fields `+0x590` then `+0x58C` | false |
| 1 | no effects | false |
| **2** | no effects | **true** |
| 3 | no effects | false |
| Any other unsigned state | no effects | false |

**Paired updater `0x22315390`**:

- State 0 or invalid: no effect.
- State 1: read current time, subtract `+0x590` using the native x87 FPU, compare against 1.0. If the result is below the threshold (or unordered), return unchanged. Otherwise set **state 2**, then **independently reread engine time twice**, writing `+0x590` and `+0x58C` in that order.
- State 2: read current time, subtract `+0x58C`, compare using the same original x87 threshold. If the result is at least 1.0, set **state 3**. Otherwise retain state 2.
- State 3: invoke original virtual `+0x3C8(13)`, then virtual `+0x3C4(14, 0)`, then set **state 0**.
- Both native methods use the resolved entity's actual vtable. We identify the original virtual slot **offsets and argument values** only; their underlying engine semantic names are not invented.

This yields the verified cycle **0 → 1 → 2 → 3 → 0**, and the Elite event43 gate returns true only in state 2. The original Elite event43 handler subsequently writes the gate result into Elite field `+0xFD9` and conditionally invokes inherited Grunt animation-event behavior.

### Original null-target behavior

The gate's **first read is `[ECX+0x594]` with no null check**. Although the Elite event handler executes the gate call even when serial-handle resolution yields zero, calling the original body on that null pointer would be invalid. Real in-game equipment invariants may prevent that situation in normal gameplay, but the DLL bytes alone do not prove why.

The portable model explicitly returns `RejectedNullTarget` for a missing native weapon. It does **not** pretend the original function has a graceful null fallback. The Phase626 host's `original_gate43(0)` remains an abstract callback and is not yet connected to a live engine weapon.

### x87 accuracy boundary

The updater's original x87 FCOMP/FNSTSW control tests the C0 unordered-or-less bit. The model compares `(double)now - (double)original_snapshot >= 1.0`, matching ordinary finite f32 game time and a conservative unordered fallback. This is not a full x87 FP status/control-word emulator and does not claim identical exceptional/trap behavior at every floating-point boundary.

## New independent source implementation

- `include/sin_compat/original_elite_event43_chaingun_gate_phase629.hpp`
- `src/original_elite_event43_chaingun_gate_phase629.cpp`
- `tests/test_original_elite_event43_chaingun_gate_phase629.cpp`
- `research/tools/audit_original_elite_event43_chaingun_gate_phase629.py`

The C++17 `Reference::gate` and `Reference::update` work on a host-supplied state snapshot with callbacks for the exact original dynamic virtual offsets and independent clock reads. An immutable instance protects against reentrant host callbacks. Host failures report stages and preserve prior state mutations without inventing rollback of already performed native effects. No real engine or commercial DLL is linked into any test.

Synthetic tests exercise:

- Full gate/update 0→1→2→3→0 cycle, sub-threshold and exact 1-second boundaries.
- Gate permits ONLY phase 2; invalid state, null-target rejection.
- Two separate clock reads in gate, three separate reads in state1 transition, no unverified coalescing.
- Native virtual `+0x3C4` and `+0x3C8` command/order, callback failure and partial state.
- NaN/unordered non-advancement, injected exceptions and host reentrancy.

The owner-original auditor requires both exact retail DLL SHA256s, hashes the **complete** 82-byte gate and 173-byte update bodies and 16-byte dispatch table, verifies case destinations, the Elite direct callsite, the literal 1-second threshold and short original opcode witnesses. Synthetic mode rejects changed invented functions/operands and never contains proprietary game bytes.

### Owner-original audit

```powershell
python research/tools/audit_original_elite_event43_chaingun_gate_phase629.py `
  --server "D:\SiN-OwnerOriginal\server.dll" `
  --client "D:\SiN-OwnerOriginal\client.dll"
```

## Next actual-game milestone

This new reference can inform the future native callback host, but neither the original updated weapon nor reconstructed Elite NPC has been run inside a game process by our code. The Phase627/628 **hardware debugger** still needs an owner-side `elite_spawn`/event-entry trace from a disposable Windows retail game run. Once real engine ownership and dispatch are demonstrated, wire this gate/update sequence behind an engine-confirmed weapon actor instead of guessing an ABI.

Already working HL2 physics/surfaces/model/prop/rendering paths remain out of scope.
