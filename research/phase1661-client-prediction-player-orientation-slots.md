# Phase 1661: four original CPrediction player-vector interface contracts

## Original file and method authentication

This phase is a retail **static, read-only** x86 audit and synthetic
MSVC Win32 x86 runtime test. It builds on Phase1660 PR #1669 and on
Phase1659's authenticated original current-local-player `C_BasePlayer`.
It does NOT enable the replacement client's `VClientPrediction001`
factory or launch the original game engine.

Exact 2006 `client.dll` complete SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
Exact `engine.dll` complete SHA256:
`f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81`.
Both are authenticated by the existing Phase1658 read-only parent
verifier before Phase1661 inspects any original code.

The complete original prediction 23-method vtable is at client
VA `0x242A363C`. Phase1661 classifies **four additional genuine
native virtual targets**, without inventing method names:

| Prediction slot | Original VA | Extent | Complete native SHA-256 |
|---:|---|---:|---|
| 9 | `0x24195DF0` | 58 bytes | `8ad6c6cc4e0e1f03fe8b7c444622224cbc799ee5f51b1d30d1c2e024f3fa65ff` |
| 10 | `0x24195E30` | 69 bytes | `0873e7c509ffea683f8677d629df469abd45624946e1b1f2ebb71277135c6ff4` |
| 11 | `0x24195E80` | 58 bytes | `a182f1e271cb3d50d82d45c749ddaa74a4ce9fa328fc9a4b9d11dfa60f7be977` |
| 12 | `0x24195EC0` | 69 bytes | `d933b2d8fac56d107628bf53d2bca6148be9a4b5dbfbaa30db81edbf2c1d1242` |

These are **254 newly scoped full original method bytes**,
distinct from Phase1658's slots and Phase1659's slots 8/13.
Each verified function is followed by compiler INT3 padding and has
a HIGHLOW relocation for the original current-player global VA
`0x24338CF0` (it is not the `cl_predict` ConVar).

## Two 3-DWORD OUT getters: slots 9 and 11

Both receive one 32-bit pointer to a caller-provided 12-byte
output span and return with `RET 4`.

- **No current local C_BasePlayer:** zero all three output DWORDs.
- **Player present:** invoke the current-player object's primary
  virtual method at vptr byte offset **`+0xE4`** (slot9) or
  **`+0xEC`** (slot11), using the player's pointer in ECX.
  The original player virtual returns a pointer in EAX.
  CPrediction immediately copies its three DWORDs into the OUT
  span. No actual player virtual implementation is reconstructed.

No null-OUT or null-virtual-return guard exists in the original
method. The synthetic implementation refuses absent buffers or
callbacks rather than dereferencing them.

## Two 3-DWORD player setters: slots 10 and 12

Both receive one caller pointer to three DWORDs and return with
`RET 4`. With no current player they do nothing.

With a player present, the original instruction order is exact:

1. Dispatch the player's virtual method at byte offset
   **`+0xE8`** (slot10) or **`+0xF0`** (slot12), passing the
   caller's pointer by one x86 stack DWORD.
2. **AFTER** that callback returns, read the three source DWORDs
   and mirror them to native player fields:
   - slot10: `player+0x2C8`, `+0x2CC`, `+0x2D0`
   - slot12: `player+0x2D4`, `+0x2D8`, `+0x2DC`.
3. **AFTER** mirroring, invoke the embedded player-side object's
   virtual byte offset `+0x0C` (fourth virtual), located at
   `player+0x22C` (slot10) or `player+0x268` (slot12).

The source pointer is read AFTER the original player callback, a
detail verified by the native fixture using a deliberately
mutating callback. Setter methods do NOT simply copy the caller's
original raw input and exit: both the primary virtual dispatch
and the secondary notifier are observable contract boundaries.

These are raw 3-DWORD values; field semantics or high-level
names are not inferred from source-free Source engine assumptions.

## Synthetic code and tests

The existing original-free `Prediction23Fixture` now has four
newly audited virtual slots 9–12. A separate
`OrientationOps` structure exposes **host-owned callback stubs**
for the two native player getters, setters, mirror writes, and
embedded notifier invocations. These are adapters, not original
C_BasePlayer implementations or restored engine vtables.

The new native MSVC x86 test
`test_original_client_prediction_player_orientation_slots.cpp`
builds an entirely invented player-layout buffer. It verifies
the authentic mirror offsets `+0x2C8`/`+0x2D4` and notifier
subobject offsets `+0x22C`/`+0x268` at compile time,
then invokes each CPrediction slot through its actual
compiler-generated function pointer, using ECX=this,
one stack pointer argument and before/after ESP invariants.

The test covers absent-player OUT-zero/no-op behavior,
bit-exact three-DWORD getter copies (including special float
bit patterns), caller-input mutation inside the player setter,
then exact three-word mirrors and notifier callbacks in
the original observed order. It also checks null-OUT/input
safety divergence, detachment, and local object destruction.

`research/tools/audit_sin_original_client_prediction_player_orientation_slots.py`
authenticates the original two complete DLL SHA gates, all
four full-method SHAs, four actual vtable indices, .text
boundaries and global relocations, plus sparse x86 structural
witnesses. The public `--self-test` constructs invented
**non-executing sparse x86 windows**, mutations of every
covered opcode/operand, and forged-original SHA rejection.
It does not store or run the original complete native code.

The original decomp synthetic GitHub Actions workflow runs
the new auditor's `--self-test`. Two additional CTests raise
the stacked native target from Phase1660's **268** to **270**.

## Limits

The original C_BasePlayer virtual target implementations,
true player lifecycle, net prediction frames, and all
remaining CPrediction methods still require reconstruction.
No runnable Windows original engine boot, Android native
execution, or Quest 3 VR startup is claimed. No original
game binaries or native method bodies are committed to Git.

No physics, surface/material, model, prop or rendering
implementation is pursued under the project's active
priority override. This PR remains open and stacked.
