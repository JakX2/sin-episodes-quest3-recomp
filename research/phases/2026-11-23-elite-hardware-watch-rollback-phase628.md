# Phase628: Transactional per-thread DR0–DR3 watches for the owner-only original Elite debugger

## Why Phase627 required repair before a real game run

Phase627 introduced an independently compiled Windows x86 executable that launches a **disposable original SiN Episodes** copy and watches SHA-256-authenticated original `server.dll` Elite factory, Spawn, event and release entrypoints using read-only thread hardware breakpoints.

Inspection of the actual source uncovered a genuine cleanup failure: the previous `apply_all_threads` could successfully arm several threads, fail while arming a later thread, return `false`, and leave earlier breakpoints set. The caller only detached when `armed==true`, so its failed-arm result could still leave live debug registers installed on the original game.

The same bug existed for any new `CREATE_THREAD_DEBUG_EVENT` after arming: a failed thread-context change was logged but the event loop could continue. On timeout, the prior cleanup also attempted to detach **even when disarming failed**, potentially abandoning active DR0–DR3 execute watches in the original process. The original DLL might also unload while the debugger still held address-based hardware watches.

These are **debugger correctness issues**, not evidence about native Elite AI behavior. Phase628 repairs the instrument before relying on owner traces. The retail module and any game code remain unchanged.

## Implementation changes

- `update_owned_watch_registers` edits only hardware registers that have **no previously enabled breakpoints**. It rejects nonzero foreign DR slot addresses and existing DR7 local/global enable or RW/LEN fields instead of overwriting them. The original DR7 reserved/status settings are preserved.
- The disarm path removes only **our authenticated four-watch lease**: exact DR0–DR3 addresses, local-enable mask `0x55`, and no foreign breakpoint control. It returns an explicit failure for other observed DR state rather than erasing another component's debugger state.
- All original x86 Elite addresses remain the independently SHA-pinned phase627 RVAs (`0x355C70`, `0x355820`, `0x355480`, `0x3552C0`) and are checked against the loaded retail image base and live original code prefixes.
- A `arm_transaction` routine enumerates the target's game threads and rolls back **every attempted thread, including the thread whose arm call failed**. It distinguishes successful arming, clean rejection, and incomplete rollback. Interrupted ToolHelp enumeration aborts before arming; an interrupted cleanup enumeration cannot be silently counted as complete.
- The main debugger exits observation on a failed thread-arm, modified original hardware context, or failed register cleanup. On safe cleanup it detaches normally. If the disposable game still has an unremovable watch, the tool **terminates that explicitly owner-confirmed disposable test process instead of detaching with debug registers potentially active**. The resulting JSONL trace records the failure and cannot count as a successful callback sample.
- The original module's `UNLOAD_DLL_DEBUG_EVENT` now disarms all game threads before concluding the observation interval; failure aborts the trace. This prevents a stale original executable address from remaining armed after an original DLL unload.
- Trace success now requires an authenticated original server, at least one complete watch-arm, no missed threads, no abort, and successful process exit or debugger detach. **Zero observed Elite callbacks is a valid negative observation interval**, not proof Elite code is unreachable and not a playable-native-port milestone.

## Original-free regression testing

The existing Windows x86 executable's `--self-test` now tests the **same policy functions used in real debugging** against synthetic Win32 `CONTEXT` structures. It checks:

1. All four correct source RVAs are enabled with DR7 mask `0x55` and removed without altering the Windows DR7 reserved bit.
2. Foreign DR7 enables, previously used DR0 address, and nonzero breakpoint RW/LEN settings are refused.
3. A changed hardware watch address cannot be silently cleared as if it were ours.
4. A successful three-thread synthetic watch transaction arms all threads.
5. A synthetic second-thread arm failure, **after it partially installed a watch**, rolls back the first and failing threads, never touching unattempted threads.
6. A rollback failure is classified as `FailedUnclean`, not incorrectly reported clean.
7. Empty thread sets fail closed.
8. Existing fabricated-module `ReadProcessMemory` original-prefix mutation checks and command-line/PID observation tests still run.

No original game binary is used in public CI. All real runtime evidence remains owner-only.

## Validation and next step

Build and Windows x86 CTest remain under the existing `pc/windows_x86_proxy/CMakeLists.txt` phase627 target. The phase627 owner PowerShell launcher is unchanged; it picks up the corrected phase628 implementation from the stacked branch.

A **real disposable Windows game run is still necessary**. Until a complete owner's JSONL trace shows an authenticated original `elite_spawn` method-entry hit (or absence for a known tested map/time), there is no observed original-engine callback to attach to Phase626.

No 2006 gameplay method is newly reconstructed in this phase and no original-function or recovered-byte count is incremented. No physics, props, models, surfaces, or already-working HL2 rendering code was touched.
