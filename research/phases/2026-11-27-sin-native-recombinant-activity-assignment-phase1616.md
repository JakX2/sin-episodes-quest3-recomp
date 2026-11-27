# Phase 1616: Native recombinant activity assignment composition

This phase connects the existing reconstructed server activity assignment (original 220417d0 / 22049b50) with the previously authenticated Major and Minor +4e4 activity overrides (Phase 1612). It does not claim additional original function bytes or newly decompiled methods.

The existing assignment dispatches the activity-change callback only when the previous activity differs from the requested value, then publishes the new activity to +878 and notifies the component. The separate composed API selects the Major/Minor native override by the existing original class identity; Major also requires a valid generation-bound activity registration owned by the policy dictionary. The prior activity remains visible during the override, including the Major 27/29 latch logic. The synchronous per-call frame forwards top-level assignment callbacks to their original context while leaving translation, reset, dictionary and transition contexts unchanged. The older assignment and activity-transition ABIs remain unchanged. Other actor classes use their existing entry points.

The loaded Windows fixture checks the Major latch clear/write branches, unchanged-activity skip, frenzy sound/random/scalar sequence, Minor random scalar, rejected draw, skipped request, scripted activity request, wrong class/extent, stale registration and component failure after activity publication. The underlying sequence, view, weapon and component owners remain delegated. The wrapper is not a transaction and does not roll back earlier changes on callback failure.

No original binaries committed; no generic physics, surfaces, props or model loading work. No retail-engine startup or playable game claim. CI pending.
