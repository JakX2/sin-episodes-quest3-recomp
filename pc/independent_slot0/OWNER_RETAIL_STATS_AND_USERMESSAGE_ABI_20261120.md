# First real two-stage Stats engine success and next client user-message ABI

## Owner-supplied retail-engine evidence, 2026-11-20

The owner tested the rebuilt independent Windows PE32 client.dll with
the genuine original SiN Episodes: Emergence 2006 Source engine using
a disposable, SHA-authenticated game copy. The private 2026-11-20
three-phase results ZIP was inspected directly. No private paths,
game DLLs, commercial source files or raw trace logs are committed.

Three separately bounded original Windows process launches completed:

| Phase | Genuine original engine exit | Observed result |
|---|---|---|
| Presence | 0x53494E68 | Both stat script files present via VFileSystem012 |
| Read | 0x53494E6B | Open/Size(handle)/Read/Close, counts 62:8:156:66 |
| Original staged resources | **0x53494E70** | Definition rb/GAME and layout rt/null; separately owned 66-slot catalog, 62 bindings, 8 percentage refs, 156 layout pairs, 66 dividers |

The final stage emitted
`engine-fs-staged-counts=66:62:8:156:66`,
`engine-fs-staged-stage=7:7` and
`engine-fs-staged-result=owned-staged-verified-not-engine-init`.
There was **one transient UnauthorizedAccessException on restoring
the independent DLL from the disposable game copy**. The new bounded
recovery loop retried successfully and logged
`OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE`.
The one-click summary confirmed
`VERIFIED_AND_REMOVED_DISPOSABLE_COPY` and the owner's original
installed game DLLs were never the swap targets.

**Evidence limit:** These are original-engine method calls with
independent authored Stats objects. The stage 4 UI / engine state
side effects and the final three CUserMessages registration callbacks
were inert stand-ins in the owner-executed build; the seven-step count
does NOT establish original C_StatsSystem::Init completion or a normal
VClient011::Init return. The highest owner-proven milestone remains
**0x53494E70**. No gameplay was reached.

## SHA-grounded original network message hook research

Original client.dll SHA-256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

The original client Stats Init at 0x241FCAA0 calls the private
**CUserMessages::HookMessage** at 0x241CFF70, NOT the console command
or VEngineCvar002 API. Its original error identifies missing message
names in CUserMessages. `HookMessage` is x86 thiscall with two 32-bit
parameters, ending in `ret 8`; it looks up a message by name and
appends the native callback to that message's hook vector.

Original callback registrations (all addresses are read-only evidence
and MUST NOT be invoked from independent client.dll):

| Stats Init callsite | Network message | Shared message ID | Message size | Original handler |
|---|---|---:|---:|---|
| 0x241FCADF | StaticStat | 27 | variable (-1) | 0x241FB7A0 |
| 0x241FCAF4 | DynamicStat | 28 | variable (-1) | 0x241FB7E0 |
| 0x241FCB09 | ResetHUD | 5 | 1 byte | 0x241FCA60 |

Both original client and original server register the same 39
messages in the same order. Our prior shared independent protocol
catalog is already based on that dual SHA-grounded comparison.

Original handler disassembly:
- StaticStat at 0x241FB7A0 begins with a call to
  `bf_read::ReadUBitLong(16)` at 0x240A5600 and routes to stat
  definition virtual method +0x4c. The 55..65 dynamic indices are
  excluded from the normal StaticStat path.
- DynamicStat at 0x241FB7E0 calls helper 0x241FB370, which also first
  reads a 16-bit unsigned stat ID, then dispatches through the dynamic
  record machinery for IDs >=55. The independent model additionally
  refuses out-of-range IDs >=66.
- ResetHUD at 0x241FCA60 resets multiple original stat containers and
  reinitializes both original static/dynamic record groups. Its entire
  runtime behavior remains only partially characterized.
- The rest of the StaticStat/DynamicStat wire formats are different
  unknown type-specific values: knowing the initial 16-bit index
  does not establish payload decoding or actual in-game effects.

The independent `client_stats_user_message_hooks.h` now checks the
three original callsites and expected names/IDs/signed sizes and
installs the **three hooks into an owned registry** during staged
Stats Init. Its bounded user-message header inspector can identify
the two 16-bit stat index classes and the one-byte ResetHUD
envelope, leaving all subsequent payload bytes opaque. This does
not register real original-engine callbacks, read private BFRead
objects or implement the game messages' effects. Subsequent unknown
UI callbacks remain inert. This stronger diagnostic has only been
native-MSVC synthetic tested until the next owner trial.

## Another original ABI discovery: VClient011 slot 33

The 2006 client vtable is 48 contiguous code pointers beginning at
**0x24296C6C**. Entry **33** (0x24296CF0) is the pointer 0x240FDCD0.
The 0x240FDCD0 function loads the process-local user-message registry
and jumps to **0x241D0000**,
`CUserMessages::DispatchUserMessage(int,bf_read*)`.

The dispatch helper checks the message index against its registry,
rejects missing handlers and, for each installed handler, copies six
32-bit words (24 bytes) of the bit-buffer state to its own stack before
invoking that handler. It returns a Boolean with thiscall `ret 8`.
That copying behavior allows each listener its own read cursor.
The original handlers are not part of the rebuilt DLL.

Our independent VClient011 slot 33 previously used a generic
fail-fast guard. It now has the correct `bool __thiscall(int,void*)`
signature but **always returns false**, recording the refusal without
touching caller-supplied BFRead memory. This avoids falsely delivering
a user message before real transport and payload decoding exist.
Native x86 test invokes the ACTUAL DLL-owned slot 33 through its
vtable and checks stack cleanup and the false return for valid,
unknown and null-reader inputs. This replacement remains a safe
ABI boundary, not a working network receiver.

## Next concrete work

1. Recover original per-stat virtual value decoder interfaces
   (StaticStat vtable +0x4c), dynamic stream string/number decoding
   and stat record update semantics.
2. Implement independent bounded BFRead-compatible bit cursors
   rather than guessing a byte-aligned payload structure.
3. Replace fail-closed slot 33 only after native synthetic fixtures
   verify message ID bounds, per-handler reader copies, and correct
   callback delivery for every supported payload.
4. Reach a real normal Windows client Init return, then build server
   entity/user-message transmission integration. Only after the
   working Windows version should Android and Quest 3 ports proceed.

The new owned-hook and slot33 code is **not** a new owner-verified
retail milestone until it runs in a subsequent original engine trial.
