# Phase618: Reattest original Grunt native identity on every registry use

The Phase614 session registry already requires an independently constructed, host-confirmed native NPC at initial attachment. However, the registry only compared the stored generation on subsequent steps/snapshots, and delegated liveness to behavior callbacks. This leaves room for an incorrect host callback or recycled slot to drive stale reconstructed behavior.

Phase618 makes the **native host confirmation a continuing precondition**. For each `step` and `snapshot`, under the existing non-reentrant guard, the registry calls `confirmed_existing_native_npc` with the **canonical original factory classname**, the full engine-owned `(index,generation)`, and the **same opaque owner token recorded at attachment**. Caller-provided classname storage is not retained; the slot owns a view of a static literal. The host is still responsible for authoritative verification of these values against its real entity registry.

- If host confirmation returns false, `step` refuses to execute the behavior and reports `HostRejected` with underlying ABI `ACTOR_EXPIRED`; `snapshot` refuses to return a stale state and leaves its output unchanged.
- If host confirmation throws, `step` refuses with underlying ABI `HOST_FAILED`. No exception crosses the registry's `noexcept` entrypoints.
- Stale generation is rejected before host callbacks. Callback-driven reentrancy still fails closed.
- **Detach remains available after death or replacement** if the old full generation matches. It only deletes an independent session, never an original engine entity.

Regression tests deliberately change a simulated native identity's owner token and classname after attachment, raise an exception in the host confirmation callback, verify no behavior side effects occur, then restore identity and confirm normal stepping resumes. This is **native integration safety code, not recovered 2006 original function logic**, so the Phase617 original method-byte recovery count remains 43 / 11,233.

The real production host still needs a verified SiN/Source engine-created Grunt and working NPC callback adapter. This phase adds neither a Source vtable hook nor a synthetic NPC factory. It does not duplicate Half-Life 2 physics, materials, models or props.
