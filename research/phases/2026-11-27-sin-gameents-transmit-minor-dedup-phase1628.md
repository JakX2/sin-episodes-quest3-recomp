# Phase 1628: Canonical Minor +0x398 and composed ServerGameEnts001 transmit entry

## Scope

This phase fixes a verified decomplication pitfall and connects an existing original-guided network policy to the authenticated `ServerGameEnts001::CheckTransmit` entry. It does **not** claim two new original functions: Phase 1613 and Phase 1625 independently implemented the **same** retail 56-byte Recombinant Minor virtual `+0x398` at VA `0x2237e8c0`, SHA-256 `e08a5b91b9f7e3411976303123a218fb28e6d3ff2df0690a1e8df6525b714ff7`. That method counts once in any coverage measure.

### Canonical +0x398

The original method first calls the process-global service at `0x22572198` virtual `+0x170`; when its result is true, the method returns false without accessing the candidate at all. Otherwise it reads the candidate float at `+0x2c` and uses x87 strict-greater comparison with `10.0f` (`0x223d8f14`). **IEEE positive infinity is accepted**, while negative infinity and unordered NaN fail. The Phase 1613 `native_server_recombinant_minor_damage_gate.cpp` originally imposed `std::isfinite`, falsely rejecting positive infinity. This phase corrects that discrepancy.

`SiN_GetServerRecombinantMinorDamageGateApi` is now the sole implementation of the original method's behavior. Phase 1625's later `SiN_GetServerRecombinantMinorMetricGateApi` remains an ABI-preserving compatibility entrypoint: it adapts its binding/result struct to the original Phase 1613 export, rather than duplicating the predicate. Cross-export Windows regression verifies valid/invalid actor, blocked manager, failed callback, finite/equality values, positive/negative infinity, NaN and instrumentation counters. Neither public export's struct or function signature changes.

### Original `ServerGameEnts001::CheckTransmit` entry and existing core

Original `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Original vtable `0x22443030` slot `+0x18` points to `0x221e94d0` (354 bytes, SHA-256 `e069400e844edfe9c5e756bfd828abfc6764df7b37ce13dfca374f94c25ad176`). Phase 1627 already authenticated the method, its table-query first, recipient/HLTV snapshot and mask/bitset handling. There is also already an independently authored `server_transmit_policy_core.h` that implements the original-guided candidate loop with explicit Source-owned callbacks. This phase does **not** write a second candidate decision loop.

New `server_gameents_transmit_entry_core.h` composes four explicit operations:

1. Query the engine's original global table manager (`0x22572198`, virtual `+0x4c` with argument 0) to get the candidate table. The owner exposes bounded `TransmitEntry` metadata; the native bridge never reads undocumented engine memory directly.
2. Resolve a recipient from the original CCheckTransmitInfo (including fallback when appropriate) and snapshot its `+0x828` skybox/area property and `+0x92c` special-recipient flag. The host supplies typed `TransmitRecipient` metadata, rather than a guessed CCheckTransmitInfo layout.
3. If the signed candidate count is not positive, return with no bitset access or per-entity calls. Otherwise, obtain the source-owned selected and always bitsets corresponding to original context `+0x2008/+0x200c`.
4. Delegate the *entire* indexed candidate loop to the preexisting `CheckTransmit` policy with its real ShouldTransmit/area/PVS/SetTransmit service adapters. That core preflights candidate bounds and bitset extents before mutation, a deliberate synthetic safety departure from trusted retail memory.

Original-to-native callback failure is explicitly represented as `GameEntsTransmitEntryStatus::HostFailure`; the retail method itself has a void return. Original count/array semantics and candidate order are retained without requiring the engine to accept a partially implemented virtual game interface.

`test_server_gameents_transmit_entry_core.cpp` covers table/recipient/bitset request order, direct Always mode, FullCheck ShouldTransmit, PVS vs skybox, HLTV always-bit behavior, repeated selected indices, snapshot behavior when a virtual callback mutates live recipient state, signed zero/negative counts, missing recipient and callback failures, malformed candidate extent and all-before-mutation validation. The new Windows CTest target is `independent_server_gameents_transmit_entry_native`. Expected CTest executable count grows from 235 to 236, rather than claiming 236 original recovered methods.

## Limits

`ServerGameEnts001` is **not exposed** to the real Source engine in this change. The original interface's engine-owned entity table lifetime, recipient and networkable retrieval, global contact state, entity removal, and real network send effects remain unintegrated. The Win32 binary loader and Android Bionic smoke checks verify native behavior in synthetic hosts, **not** playable SiN startup on Windows, Android or Quest. No proprietary executable bytes, Source physics, surface/material, prop, model-loading or rendering code is committed.
