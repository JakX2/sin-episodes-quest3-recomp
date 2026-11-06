# SiN Episodes 2006: assault rally-point ownership and point-chain reconstruction

**November 6, 2026 | Research milestone**  
Input: the owner's original `server.dll` from `bin.zip`, SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
The audit checks exact x86 instructions and call targets in the unmodified PE32 DLL.
No binary executable code or original game assets are distributed in this repository.

## 1. Original rally-point ownership is a real game subsystem

The original `CAssaultRallyPoint` data map stores `m_hLockedBy` at x86 object offset `+0x3BC`.
Two original native methods now have independently verified, complementary semantics:

| VA in original 2006 `server.dll` | Reconstructed function behavior |
|---|---|
| `0x220CE430` | Try to acquire a rally point for a supplied NPC entity |
| `0x220CE4A0` | Release the rally point only if unowned, expired or owned by the supplied NPC |

The acquisition function reads the existing `+0x3BC` Source entity handle, resolves its
index/serial against the live entity list, and returns false when an existing owner is still
alive. This is true **even if the requester is that same owner**, since the branch rejects
any valid current owner. If ownership is empty or expired, it fetches the requester's
Source entity handle via a virtual call and stores it in `+0x3BC`; a null requester
clears the field to `0xFFFFFFFF`.

The release routine checks the same current handle, and **refuses to clear** when the
live owner pointer differs from the requested owner. Otherwise it clears the handle to
`0xFFFFFFFF` and returns true.

This is especially relevant when multiple original MobGrunts share the same rallypoint
name in a scripted rooftop battle. A point assigned to a living NPC cannot simply
be taken by another soldier.

### Readable reconstruction, not verbatim original source

```cpp
bool TryLockRallyPoint(Handle requester) {
    if (IsLiveEntity(m_hLockedBy)) return false;
    m_hLockedBy = requester;
    return true;
}
bool TryUnlockRallyPoint(Handle requester) {
    if (IsLiveEntity(m_hLockedBy) && m_hLockedBy != requester) return false;
    m_hLockedBy = INVALID_HANDLE;
    return true;
}
```

The engine-neutral C++17 `include/sin_compat/rally_point_lease.hpp` models this
protocol with `std::optional<uint64_t>` tokens and a **host-provided liveness check**.
It cannot resolve Source's original 32-bit entity handle format or 64-bit Quest
entity objects by itself. `tests/test_rally_point_lease.cpp` checks same-owner lock
refusal, different-owner exclusion, valid-owner release, stale-handle replacement,
entity-index reuse with distinct generations, null-owner release, and empty-state cases.

## 2. Original assault initialization links the selected rally to its assaultpoint

`CAI_AssaultBehavior::SetParameters` at original `0x220CFC00` previously
was shown selecting the highest-priority eligible rally point, using the nearer
squared 3D distance for ties. Its success path calls the lock function from
`0x220CFE49`, stores the selected rallypoint handle in the behavior object at
`+0x18`, and calls state initialization function **`0x220CE960`** from
`0x220CFEDD`.

The state initializer clears flags at `+0x24`, `+0x25`, `+0x26`, integer state
`+0x20`, and timestamp `+0x28`, then resolves a point name from the
original rallypoint's **`m_AssaultPointName`** (`+0x394`).
It calls the engine's name lookup at **`0x223A5BAB`** from `0x220CE9D1`.
When found, the assaultpoint handle is stored at behavior **`+0x14`**.
When unresolved, an original game message reports the missing assaultpoint name.

The exact role of every reset flag and the NPC schedule that consumes them
still needs decompilation. These x86 offsets must **not** be copied into a new
ARM64 class layout.

## 3. NEW: linked next-assaultpoint progression and terminal lock release

The original progression function at **`0x220CE5B0`** resolves the current
assaultpoint using the behavior `+0x14` handle. It reads that assaultpoint's
original **`m_NextAssaultPointName`** key at object **`+0x398`**.

- If the name is nonempty, it calls the same entity-name resolution engine
  function `0x223A5BAB` at **`0x220CE63F`**.
- When that lookup finds the next point, the method stores its handle at
  behavior `+0x14` at **`0x220CE654`** and resets arrival flag
  `+0x25` at **`0x220CE657`**.
- If the named next point is absent, the original binary references its own
  explicit diagnostic at `0x223F3B8C` reporting a missing next assault point.
  The fallback/schedule side effects are not yet fully characterized.
- If `m_NextAssaultPointName` is null, the routine takes a terminal path
  beginning at **`0x220CE6D7`**. It clears behavior state `+0x1C`, finds
  the selected rallypoint and calls **`0x220CE4A0`** from
  **`0x220CE70F`**, releasing the rally-point ownership.
  It makes additional engine calls whose precise event semantics require
  further study.

This is the real chain topology implied by the SDK:
`assault_rallypoint.assaultpoint` links the first point, and
`assault_assaultpoint.nextassaultpoint` links subsequent points.
The previous PR's `analyze_sin_assault_graph.py` checks those keys without
rewriting the user's map.

### Evidence chain

```text
InputAssault (CNPC_MobGrunt)              0x22357E50
 -> CAI_AssaultBehavior::SetParameters    0x220CFC00
    -> TryLockRallyPoint                  0x220CE430
    -> resolve first assaultpoint        0x220CE960
       -> entity name lookup             0x223A5BAB
 -> (schedule/arrival transition logic: still incomplete)
 -> next point resolver                  0x220CE5B0
    -> nextassaultpoint name +0x398
    -> entity name lookup                 0x223A5BAB
    -> on terminal link: unlock          0x220CE4A0
```

We have now verified the **link and ownership subgraph**; we have **not**
recovered all movement, pathing, squad scheduling, weapon firing, or exact
conditions that trigger the transition. No real Quest game-module entity
has been registered.

## Reproducible verification

The new standard-library script
`research/tools/audit_sin_rally_lock_assault_link.py` reads an exact
SHA-256-pinned original 2006 `server.dll`, verifies **31 separate x86 instruction
witnesses, 5 relative call targets, and both diagnostic strings**, and
outputs structured evidence. The owner's original DLL was checked directly in
the research environment. Tests create synthetic instruction witnesses,
including intentional corruption; no proprietary DLL bytes are committed.

```powershell
py research\tools\audit_sin_rally_lock_assault_link.py --server "D:\SiN\bin\server.dll" --output "D:\SiN_Test_Only\rally_lock_assault_link.json"
```

Use the actual local path to the owner's original 2006 `server.dll`.
The output file is created only when absent. This script never writes to
the game's DLL, maps, archive or material files.

CI: `research/tests/test_audit_sin_rally_lock_assault_link.py` and
`sin_rally_point_lease_tests` are included in the existing Python and
C++ test workflows. Successful cross-build proves the reference code
compiles for the configured target, **not** that stock SourceVR can load
custom native entity code.

## Next decompilation priorities

1. Identify the exact events/conditions that invoke progression function
   `0x220CE5B0`, especially arrival, timeout, contact, and diversion.
2. Trace later calls in the terminal path after unlocking to resolve the
   output/event behavior.
3. Implement a **separate engine-neutral assault route progression reference**
   once the state transition conditions are proven.
4. Rebuild the game-specific NPC and assault entity factories in a legally
   buildable ARM64 Source-compatible server module. The existing stock
   SourceVR app only supports content-only mods.
