# Phase 489: original SiN 2006 C_SoundscapeSystem::Shutdown

## Why this is a real runtime boundary

Earlier Phase 482–488 stages reconstructed the original resource
manifest, soundscape graph, candidate wave paths and source-only
audio declarations. This phase authenticates an **actual original
2006 game-system virtual Shutdown** method and reconstructs its
ordered cleanup behavior. It does not call proprietary functions.

The owner-provided \`client.dll\` has complete-file SHA256
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

- Original soundscape system vtable VA: **0x24293C34**.
- Original vtable slot 0: \`Init\`, VA **0x240F4160**.
- Original vtable slot 1: \`Shutdown\`, VA **0x240F3EE0**.
- \`Shutdown\`: **267 bytes**, complete method SHA256
  **fd966f21a2da53fb0f4bf655c17e06fce64f4f2ed01c731ede98948dbe90d8f8**.

The original binary was examined *read-only in private*.
\`audit_sin_original_soundscape_shutdown.py --client\` or
\`--archive\` confirms both vtable slots, the exact entire method
hash and selected source instruction witnesses. CI uses
\`--self-test\`, with fabricated method bytes and corruption tests;
no original private DLL is included.

## Exact source-observed Shutdown control flow

The original loads the count of active sound records from
\`this+0xAC\`, then iterates from **last to first** over a
40-byte-entry array pointed to by \`this+0xA0\`.

| ABI data | Original field |
| --- | --- |
| Active sound record size | 0x28 bytes (40) |
| Original active entry vector | object+0xA0 |
| Original active count | object+0xAC |
| Sound identifier | entry+0x0C (DWORD) |
| Route flag | entry+0x24 (BYTE) |
| First stop route | virtual call through global interface at vtable byte offset 0x2C, five arguments, including the sound ID |
| Second stop route | direct function call at 0x241BD680, arguments include sound ID and integer 6 |
| Iterator movement | \`sub edi,0x28\`, reverse by one record |

The meaning of the flag and the external interface object are
not yet sufficiently established to describe them as universal
mixer or channel semantics. Therefore the independent output names
the **observed virtual/direct routes** rather than guessing their
ultimate audio behavior.

After processing all current sound entries the original:

1. Sets \`this+0xAC = 0\`, \`this+0xC0 = 0\`, and
   \`this+0x28 = 0\`. The +0x28 field is the count of the
   array at \`this+0x1C\` populated by source helper
   \`0x240F4070\`, **not** the resource-pointer collection at +0x08.
2. Considers packed handle fields at \`this+0x9C\` and
   \`this+0x94\` for conditional invalidation. The original
   checks sentinel \`-1\`, generation bits and handle-table
   state, then invokes a vtable method on a subobject at
   \`this+0x30\` if the handle is live.
3. Iteratively removes and deletes the **first pointer** from
   the KeyValues collection at \`this+0x08\`, count at
   \`this+0x14\`. It shifts later pointers left with memmove
   and invokes helper \`0x24166050\` to release each KeyValues
   instance. This yields original **first-in, first-out**
   resource deletion order.

These operations are authenticated with exact branch/opcode
witnesses, original relative CALL targets and the complete
Shutdown method SHA, rather than Source 2013 documentation.

## Independently owned reimplementation

\`client_soundscape_original_shutdown_owned.h\` supplies:

- A 40-byte candidate layout with compile-time assertions for
  original ID/route offsets, without dereferencing a real game
  sound record.
- An \`OwnedLifecycle\` operating only on independent sound IDs,
  route bits, synthetic KeyValues owner labels and candidate
  integers for handles.
- \`Prepare(Plan&)\`, producing **ordered events**: reverse
  routed sound stops, the three object count clears, conditional
  handle candidates, and FIFO resource releases.
- \`CommitCleared(Plan)\` for independent cleanup, only when
  the plan's revision, event count and **complete event identities
  and order** still match the owned state.
- Atomic refusal on failed installation, overcapacity state or
  malformed resources. The independently chosen limits are
  4,096 active sound records and 256 source resource labels.

The simulated stop events DO NOT call original or engine sound
interfaces. The conditional packed EHANDLE cleanup is modeled as
a candidate event, not as a claim that any non-minus-one handle
must pass the original generation/table validity checks.

The independent event plan describes the original sequence as a
basis for future source-compatible engine calls but does not
conflate event generation with performing source audio side effects.

Native
\`test_client_soundscape_original_shutdown_owned.cpp\` covers
the exact 40-byte record/offset layout, two sound-stop routes in
reverse order, three zero-reset events, two candidate-handle
events, FIFO KeyValues delete events, repeat initialization,
stale-plan refusal, forged route/identity refusal, capacity
refusal and clean restart.

## Incomplete for Windows/Android/Quest 3

This is NOT the original \`C_SoundscapeSystem\` instance, no
engine \`ISoundEmitterSystem\` vtable is invoked, no real sound
is stopped, and no original KeyValues object is deleted.
The remaining C_SoundscapeSystem startup, trigger selection,
sound transition, precaching, update and event callback behavior
still needs reconstruction and real engine ABI validation.

Normal independent \`VClient011::Init\` still returns false.
The last **observed** original Windows retail checkpoint remains
\`0x53494E74\`, while opt-in \`0x53494E76\` has not yet been
observed in the owner's retail game. CI native synthetic tests
do not establish playable Windows, Android or Quest 3 builds.
