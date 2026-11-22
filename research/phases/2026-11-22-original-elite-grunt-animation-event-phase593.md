# Gameplay Phase 593: Original Elite Grunt override and conditional inherited event forwarding

## What was recovered

Owner-private 2006 SiN Episodes: Emergence original `server.dll` (whole SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`) and its matched `client.dll` (`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`) were verified before reading any private code. Source recovery is restricted to gameplay-specific Grunt NPC behavior, not unrelated HL2 physics, props, surfaces, rendering, Windows factory/loader or Android/Quest runtime.

One **new complete native x86 function** is authenticated:

| Native VA | Original byte extent | Whole-method SHA256 | Ownership |
|---|---:|---|---|
| **`0x22355480`–`0x223555E7`** | **360 original code bytes** | `67d8c8be564d99bde7f2bd5f2ae7956ae0b1213aee0faf5f33c7cbff3a42a639` | Primary `CNPC_EliteGrunt` vtable `0x2248DF04 +0x444` |

The original `ret 4` ends at `0x223555E5`. Eight authentic `INT3` padding bytes follow at `0x223555E8..0x223555EF`; the next native method starts at `0x223555F0`. The entire inherited Grunt Phase592 method at `0x2235BFD0` (3,340 bytes) is reauthenticated as supporting evidence, but **not counted as new bytes**.

Exact MSVC RTTI TypeDescriptor `.?AVCNPC_EliteGrunt@@` for the original primary vtable independently proves class ownership. Original vtable slots `+0x338 -> 0x2235A250` (Grunt activity remapping) and `+0x5A8 -> 0x2235AE30` (flank voice handler) are additionally checked. Ordinary `CNPC_GruntBase`, `CNPC_MobGrunt`, and `CNPC_SintekGrunt` share inherited `+0x444 -> 0x2235BFD0`; original `CNPC_EliteJetpack` instead uses Jetpack `+0x444 -> 0x22364280`. Distinguishing these dispatch sites is essential for SiN-specific NPC fidelity.

## Original Elite-only event split

The original `0x22355480` reads the event pointer from its single stack argument and compares event ID explicitly with **`0x03`, `0x43`, `0x47`**, in that order. All other event IDs, including **`0x54`**, make an original E8 direct call at `0x22355519 -> 0x2235BFD0` and return. No additional Elite-only gate is imposed on `0x54` at this override. The inherited Grunt still retains Phase592's optional owner+0xC18 component gate before shared NPC/GRUNT_FLANK dispatch.

### Event `0x47` (`0x2235549E`)

1. Inspect the original Elite member at **`+0xFDC`** via original `0x22355300(0)`.
2. When that method returns true, call `0x22355370`, pass its return through `0x22315440`. The source-specific ABI or meaning of that selected entity is not assigned a fabricated C++ name.
3. Regardless of the result of the first condition, call original `0x220325C0` with exact float **`1.0f`** and integer `0`, discard the x87 result and return without invoking the inherited Grunt event handler.

### Event `0x43` (`0x223554D5`)

1. Resolve the original 32-bit serial handle **owner+0xFDC**: invalid sentinel `0xFFFFFFFF` becomes null; otherwise the low **12 bits** select the slot in a 16-byte slot table at original global `0x224F5620`, and the high bits must match slot serial generation. An unresolved handle becomes null.
2. Pass the resulting entity pointer (possibly null) into the exact original native predicate `0x22315320`.
3. Write the predicate's byte result at **owner `+0xFD9`**.
4. **If false**, return without invoking the inherited Grunt. **If true**, forward the unmodified event pointer through original direct call `0x22355519 -> 0x2235BFD0`. The inherited Grunt has its own dedicated `0x43` case; it does not automatically take the parent default.

The independently authored host uses callbacks for original entity slots and predicate. It guards null/stale handle safely without claiming that the engine's unknown predicate preserves all crash/exception behaviors.

### Event `0x03` (`0x22355524`)

1. Load the event's original float field **`+0x04`**, save the original x87 control word, set temporary truncate rounding, use `fistp` to convert to signed integer, and restore the saved x87 control word. **Only converted integer 1** enters the target-dependent branch; all other values return without forwarding to the Grunt parent.
2. Call original `0x22031A80` for a candidate, require its virtual `+0x120` return true and its virtual `+0x300(owner)` return false. Otherwise call original reset `0x2203D2D0(0)`.
3. Compare original engine float time minus Elite field **`+0x99C`** against genuine `.rdata` double **`3.0`** at `0x223D9EE8`. The native `fnstsw` status check enforces ordered **strict >3.0** for ordinary finite values, and the host marks unsupported x87 exception cases rather than claiming bit-for-bit equivalence.
4. On success, update owner **`+0x99C` to current time**, fetch the phrase via owner virtual **`+0x7CC`**, call embedded original sentence queue at **owner+0xCC0** with **argument `2`, mode `2`**, call original `0x220325C0(2.0f,0)`, then owner virtual `+0x4DC(1)` and return. The original does **not** check whether sentence queueing succeeds before the latter two calls.
5. On gate failure, call original `0x2203D2D0(0)` and return. The full gameplay meaning of the three target virtuals is not established here.

**Important distinction:** The Elite `0x03` voice gate's strict **3-second** condition is separate from the Grunt lost-enemy 10-second threshold in Phase580 and from the Grunt `GRUNT_FLANK` route-length >240 check in Phases565–566.

## Original evidence and C++17 reference

Read-only original auditor: `research/tools/audit_sin_original_elite_grunt_event_phase593.py`.

- Original `client.dll` and `server.dll` unchanged full SHA256 prerequisites: **PASS**.
- The full one-function **360-byte original SHA256**, original `ret 4`, and 8 subsequent `INT3` bytes: **PASS**.
- Original `CNPC_EliteGrunt` TypeDescriptor, primary `+0x444`/`+0x338`/`+0x5A8` pointer words, three ordinary-Grunt parent pointers, and independent `CNPC_EliteJetpack` override: **PASS**.
- All **10 original E8 rel32 direct-call edges** and **29 original instruction/field witnesses**, exact `1.0f`, `2.0f` and `.rdata` **3.0 double**: **PASS**.
- Synthetic original-free self-test mutates each byte of each of the ten E8 call encodings and rejects unsupported owner binaries: **PASS**.
- Bounded source reference: `include/sin_compat/original_elite_grunt_event_phase593.hpp`, with `tests/test_original_elite_grunt_event_phase593.cpp`. It explicitly distinguishes the three event IDs, event `0x54` inherited delegation, `0x43` handle/predicate state write, `0x47` callback ordering, `0x03` float-to-integer gate and time-dependent voice/timed action sequence. Engine target predicates, native host object lifetimes, animation clip emitters and exotic x87 float behavior remain outside its asserted equivalence.
- Strict GCC and Clang C++17 warnings-as-errors, GCC ASan/UBSan and standalone CTest **2/2 PASS**.

No commercial DLL, full native disassembly, retail map/model, game assets or executable patch are shared. This is a source-bounded gameplay reconstruction, not a fully loadable `server.dll` or a playable port.

### Next source target

Original `CNPC_EliteGrunt` target dependency `0x22355300` and `0x22355370` can clarify which real entity/attachment drives event `0x47`. Separately, identify owner `+0xC18` component virtual `+0x60` to determine which forwarded events, including `0x54`, are consumed before shared NPC dispatch.