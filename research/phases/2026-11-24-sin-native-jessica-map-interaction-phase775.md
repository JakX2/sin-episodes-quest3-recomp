# Phase 775: Jessica map-host interaction callback boundary

Connect the authenticated Phase774 fixed interaction routine to the Phase772 NPC map host. Optional server API preserves previous constructor behavior. The host routes only active Jessica entities with matching generation; unsupported kinds and missing optional API are unhandled, stale, preactivation and blocked calls rejected. Original AL truth value is independent of map status. Host callback installation still belongs to the real engine, not an address copied from the original DLL.

Loaded synthetic BSP fixture now installs a callable adapter at Jessica Spawn +80 boundary (original thunk 22361e40), verifies preactivation rejection, invokes after activation, checks HELLO/IDLE and USE branches plus original false on successful USE, stale handles, failure/ack and post-unload rejection. This is engine-host adapter coverage, not retail callback installation or a running campaign. No physics/models/rendering.
