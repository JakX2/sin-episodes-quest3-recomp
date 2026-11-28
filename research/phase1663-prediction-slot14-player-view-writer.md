# Phase 1663: original CPrediction slot14 / C_BasePlayer view writer

## Provenance and exact native linkage

Phase1658's original-file auditor authenticates the unchanged 2006
`client.dll` complete SHA256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
and `engine.dll` complete SHA256
`f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.
No original binaries or full copyrighted machine-code bodies are
committed to Git, and no original game code executes in public CI.

Phase1659 established original current-player global
`0x24338CF0` (client RVA `0x338CF0`), with native writer,
conditional destructor clearing and primary RTTI proving
`C_BasePlayer`. Its CPrediction slot13 (VA `0x24195F10`)
takes one three-DWORD OUT pointer, reads player+`0xF04`
/`0xF08`/`0xF0C`, or zeroes the output if no current player.

This phase closes that read/write pair by authenticating
**two more original native methods, 51 bytes combined**.

| Method | Original client VA | Bytes | Entire original SHA256 |
|---|---|---:|---|
| CPrediction vtable slot14 | `0x24195F50` | 18 | `067addd7541796c20977a3e297c56aad4f04ea9c267d7b774a56553a87a9c240` |
| C_BasePlayer nonvirtual view writer | `0x240D22E0` | 33 | `32406e3e02bca0f9490059e964d1e396a4dfe356c9796d42c86c3ceb8de87793` |

The original 23-slot prediction vtable at VA `0x242A363C`
contains the exact target at vtable cell `0x242A3674`.
That cell and the client player-global operand in slot14 have
actual HIGHLOW relocations in the owner original PE32 file.

### CPrediction slot14 guard and tail jump

Original method slot14 reads the global C_BasePlayer pointer,
tests for null, and has two cases:

- **Current player null:** return with `RET 4` without touching
  the caller's single pointer arg.
- **Player present:** jump directly to
  `C_BasePlayer::nonvirtual 0x240D22E0`, preserving thiscall's
  original single pointer stack arg. It uses a native x86
  **tail JMP**, not a C++ virtual call or a standard call
  followed by a return.

The full method SHA and relative JMP target are authenticated;
the null branch and original method extent (18 bytes) are
explicitly checked.

### C_BasePlayer view writer

Original 33-byte player method takes a pointer to three
consecutive 32-bit words. It copies:

- source DWORD[0] -> player +`0xF04`,
- source DWORD[1] -> player +`0xF08`,
- source DWORD[2] -> player +`0xF0C`,

then returns with `RET 4`. The method has no comparison,
dirty helper, orientation notifier or float conversion.
The values are copied *bit-exactly*, including negative zero,
infinity and NaN payload bits. The original does not validate
a null input pointer.

This is **different** from the original C_BasePlayer
orientation setters at vtable slots58/60, recovered by
Phase1662, which compare floats with x87 before copying.

Both new original native extents end immediately before
INT3 padding; their full hashes are pinned by the source audit.

## Original-free implementation and regression

The existing 23-slot MSVC x86 prediction fixture implements
its previously inert slot14 as
`SetHostView(ViewBits3*)`, with the original no-player
no-op and raw three-word copy behavior. A separate
`set_writable_host_view(HostSnapshot*)` method explicitly grants
synthetic write access to an invented host. This never casts away
constness of the existing Phase1659
`set_view_host(const HostSnapshot*)` read-only observer.

A switch back to the read-only API explicitly **revokes**
previous write authority. The model also refuses null
setter pointers when a synthetic host is present, instead
of reproducing the original null-pointer crash.

`test_original_client_prediction_view_setter_slot14.cpp`
is a genuine original-free MSVC x86 executable. It calls
both compiler-generated prediction virtual slots13/14
with ECX=this, one pointer argument and before/after
ESP invariants. It checks:

- no-player slot14 no-op and slot13 zero-filled OUT;
- player+`0xF04` field alignment in invented host;
- slot14 source -> exact three DWORDs -> slot13 OUT;
- bitwise special values and unchanged input/canaries;
- nonnull host/null input safety refusal;
- host replacement preserves old host contents;
- switching to read-only revokes writes;
- player-host detachment does not read stale memory.

The owner-original audit
`research/tools/audit_sin_original_prediction_view_setter_slot14.py`
authenticates original client/engine file SHA gate, both
entire-function SHA digests, exact native slot14 vtable cell,
global PE32 relocations, 18-byte tail branch to the 33-byte
player method, sparse word-write opcode witnesses, and exact
INT3-padded boundaries. The public `--self-test` mutates
invented synthetic x86 instruction windows, rejects forged
method targets, invalid offsets and fake retail-image SHA.

One new public Python CTest plus one actual x86 native CTest
raise the stacked target from **272 to 274 total CTests**.
Original-free synthetic decomp GitHub Actions also executes
the new source auditor self-test.

## Limits

This only reconstructs the raw current-player view read/write
pair. The actual engine-owned C_BasePlayer lifetime, total
prediction object contract, incomplete original prediction
virtual methods, live game loop and factory initialization
are still unresolved.

No production VClientPrediction001 CreateInterface registration,
original retail Windows gameplay boot, Android native startup
or Quest 3 VR startup is claimed. No physics, surfaces,
materials, models or prop/loading/rendering work is added
under the project's active priority override.

PR is intentionally open and stacked on Phase1662 PR #1671.
