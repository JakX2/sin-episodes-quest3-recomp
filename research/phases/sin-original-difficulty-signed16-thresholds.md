# SiN Episodes 2006 dynamic difficulty: exact signed-16 wire fields and x87 threshold paths

**Primary evidence:** unmodified owner-supplied original client.dll SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` and server.dll SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Verified again on 2026-11-03 using real x86 code. Proprietary executable bytes are private, not in the repository.

## Important correction to PR #72 and initial #73

The original `DDDecisionMade` ordinary server writer **loads three 32-bit object members** but calls `0x221E87A0`. At `0x221E87C2`, that wrapper calls `0x2225A160`. At `0x2225A164`, `push 0x10` selects **16 bits**, and its callee `0x22259B00` explicitly handles **signed** values (separate negative and nonnegative paths). The client `0x240A5600` similarly pushes `0x10` and calls `0x240A5080`, its signed-bit reader. The float helper paths on both sides use 32 bits (`0x22258F00` / `0x240A5631`).

Therefore the actual ordinary **on-wire field operation sequence** is:

| Order | Type | Original server CALL site | Original client CALL site |
|---:|---|---|---|
| 1 | float32 | 0x222B9787 | 0x241FAB1D |
| 2 | signed16 | 0x222B9790 | 0x241FAB7C |
| 3 | signed16 | 0x222B9799 | 0x241FAB85 |
| 4 | signed16 | 0x222B97A2 | 0x241FAB8E |
| 5 | float32 | 0x222B97AB | 0x241FAB99 |
| 6 | float32 | 0x222B97B4 | 0x241FABA4 |
| 7 | float32 | 0x222B97BD | 0x241FABAF |
| 8 | float32 | 0x222B97C6 | 0x241FABBA |

**32 + 3×16 + 4×32 = 208 payload bits (26 bytes)** for an ordinary record, **before** usermessage framing and other engine transport metadata. This is not an 8×32-bit packet.

## Client negative-value control branches (not exact 0/-1 equality)

Client disassembly of `0x241FAB10` shows:

- `0x241FAB26` compares the first float with `0.0f` using x87 `FCOMP`; `0x241FAB2E` `TEST AH, 5`; `0x241FAB31` `JP 0x241FAB6D`. For finite values, **first >= 0** continues to read the seven remaining ordinary fields.
- If **first < 0**, `0x241FAB37` compares against `-1.0f`; `0x241FAB3F` `TEST AH,5`; `0x241FAB42` `JP 0x241FAB57`.
- For **-1 <= first < 0**, client clears two 32-bit members at `this+0x9C` and `this+0xA0`.
- For **first < -1**, client calls `0x241FA450` with `this+0xA4`, a distinct list/container operation.
- x87 unordered comparisons (for NaN) follow the normal-read jump via the observed flag test; no NaN gameplay behavior is claimed.

The original server has an independently verified **single -1.0f notification** at `0x222BA885`, written by `0x221E8830`, but other negative messages, and the meaning of both client cleanup branches, remain to be traced. The prior labels `ZeroSentinel`/`MinusOneSentinel` are misleading; **zero itself is on the ordinary path**.

## Source changes and regression safeguards

- Updated `include/sin_compat/difficulty_message_contract.hpp` and its C++ tests in the parent PR #73: signed-16 typed operations, threshold classification rather than equality markers, allow first=0 in ordinary records.
- Updated `audit_sin_difficulty_protocol.py` and its synthetic tests: use `s16` in the recovered *wire* sequence. The **in-memory** `CDynamicDifficultyAction` class member offsets and `i32` field types are unaffected.
- `audit_sin_difficulty_bitwidths.py` now checks exact **helper call edges, bit-width pushes, signed-bit branches, threshold comparisons and client state offsets** on SHA-verified original 2006 binaries. It reuses the original protocol auditor rather than duplicating the PE parser.
- Additional original-free regression fixtures reject mutated width constants, x86 call destinations, condition codes, state offsets, and server notification constants.

Reproduce against private original DLLs:

```powershell
py research/tools/audit_sin_difficulty_bitwidths.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --output "C:\private\sin-dd-bitwidths.json"
```

**Scope:** this establishes original field sizes and key client side effects, not original game scoring formulas, all DD message formats, byte-perfect usermessage framing, or native ARM64 SourceVR integration. Those remain subsequent implementation tasks.
