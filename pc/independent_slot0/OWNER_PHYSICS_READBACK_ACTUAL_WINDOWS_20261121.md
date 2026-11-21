# Phase 519: Original Windows x86 VPhysics surface-table readback observed

## Private owner artifact, public metadata only

Owner-supplied TEXT-ONLY ZIP:
`SiN_FileSystem_Test_Results_20261121-022233-7689a6508b.zip`

SHA256: `d05b7ca2db2ed6f9e3f1b9d34a65ac94c60eee4d11bb55537c0d4a56fd1e7b81`

This report deliberately does not include the owner archive, private
installation path, original client/server DLL bytes, or proprietary
surfaceproperty source texts. Retain them privately.

The owner test ran through the original 2006 engine while injecting our
**independently compiled** 32-bit `client.dll` only into the
SHA-authenticated disposable test copy.

### Results from same client session

| Ordered stage | Observed trace | Interpretation |
|---|---|---|
| Original soundscape CVar lookup | `original-soundscape-both-vars-cached:2:2:1:1` | Both original ConVars found |
| Original physics interfaces | `original-physics-three-available:3:3:1` | All 3 original Factory queries returned |
| Original physics resources | `original-surfaceprops-retail-resource-shape-verified:2:2:93:71:0` | 2 source scripts, 93 definitions, 71 base references, 0 duplicates |
| Actual physics mutator | `original-surfaceprops-register-two-calls-returned:2:2:0:0` | Both original native VPhysicsSurfaceProps001::ParseSurfaceData slot1 calls returned; integer returns 0/0 |
| Original physics readback | `original-physics-surface-readback-three-in-range:94:0:1:85` | Original engine SurfacePropCount slot2=94; GetSurfaceIndex slot3: default=0, solidmetal=1, metalvehicle=85 |

The ZIP's active trace contains, strictly in that order, all five
three-event sequences (`entered`, `result`, `terminal-stop`).
`staged.txt` independently records every opt-in as enabled and
identifies the same active session for every original-physics result.
The real engine's diagnostic process exited `0x53494E76`;
`OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE`.
The package reports `COPY_VERIFIED_ORIGINALS` and
`VERIFIED_AND_REMOVED_DISPOSABLE_COPY`.

### Evidence boundary

**Verified**: the independently built client crossed the original
factory, filesystem and source-era physics virtual method ABIs without
the diagnostic terminating before readback; the original engine returned
a 94-entry surface database and three valid indices.

**Not verified**: that either ParseSurfaceData invocation inserted any
new entry. The values returned were `0:0`; they might mean zero new
entries, another engine-specific status, or successful no-op. We DID
NOT record SurfacePropCount/indices before the two calls, and must
not infer before/after delta from a post-call table alone.

**Not verified**: full `VClient011::Init`, full
`ServerGameDLL003::DLLInit`, playable map startup, independent
native Windows game, Android, or Quest 3 VR runtime. The code
intentionally returns a diagnostic terminal, not game init success.

### What Phase 519 implements

When BOTH existing `-SurfacePropsRegister` AND
`-SurfacePropsReadback` are explicitly requested, take a new
nonmutating original-engine slots2/3 `Query(surfaceProps)` snapshot
BEFORE the first ParseSurfaceData call. Emit:

- `engine-fs-preparse-entered` with
  `VPhysicsSurfaceProps001:slots2-3:before-parse`
- `engine-fs-preparse-result` with the same typed count/name-index
  status as the existing `engine-fs-readback-result`
- `engine-fs-preparse-terminal-stop` with
  `read-only-before-original-surface-parse`

The pre-existing after-Parse readback remains unchanged. Thus the
next owner experiment can compare a real before/after table in the SAME
engine process without confusing a prepopulated engine table with
independent registration. The same standalone one-click launcher
authenticates installed retail hashes, copies to a disposable directory,
and verifies originals are restored afterward.

No baseline snapshot runs when physics register/readback opt-ins are
absent. Querying slots2/3 is read-only; ParseSurfaceData slot1 remains
engine-mutating and opt-in, with possible engine-side crashes.
