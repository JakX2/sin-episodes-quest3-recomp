# Phase 75: Original SiN Episodes DDInfo cross-DLL wire and client state

**Evidence:** owner-held original 2006 Windows x86 `client.dll` SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` and `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Both independently rehashed and the relevant x86 instructions inspected on 2026-11-03. Private DLL bytes stay outside GitHub.

## Original code verified

- Server sender function `0x222BA5B0`, opening the `DDInfo` usermessage at `0x222BA5FD`; client handler `0x241F99A0`, reached via the prior verified usermessage callback at `0x241F9F80`.
- Exact typed field sequence **bit, bit, float32, signed16, bit, six bits**. Underlying bit writer `0x221E9690` manipulates exactly one bit per call; float helper uses 32 bits, signed-int helper uses 16 bits. Total field operations: **57 bits**. This is **not** the full message envelope size or a claim about byte alignment.
- Sender's first bits load bytes at `this+4` and `this+5`. It computes a float difference using a value at `this+0x60`, compares against original zero at `0x223D2590`, and zeros a negative result; it then writes the 32-bit float. Next it reads a source member at `this+0x40`, sending its signed **16-bit** wire representation. It writes the shared flag loaded from original global `0x225927E4`.
- Last, the sender iterates **six pointer entries** beginning at `this+0x48` (pointer stride 4); each virtual `[vtbl+0x18]` call provides a one-bit value. The meanings of the entries remain unproven.
- Client reads two bits into bytes `this+4`/`this+5`, then float to `this+8`, signed wire integer into a **32-bit client member** at `this+0xC`, shared bit to `this+6`. It consumes the six remaining flags into six bytes `this+0x28..0x2D` using two three-bit groups.
- **Important override:** client branches at `0x241F9AD2`, `0x241F9ADA` and their subsequent group-index equivalents, **replacing positions 2 and 3** of the six flags with `this+6` after their bits are read. It then clears `this+0xB8`. Thus raw transmitted flags and client effective flags are not necessarily identical.

These observations reflect direct instructions from the exact original DLLs, not inference from HL2 or from debug strings alone.

## Committed source and tests

- `research/tools/audit_sin_ddinfo.py` reuses the earlier SHA-pinned original dual-DLL auditor; checks sender and receiver exact code bytes, real original call targets, and the override branches. Emits JSON metadata only.
- `research/tests/test_sin_ddinfo.py` is an original-free mutation fixture covering widths, call targets, field operations, client flag handling and message name.
- `include/sin_compat/difficulty_info_contract.hpp` adds a host-independent typed read/write contract. It preserves raw wire flags and exposes a separate effective-client-flags calculation, without inventing gameplay meanings or bitstream packing.
- `tests/test_difficulty_info_contract.cpp` tests 11 typed operations, truncation/error handling, sample roundtrip, and the override. Registered with CMake's CTest.

Direct original-binary audit and local C++17 tests passed before committing. These tests do **not** mean an Android client/server runtime has implemented DDInfo.

## Reproduction on the original binaries

```powershell
py research/tools/audit_sin_ddinfo.py --client "C:\SiN\bin\client.dll" --server "C:\SiN\bin\server.dll" --output "C:\private\sin-ddinfo-report.json"
```

## Still unknown

The exact game-facing names and semantics of these flags; which performance values fill the shared member; callback timing; Source usermessage framing/padding; server-side adaptive difficulty scoring and decision formulas; actual ARM64 engine integration. The next useful research is `DDAdvisorScores` (`0x222BA6A0` server / `0x241FAA60` client), `DDAdvisorChannels` (`0x222BA780` server / `0x241FA850` client) and the nonvirtual manager call graph.

**Scope boundary:** this is a concrete original cross-DLL gameplay-message reconstruction milestone, **not** complete DLL decompilation or a playable Quest 3 build.
