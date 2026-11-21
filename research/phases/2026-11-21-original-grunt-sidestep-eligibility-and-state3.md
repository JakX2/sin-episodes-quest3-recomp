# Phase 536: original CNPC_MobGrunt mobility-sidestep eligibility and selector state 3

## Scope, evidence and explicit limitations

This phase rechecks the original 2006 SiN Episodes `server.dll` / `client.dll`, whose full-file SHA-256 values are respectively:

- `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The immediate motivation is the owner's two genuine 2006 Windows console logs on `SE1_U4Lab05`. The Assault Rifle Grunt in `condump001.txt` runs `SCHED_GRUNT_MOBILITY_SIDESTEP`, later fails `TASK_GRUNT_SIDESTEP_GOAL` with `Don't have a route`, enters `SCHED_GRUNT_COMBAT_FAIL`, and resumes ranged attack after `COND_CAN_RANGE_ATTACK1`. The earlier Pistol and Scattergun logs contain different behaviors and assault-navigation failures. These reports are retained in Phase 534 research notes. They do not establish which low-level return code maps to a visible schedule name.

The original Grunt vtable at `0x2249AAA4` contains virtual byte `+0x848` = `0x22359620`, the `GruntVtable530` call site previously covered by Phase 131. The entire method is now independently verified as **164 bytes**, SHA-256 `4eab1c761ef9d125b01708a37c919bec1b4042f3d99a834a16243a8f23364f3f`.

The original Grunt schedule selector at `0x2235E4E0` is **991 bytes**, SHA-256 `05b949835363d8576fbc9839747474575c20e408c0c3a2be8ac9ec4e1d89acfd`. We verify both full methods and individual byte/code witnesses against the original DLL, not just the presence of matching text strings.

## Verified +0x848 gate execution order

1. Reject when the native byte at Grunt `+0xCA0` is nonzero.
2. Reject when bit `0x02` of native byte `+0x988` is set.
3. Query original `0x220397C0` for shared condition ID **4**. Reject if present.
4. Query the same original method for shared condition ID **3**. Reject if present.
5. Reject when native dword `+0x8C` is nonzero. Important: this test occurs **after** condition checks 4 and 3.
6. Obtain a context argument through calls `0x2202E310` and `0x220092D0`; pass it through the Grunt virtual at `+0x304`. Reject if the virtual returns zero. The context type/semantic role is not recovered.
7. Obtain a manager-like object from original `0x2202E310`; call its own virtual at `+0x300`, passing the Grunt as its argument. Reject if it returns zero. The manager's exact C++ interface name is not claimed.
8. Query original condition **0x4D**, previously registered in Phase 7 as `COND_GRUNT_IN_PLAYER_SIGHTS`.
9. Load an original global pointer at `0x22572914`. Call its virtual `+0x1A0` with **4.0f** when the sight condition is present, otherwise **1.0f**. Return that native virtual's boolean result.

The original constants are IEEE-754 `0x40800000` for 4.0f and `0x3F800000` for 1.0f. This exactly corroborates the earlier Phase 131 preserved call-site factors at original `0x223596A9` and `0x223596B8`. Do not reinterpret the factor as direct player movement speed: Phase 131 describes it as a difficulty/mobility decision via the original game-rules virtual. The independent host supplies the genuine rules object, player-sight condition, RNG semantics and method return values.

## Verified selector integration at native owner field `+0x850`

After higher-priority conditions, original `0x2235E4E0` checks Grunt dword `+0x850`. When its value is exactly **3**, the selector:

- Calls its own Grunt virtual `+0x848`.
- When that gate returns nonzero, **returns the native numeric schedule ID `0x55`**, witness `0x2235E7C8`.
- When the gate returns zero, calls Grunt virtual `+0x844`; if that virtual returns a nonzero schedule ID, returns it immediately. Otherwise it continues to later selector code.

**Critical evidence boundary:** this report authenticates native ID **0x55** and the distinct retail string `SCHED_GRUNT_MOBILITY_SIDESTEP`, but **does not independently prove their string/number registration is the same**. It is a plausible mapping requiring direct original schedule-registry corroboration. The portable code therefore returns raw original ID `0x55` rather than giving it an unverified enum label.

Other `+0x850` states (1, 2, 4, 6/7) and higher-priority branches are outside the isolated helper; Phase 132's partial selector continues to own those gaps. State 3 is not a global always-on sidestep condition.

## Independent portable implementation and tests

- `include/sin_compat/original_grunt_sidestep_gate.hpp` and `src/original_grunt_sidestep_gate.cpp` model the exact short-circuit order without copying or calling 2006 x86 method pointers. The `IHost` gives access to the real shared-condition queries, source virtual +0x304, manager virtual +0x300 and global rules +0x1A0. Host-native flags are supplied through explicit offset-labelled boolean facts.
- The verified state-3 selector fragment is a separately named helper: it returns raw ID `0x55` on successful gate, invokes native virtual +0x844 on a clean rejection, and returns no schedule if the fallback yields zero. It must only be invoked when the original full selector has reached this precise branch.
- `tests/test_original_grunt_sidestep_gate.cpp` covers native offsets and identities, every early rejection, strict callback ordering, factor 1 and factor 4, successful schedule 0x55, unsuccessful fallback (0), nonzero fallback return, other state (no calls), exceptions and reentrant callbacks. Exceptional host errors fail closed and intentionally do not attempt a fallback; this is independent adapter safety, **not an assertion about the original x86 exception model**.
- `research/tools/audit_sin_original_grunt_sidestep_gate.py` requires both exact original whole-DLL hashes, verifies two complete method SHA-256s (**1,155 complete original bytes**), original vtable entry, 17 instruction/field witnesses and six rel32 direct calls. Includes a synthetic mutation self-test.

Local strict GCC and Clang C++17 compilation and host tests passed. AddressSanitizer/UndefinedBehaviorSanitizer passed. A standalone CMake/CTest run passed 2/2 tests. The auditor was also executed against the exact original owner's `bin.zip` and returned `ORIGINAL_GRUNT_SIDESTEP_GATE_AUTHENTICATED`.

## What this does not prove

This phase does not provide native entity AI navigation, map pathing, schedule registry names, game-engine `ServerGameDLL003::DLLInit`, a runnable Windows game module, or an Android/VR port. The actual retail Grunt's `Don't have a route` is a **movement scheduling failure**, not proof that the binary eligibility gate is incorrect. Source/HL2 physics, surfaces, models and props are deliberately excluded per owner priority.