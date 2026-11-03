# Phase 76: Original SiN DDAdvisorScores / DDAdvisorChannels behavior

## Primary evidence and limits

Original **SiN Episodes: Emergence** 2006 owner-held Windows x86 PE32 binaries were inspected directly and exact SHA-256 verified:

- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

The original DLLs are private and not committed. Prior PRs #72-75 established the paired message registrations, original helper functions and the fact that the signed integer *wire operations* are 16 bits (not 32-bit despite 32-bit C++ member registers).

## Two newly verified cross-DLL protocols

| Message | Server writer | Client reader | Exact repeating field operations | Field data only |
|---|---|---|---|---:|
| `DDAdvisorScores` | `0x222BA6A0` | `0x241FAA60` | 6 × (f32 + signed16) | **288 bits / 36 bytes** |
| `DDAdvisorChannels` | `0x222BA780` | `0x241FA850` | 6 × (f32 + f32 + f32 + f32) | **768 bits / 96 bytes** |

Both server senders check a global state object and that a recipient/message object is available before initiating the usermessage. They use six advisor pointers beginning at server object offset `+0x48` with a 4-byte pointer stride; both messages are sent via the original usermessage writers `0x221E86D0` (begin), `0x221E8830` (float32), `0x221E87A0` (signed16 where used), `0x221E8720` (end). These sizes exclude the message envelope and any byte alignment.

### Scores: server code remapping, not a score formula

Per-advisor original server member reads: `+0x1C` (float32 score value) and `+0x20` (code). The server also reads a context code from manager `+0x40` for advisors 0–3. Original code `0x222BA714..0x222BA750` contains a verified `CMP/Jcc` decision tree. The following **independent C++ equivalent** reproduces that tree for finite signed integer inputs:

```cpp
if (index >= 4) return candidate;
if (state < 2 && candidate < 2 && state <= candidate) return 2;
if (state > 2 && candidate > 2 && state >= candidate) return 2;
return candidate;
```

Advisors **4 and 5** bypass the remapping. The numeric value **2** is a verified compiled constant; what it represents in gameplay is *unknown*. This is not a reconstruction of advisor weighting or player performance formulas.

| Context / Candidate | 0 | 1 | 2 | 3 | 4 |
|---:|---:|---:|---:|---:|---:|
| 0 | 2 | 2 | 2 | 3 | 4 |
| 1 | 0 | 2 | 2 | 3 | 4 |
| 2 | 0 | 1 | 2 | 3 | 4 |
| 3 | 0 | 1 | 2 | 2 | 4 |
| 4 | 0 | 1 | 2 | 2 | 2 |

Original client score handler `0x241FAA60` reads six floats via `0x240A5630` and six signed16 values via `0x240A5600`. Floats populate/reset a dynamic vector at client `this+0x30`; integer results are placed in **32-bit client slots** beginning at `this+0x10`. The handler then clears the state byte at `this+0xB8`.

### Channels: four scalar values for each of six advisor groups

Original server member reads per advisor at **`+0x24/+0x28/+0x2C/+0x30`**, written as four 32-bit floats before moving to the next advisor. The original client reader `0x241FA850` loops **six** times and reads four consecutive floats each time.

The client transposes this wire layout into four separate dynamic float vectors at `this+0x44`, `+0x58`, `+0x6C`, `+0x80`. Each vector count resets at message entry, and `this+0xB8` is cleared after processing.

## Committed verification and implementation

- `research/tools/audit_sin_dd_advisors.py` runs **all prior SHA-256-pinned original DDInfo/protocol checks**, then validates exact original server score-code branch bytes, message identifiers, signed16/f32 helper widths, six loops, source field member offsets, client layouts and real direct call targets. Emits JSON only.
- `research/tests/test_sin_dd_advisors.py` supplies synthetic, original-free x86 fixtures; it rejects corrupted branch conditions, widths, group loop counts, writer/reader call edges and member offsets.
- `include/sin_compat/difficulty_advisors_contract.hpp` defines two host-independent typed field contracts, normalized score-code decision logic, and explicit conversion to the four client column arrays.
- `tests/test_difficulty_advisors_contract.cpp` checks field order, 6×2 and 6×4 serialization, output atomicity on truncated input, client storage, and a 5×5 context/candidate table. No original DLL content appears in tests.
- `CMakeLists.txt` adds this C++17 regression to desktop and ARM64-compatible builds.

Local audit on the original SHA-verified 2006 DLLs passed. Standalone C++17 compilation under `-Wall -Wextra -Wpedantic -Werror` passed, and eight synthetic audit tests passed. The external CI status is tracked separately on the PR.

## Next needed work

1. Decode the meaning of these six advisor groups, candidate codes, and dynamic manager state. No advisor names or weights are claimed yet.
2. Trace server nonvirtual `CDynamicDifficulty` progression/scoring call graph and effects on `CSE1Rules`, NPCs, saves and game state.
3. Verify exact Source message envelope/bitstream timing, not just these typed field calls.
4. Integrate against actual available Source/SourceVR native client/server interfaces and compare one real SiN encounter.

**This does not constitute complete original C++ recovery or a playable Quest 3 ARM64 game build.**
