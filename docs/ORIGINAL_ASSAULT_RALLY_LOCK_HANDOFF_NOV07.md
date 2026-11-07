# SiN 2006 AssaultBehavior: rally ownership handoff and fail-closed host binding

**7 November 2026 | Original Windows x86 server.dll and new C++17 Quest-port reference**

Original hash: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.

## New machine-code finding

At `0x220CFE43`, original `CAI_AssaultBehavior::SetParameters`
pushes its owning NPC and invokes the selected rallypoint ownership
method at **`0x220CFE49`** (direct call to `0x220CE430`).
The **immediately following instructions** are:

| Original address | Bytes | Observed operation |
| --- | --- | --- |
| `0x220CFE49` | `E8 E2 E5 FF FF` | Call rally ownership acquisition, returning success in AL |
| `0x220CFE4E` | `8B 13 8B CB FF 52 08` | Read selected rallypoint vtable and call slot `+8` to get the entity handle; **no AL test** |
| `0x220CFE55` | `8B 00 83 F8 FF 89 47 18` | Read/cross-check handle and cache at behavior `+0x18` |

The original lock routine independently verifies at `0x220CE433`
through `0x220CE463` that **any live owner causes acquisition to
return false**, including a same-owner retry. Unlocked or expired
handles may be replaced at `0x220CE475`, and a null requester clears
ownership at `0x220CE481`.

The native candidate ranking at `0x220CFC70` already filters points
whose original `m_hLockedBy +0x3BC` resolves to a live entity.
Nevertheless, the **post-ranking lock return is not checked**. This is
a narrow, instruction-backed observation and does not establish how
often the edge case can happen in the original single-threaded game.

The original release routine `0x220CE4A0` rejects unlock requests
when the owner is a **different live entity**; otherwise it clears the
owner handle. Its terminal assault-chain call is at `0x220CE70F`.
These have earlier original-binary verifier coverage in
`audit_sin_rally_lock_assault_link.py`.

## New host binding

`include/sin_compat/assault_rally_binding.hpp` composes the existing:

- `choose_mobgrunt_rally_point`: original highest priority, nearest
  squared 3D distance tie-break; retains list order on exact ties.
- `RallyPointLease`: original same-owner and other-owner refusal,
  generation-safe host owner tokens and expiry handling.
- `apply_original_setparameters_cue_effects`: original no-candidate,
  unresolved-handle, and resolved-success cue-state transitions.

The **new** binding operation accepts a host liveness predicate and
an engine-dependent entity-resolution callback. It excludes currently
owned points before ranking, *rechecks* ownership when acquiring the
chosen point, and refuses to proceed if the lease fails.

If the host fails to resolve the selected entity **after** a lease was
acquired, it releases the newly acquired lease to avoid carrying a
dangling reservation, then retains the source-proven invalid-handle
cue effect (configured cue becomes `0`, received cue is unchanged).
Rollback and explicit `LockRejected` are **port-side safeguards**;
the original caller is not proven to perform them. The helper does
not retry/rerank on a failed lock, and the caller must handle that
result explicitly. This test harness assumes the host does not
re-enter or throw from these callbacks.

### Host result meanings

| Result | Cue fields | Ownership |
| --- | --- | --- |
| `NoCandidate` | unchanged | unchanged |
| `LockRejected` | unchanged | original owner retained |
| `UnresolvedRally` | configured mode `0`, received cue unchanged | newly acquired lease released |
| `Bound` | configured mode requested, received cue reset `0` | new owner lease retained |

**Limits:** neither this code nor previous research constructs an
original `npc_sintek_grunt`, replaces engine `EHANDLE`, drives
native SourceVR AI, invokes an Android/Quest loader, implements
pathfinding, or restores the commercial game. Real adapter callbacks
must use engine-safe entity identifiers and perform the other
original `0x220CE960` state resets separately. No commercial DLL
or game asset is committed.

## Reproducible checks

```bash
python3 research/tools/audit_sin_rally_lock_assault_link.py \
  --server /path/to/original/server.dll
python3 research/tools/audit_sin_assault_setparameters_outcomes.py \
  --server /path/to/original/server.dll
python3 -m unittest discover -s research/tests \
  -p 'test_audit_sin_assault_setparameters_outcomes.py' -v
cmake -S . -B build && cmake --build build
ctest --test-dir build -R sin_assault_rally_binding_tests --output-on-failure
```

The expanded SetParameters verifier now checks **27** short native
instruction witnesses, five branch destinations, three call edges,
and the diagnostic class name. The new Python regression asserts
that the post-lock handler sequence has *not* changed to test AL.
The portable C++ regression checks rank ordering with occupancy,
same-owner retry, stale-owner takeover, two failure paths,
rollback, and a lock-state change between ranking and acquisition.
