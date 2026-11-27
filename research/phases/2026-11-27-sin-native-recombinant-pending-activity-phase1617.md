# Phase 1617: Native recombinant pending activity lifecycle

Connect existing shared pending activity setter (22049c20) and deferred update (22049da0 / 22049ce0) to the Phase 1616 synchronous Major/Minor activity assignment composition. No new original code regions or functions claimed. The previous activity stays visible to the original class-specific +4e4 override from Phase 1612.

New separate API wraps the existing pending lifecycle with its nested assignment callbacks bound to native mutant transition policy. Pending storage, metadata, sequence choice, forward-only node traversal, intermediate activity 2, sequence completion and original component callback ownership remain in the existing implementation. Only the assignment's class callback is replaced. Both existing public APIs and their layouts remain intact. The call-local binding cannot be retained by consumers.

Loaded Windows regression extends the existing pending transition fixture: Minor request remains pending without drawing a scalar, intermediate animation publishes activity 2 without a draw, unfinished animation skips, final activity 10 invokes the native scalar callback before publication, and another update does not repeat the draw. Rejected draw prevents new activity publication but does not roll back earlier sequence changes. Major direct transition from activity 27 to 29 clears the original latch before notification. Stale registration, wrong actor identity, insufficient extent and null bindings reject. These are native integration tests using host fixtures, not a retail engine execution claim.

No generic Source physics, surface, model or prop work; no original binary bytes included. CI pending.
