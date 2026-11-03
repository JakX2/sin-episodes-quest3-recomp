# Phase 77: Original SiN 2006 dynamic-difficulty manager archive state

## Primary binary evidence

Original SHA-256-verified owner-held x86 Windows binaries inspected directly:
- client.dll: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- server.dll: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

Original copyrighted bytes stay local and private. The terms "writer", "reader", and "archive" below describe observed virtual serialization interfaces, **not a demonstrated in-game Save/Load trigger or a fully decoded Source savegame file**.

## Verified original server manager dispatch

The original archive writer at **0x222BB680** calls:
- `0x222B9070` for 14 float metrics (CALL site **0x222BB690**)
- `0x222B92A0` for six polymorphic advisor objects (CALL site **0x222BB69B**)
- `0x222B91B0` for ten pairs of success-modifier floats (CALL site **0x222BB6A3**)

The corresponding archive reader at **0x222BBF50** calls:
- `0x222B90C0` for at most 14 metrics (CALL site **0x222BBF6D**)
- `0x222B92F0` for at most six advisor objects (CALL site **0x222BBF78**)
- `0x222B9220` for at most ten modifier pairs (CALL site **0x222BBF80**)

These are six genuine original direct CALL edges, not string associations. Another six verified edges link the scalar helpers and modifier selector functions.

## Three separately bounded state blocks

| Original group | Archive writer | Reader cap | Evidence |
|---|---:|---:|---|
| Per-index metric floats | 14 × f32 | 14 floats | Writer count `0xE` at `0x222B9082`, loop compare `0x222B90AE`; reader compare `0x222B90F7` |
| Success modifier pairs | 10 × (f32 selector 1, f32 selector 3) | 10 pairs | Writer count `0x14` at `0x222B91C2`; loop compare `0x222B920D`; reader compare `0x222B9260` |
| Opaque advisor objects | 6 | 6 objects | Six-object count `0x222B92B4`, reader six bound `0x222B9311`; original advisor pointers start at manager `+0x48` |

The metric writer accesses original `0x222D33E0(index)` and the bounded restore uses `0x222D3340(index)` to locate the mutable metric float. Their individual semantic names are still unknown.

The modifier writer uses original `0x222C07A0(index,selector)` with selectors **1 then 3**; the reader applies `0x222C0810(index,selector,value)`, again with selectors 1 and 3. These match prior network evidence but are independently reached through a separate **state archive** path.

### Restore count behavior

Original reader obtains a signed 16-bit scalar count via archive virtual `+0x34` (sign-extended AX). It separately reads the array through archive virtual `+0x44`.

For metrics, copying stops at 14 regardless of a longer input list. For modifiers, the reader derives the number of complete pairs using signed integer division by 2, rounding toward zero, then caps the loop at 10. A positive **odd count** such as 19 therefore restores **9 complete pairs**, with the unpaired value not mapped into a modifier pair. Advisor restoration has its own six-object bound. Negative count paths skip per-entry loops.

There is **no established file-layout endian conversion, packet envelope, serializer revision compatibility, or proof that all remaining manager members are saved by these loops**.

## Commit and verification

- `research/tools/audit_sin_manager_state.py`: standard-library PE32/SHA-pinned verifier for **35 original opcode sites and 12 direct call edges**. Both exact original DLL hashes are mandatory; generated JSON contains metadata only.
- `research/tests/test_sin_manager_state.py`: synthetic original-free mutations for every tested instruction and direct CALL, plus SHA and PE guards.
- `include/sin_compat/difficulty_state_archive_contract.hpp`: clean-room, C++17 numeric field snapshot/restore reference, preserving exact 14/10/6 limits, pair ordering and odd count behavior without inventing engine storage.
- `tests/test_difficulty_state_archive_contract.cpp`: roundtrip, over-limit, odd scalar count, partial restore, truncated input and negative count tests.
- `CMakeLists.txt`: CTest registration.

The exact original binaries passed the Phase 77 verifier locally. Synthetic Python mutation tests and C++17 `-Wall -Wextra -Wpedantic -Werror` tests passed locally. GitHub CI must be checked separately.

## Next reverse engineering

1. Trace the archive interface to the actual engine Save/Restore event and verify re-entry, persistence and versioned game-state behavior.
2. Analyze the per-index metric and modifier formulas behind `0x222D33E0`, `0x222C07A0` and nonvirtual manager methods; do not invent meanings from index numbers.
3. Determine the 6 advisor object types and internal save/restore operations.
4. Validate reconstructed state changes using original SiN gameplay and a genuine Source-compatible game module.

**No original C++ recovery, complete client/server DLL implementation, playable Quest 3 APK or integration with a verified SourceVR engine ABI is claimed here.**
