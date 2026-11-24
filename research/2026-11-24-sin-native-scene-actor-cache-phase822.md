# Phase 822: Shared scene actor cache

Original server SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Disassembly: 221a2c20, 22114060, 221aaf50, 221a6330, 22113bd0, 221a4700. Vtables 2242dd3c/2242e89c share +33c/+340; +338 differs (221a2c50/221a5870).

Parsed actor list +14/count +20, scene +40c parsed word. Lazy EHANDLE cache at scene+3f8/count+404/data mirror+408 initialized to ffffffff; first matching choreography pointer maps to index. Valid cached actor with zero word at +8c bypasses name lookup. Missing, stale or flagged actors trigger original virtual +338 lookup; successful result replaces handle, failed result retains old cache. Invalid indices use diagnostic callback. Host retains serial validation, allocation and virtual name lookup; malformed storage is rejected rather than accessing invalid pointers. Tests exercise empty lazy cache, miss, cache hit, stale or flagged actor, failed fallback, and invalid index. Standalone native resolver export, not yet substituted in speech lifecycle. Phase 821 Windows far macro collision also fixed on parent. No real campaign test.
