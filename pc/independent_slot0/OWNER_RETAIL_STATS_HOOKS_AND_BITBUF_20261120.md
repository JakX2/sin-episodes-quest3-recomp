# Phase 475: genuine owner-retail Stats hooks and independent bit cursor

## Authenticated retail owner results, November 20, 2026

Private owner report: SiN_FileSystem_Test_Results_20261120-025708-0fc6c96503.zip
SHA256: d7e55c218ac660c7ada4a3dcbb84715b41059c53adcebd23ea054ff65c1e8c0f.
All eleven report items were inspected directly. No private original retail DLL,
sensitive path, raw game asset or per-session trace is committed to the repo.

| Phase | Original Windows engine exit | Grounded observation |
|---|---|---|
| Presence | 0x53494E68 | VFileSystem012 FileExists both resources |
| Read | 0x53494E6B | Open/Size(handle)/Read/Close and 62:8:156:66 |
| Owned Stats hooks | **0x53494E74** | Original rb/GAME and rt/null staged reads, 66-ID Stats catalog, independently owned user-message IDs **27:28:5** |

The last session emitted:
- engine-fs-staged-counts: 66:62:8:156:66
- engine-fs-staged-stage: 7:7
- engine-fs-staged-result: owned-staged-verified-not-engine-init
- engine-fs-hooks-ids: 27:28:5
- engine-fs-hooks-count: 3
- engine-fs-hooks-result: three-owned-hooks-bound-not-dispatched
- CLIENT_ENGINE_FS_HOOKS_TERMINAL_DIAGNOSTIC_NO_BOOT_PROOF.

All three process transcripts contained OWNER_INDEPENDENT_RESTORATION:
VERIFIED_COMPLETE; SUMMARY confirmed VERIFIED_AND_REMOVED_DISPOSABLE_COPY.
This is genuine original 2006 Windows engine evidence for original ABI
filesystem reads and independently owned hooks. The hooks were **NOT**
registered in original engine CUserMessages, no real game packet was
received, no normal VClient011 Init completed and no menu/map/game boot
was reached.

## Next reconstruction dependency: bit-accurate Source bf_read

The SHA-authenticated client Stats handlers call ReadUBitLong(16) for
their first stat ID field. The older independent InspectHeader function
was only valid on **byte-aligned** buffers. Public Valve Source SDK 2004
tier1/bitbuf.h reads from bit (cursor & 7) of byte (cursor >> 3) and
packs results least-significant-bit first. A real message cursor may
begin at a non-byte offset.

The new independently authored client_stats_bitbuf_cursor.h supports
explicit byte/bit lengths and an initial cursor position; valid bit
widths 1..32; a bounded 256 KiB packet limit; LSB-first reads; cursor
clamping and overflow on attempted raw overrun; and transactional
InspectBitHeader parsing that leaves a caller cursor untouched when
a message is refused. This is NOT a binary-layout-compatible original
bf_read object or an engine dispatcher.

The new inspector recognizes original user message IDs StaticStat=27
(static stat indices 0..54), DynamicStat=28 (dynamic 55..65) and
ResetHUD=5 (one-byte envelope). Remaining payload bits are opaque;
there is no guessed type-specific numeric/statistics decoding.

The native original-free synthetic MSVC x86 test covers bit offsets
0..8 including word boundaries, 32-bit reads, 16-bit ID bounds,
one-byte ResetHUD, incomplete input, 256 KiB maximum, invalid starting
offsets and copied independent listener cursors. The rebuilt game's
slot33 DispatchUserMessage remains intentionally fail-closed until the
original bf_read bridge and stat value decoder are genuinely rebuilt.

This bitcursor is SDK-grounded and tested against synthetic bitstreams,
not against original network traffic or retail game packet dispatch.

## Next tasks

1. Reverse engineer original StaticStat virtual per-type value handlers
   (record method +0x4c) and dynamic helper 0x241FB370.
2. Recover original 24-byte bf_read ABI from SHA-grounded original
   CUserMessages::DispatchUserMessage and Source SDK evidence; build
   a verified, side-effect-safe adapter before enabling real engine calls.
3. Implement bounded, type-specific independent stat value parsing and
   dispatch, then successful normal Windows client initialization.
4. Integrate server entities and network sender/receiver paths for
   independent PC gameplay. Android then Quest 3 are later phases.

The previous retail 0x53494E74 result does not need a duplicate owner
run simply to test synthetic bitcursor fixture work.
