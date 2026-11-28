# Phase 1660: original-client C_BasePlayer prediction host read-only observer

## Motivation and source boundary

The original SiN 2006 `client.dll` complete SHA256 is
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.
The source-authenticated Phase1659 audit establishes that global VA
`0x24338CF0` (RVA `0x338CF0`) is a pointer to the current native
`C_BasePlayer`. Its class identity comes from the actual original
MSVC RTTI and the local-player assignment/conditional destructor clear;
it is not guessed from an engine version.

Original `CPrediction` slot8 (VA `0x24195DD0`) reads a DWORD at
`C_BasePlayer+0x1DC` or zero when no player is present. Original
slot13 (VA `0x24195F10`) reads exactly three DWORDs at
`C_BasePlayer+0xF04`, `+0xF08` and `+0xF0C` into an output
pointer, or zeros the output when no player is present. Their real
code had no general synchronization against entity deletion.

This phase does **not** make a local player, register a fake prediction
factory or claim that a real Windows game engine has started. It adds
a read-only view of **when a pointer is absent, exact-base-class
recognized, or unsafe/unrecognized** without invoking any original
method.

## Observer contract

`pc/independent_slot0/original_client_local_player_readonly_snapshot.h`
exposes a native Win32 PE32 DWORD reader adapter. The caller must
first authenticate the exact original module SHA. No game code or
engine-owned pointer is read in public CI.

1. Refuse missing reader or invalid/overflowing module base.
2. Read current-player global RVA `0x338CF0` once.
   A null pointer is a **valid, expected status**:
   `no_local_player`. The stand-alone owner probe only called
   original DllMain/static registration, so the engine has not yet
   created a local player.
3. Reject an unaligned or 32-bit overflowing active player pointer.
4. Read the candidate object primary vfptr, then require exactly
   original C_BasePlayer primary vfptr `client_base+0x28AAE4`
   before any member read. An actual derived-player class can have
   a different vptr. This observer refuses it and never equates that
   to an invalid game object.
5. For a recognized object read exactly four payload DWORDs:
   scalar at `+0x1DC`, three view words at `+0xF04` / `+0xF08` /
   `+0xF0C`. Preserve bit patterns without interpreting
   float NaNs or inferring semantics.
6. Re-read the same primary vfptr, then the global pointer.
   On change, report `player_vptr_changed` or
   `local_player_pointer_changed`, not an apparently verified
   snapshot. A repeated pointer/vptr is **not** proof of full
   engine ownership synchronization or absence of concurrent
   deletion. The host offers only bounded read checks.

One fully verified snapshot therefore requires **8 successful DWORD
reads**: global, vptr, scalar, three vector words, vptr again, global
again. This is a deterministic contract with explicit overflow,
null, unreadable memory, class mismatch and pointer turnover statuses.
No original virtual, entity factory, thunk, game simulation or
engine command is called.

## Owner-original diagnostics

The existing `research/tools/sin_win32_factory_probe_phase657.cpp`
is still an explicit owner-only executable. It requires the complete
original on-disk client/server SHA before optional Windows LoadLibraryEx,
which can invoke original DllMain. Phase1660 adds an event:

`original_local_cbaseplayer_prediction_view_snapshot`

Its JSON describes the outcome, DWORDs read, exact-vptr match,
scalar nonzero and number of nonzero view words, and reports
**zero original method/virtual calls**. It does NOT print raw original
pointers, game coordinates, machine code or proprietary image bytes.
The diagnostic treats `no_local_player` as success because this is
expected after a static-only DLL-load probe. It does not interpret
unrecognized derived-vptrs as original C_BasePlayer evidence.

The original game DLL binaries are never distributed with the
diagnostic or public GitHub Actions artifact.

## Synthetic native MSVC x86 tests

`pc/independent_slot0/test_original_client_local_player_readonly_snapshot.cpp`
uses an invented client base, an invented exact-vptr local player,
and an eight-read scripted fake memory reader. It tests:

- complete scalar and bit-exact triple vector reading (including
  NaN/infinity IEEE-754 bit patterns);
- normal null current-player state and no extra reads;
- null/overflowed module and misaligned/overflowed player addresses;
- class-vptr mismatch with refusal before field reads;
- changed vptr or changed global after reading the payload;
- failed/unreadable memory at **each of the eight read boundaries**
  with the expected number of successful DWORD reads;
- wrong field offsets/refusal and alternative zero/nonzero values.

Public CTest `independent_original_client_local_player_readonly_snapshot_native`
is original-free and requires actual MSVC Windows x86. Its addition
raises the stacked native target from Phase1659's **267** tests to
**268**.

## Limitations

This is bounded and fail-closed source-authenticated observation only.
It does **not** solve current-player lifetime, thread synchronization,
all prediction virtual method contracts, engine-managed client entities,
Windows game startup, Android executable startup or Quest 3 VR.

No physics, surface-property, prop, model-loading or rendering work
was added, per the project's active priority override.

PR is intentionally open and stacked on Phase1659 PR #1668.
