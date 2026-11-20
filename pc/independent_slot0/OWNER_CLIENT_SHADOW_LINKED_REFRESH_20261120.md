# Phase 491: original SHA-authenticated linked ShadowMgr refresh

## Exact original binary evidence

This phase follows the recovered CClientShadowMgr::Init virtual
setters of Phase 490, which both reference common helper
**0x24115E80**.

Owner's original 2006 \`client.dll\`:
SHA256 \`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d\`.

The original refresh helper is **125 bytes** long, covering
0x24115E80 through 0x24115EFC, complete method SHA256
\`64b3d9cf23200b29329f83836b2507c243dfe9b49ac122aa86a2a5af2973dc66\`.

The exact full original method and selected opcode witnesses are
verified read-only by \`audit_sin_original_shadow_refresh.py\` in
its private \`--client\`/\`--archive\` mode. The public \`--self-test\`
fabricates those witnesses and requires damaged samples to fail.
No original DLL/method bytes or renderer resources are committed.

## Verified source-observed linked-record layout

The original helper:

1. Reads the **low 16 bits** of the head field at
   \`CClientShadowMgr+0x2C\`, using **0xFFFF** as end-of-chain.
2. Sets the object's byte \`+0xCE\` to **1**.
3. For each head-to-tail linked entry, computes
   \`entry = [this+0x20] + index * 120\`; the original
   assembly performs \`((index << 4) - index) * 8\`
   to obtain 0x78-byte entry stride.
4. Tests byte bitmask \`0x01\` at **entry+0x08**:
   - When set, reads a **16-bit** external ID at entry+0x04,
     invokes a method at external interface virtual byte
     offset **+0x38** (slot 14), then calls helper
     **0x24113EA0**.
   - Otherwise, calls alternate helper **0x24113F60**.
5. Reads the next **16-bit** linked index at
   **entry+0x76**; repeat until 0xFFFF.
6. Calls common helper **0x24114A90** on the object
   substructure at **this+0xDC**, then clears byte
   **this+0xCE = 0** and returns.

The original method has no source-observed cycle detection.
Our independent implementation must refuse malformed loops
rather than running indefinitely.

This identifies **control flow and source field layout** but
does **not** yet prove the original external virtual method's
canonical Source SDK name, nor the full renderer side effects
of helpers 0x24113EA0 / 0x24113F60 / 0x24114A90.

## New independently authored implementation

\`client_shadow_original_refresh_owned.h\` uses an owned
\`Entry\` with only a next index, external ID and branch bit.
A compile-time-checked **120-byte candidate entry** records the
verified original offsets at +0x04, +0x08 and +0x76.

\`OwnedRefresh::Prepare\` builds a complete read-only event
plan, including guard entry, each individual source branch in
**linked traversal order**, the optional engine slot14 event,
finalize and guard clear. It refuses invalid head/next indices,
cycles, excessive entries or allocation failure without
publishing a partial plan.

\`OwnedRefresh::SimulateComplete\` verifies the plan's revision,
all counts, and every ordered action including record indices,
external handles and source branch identity. Only then does it
record a simulated completed refresh. It **does not execute**
the engine's virtual slot14 or any original helper.

Conservative independent maximum: **4,096 owned entries**.
That is NOT an original engine limit. Separate original 16-bit
slot IDs may have larger theoretical representable ranges,
but large malformed inputs must not be blindly trusted.

Native Win32 regression
\`test_client_shadow_original_refresh_owned.cpp\` checks the
source 120-byte field layout, 0xFFFF sentinel, deliberate
out-of-order linked traversal, active/inactive branch event
ordering, unreachable entries, empty lists, forged callback
events, stale plan revision refusal, cycles, out-of-range
pointers, capacity refusal and safe cleanup. No proprietary
shadow renderer or DLL is invoked.

## Remaining engine work

Phase 490's original setters request this refresh, but this
phase **does not call original renderer methods** or modify
external game memory. The callback at 0x24115E70 and actual
renderer/engine interfaces still need safe ABI validation.
CClientShadowMgr still remains incomplete in the 12-system
startup sequence, and normal \`VClient011::Init\` remains
fail-closed.

Last genuine owner-retail Windows checkpoint remains
\`0x53494E74\`; prospective read-only original soundscape
diagnostic \`0x53494E76\` is not yet owner-observed.
Windows and Android reference CI synthetic tests are not
playable Windows/Android/Quest 3 game ports.
