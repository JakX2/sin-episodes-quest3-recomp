# Phase 1625: Authenticated Recombinant Minor +0x398 global veto and scalar threshold

Direct, read-only analysis of the user's original `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Original Recombinant Minor vtable at VA `0x224a2974`, slot `+0x398` (stored pointer VA `0x224a2d0c`), contains `0x2237e8c0`. Function occupies `0x38` (56) bytes; SHA-256 `e08a5b91b9f7e3411976303123a218fb28e6d3ff2df0690a1e8df6525b714ff7`. The inherited base class's +0x398 implementation is `0x22044730` and Major inherits it.

The original method does **not** inspect the Minor actor's ordinary fields. First it retrieves the original process-global owner from `[0x22572198]` and invokes that object's virtual `+0x170`, expecting a boolean AL. If nonzero, it returns false immediately, without dereferencing its explicit argument. If zero, it reads a float at `[argument +0x2c]`, compares via x87 `fcomp` with 10.0f from `[0x223d8f14]` (IEEE754 `0x41200000`) and returns true only for `value > 10.0`. Equality, below-threshold and unordered NaN all return false. Positive infinity returns true. The original calling convention ends with `ret 4`.

The underlying semantic identity of the input float and of the singleton's +0x170 method is **not** established by these bytes. The native port therefore exposes a neutral `SiN_GetServerRecombinantMinorMetricGateApi` and names the injected owner callback `manager_suppresses`, without inventing combat, morphing or visibility semantics. The independent wrapper uses `int` as call-success and `result.allowed` as the original method's boolean; this distinguishes host callback failure from a valid false outcome. The engine retains ownership of its process-global manager. The caller must attest that its candidate memory has at least 0x30 bytes; no dereference occurs until after the global veto.

Loaded Windows fixture covers true and false owner responses, strict >10 threshold at equality and both sides, NaN and +/- infinity, short-circuit with null candidate, invalid/undersized candidate, provider failure, wrong actor class, truncated actor, missing manager callback and version/size export guards. No new CTest target is added; the existing `independent_server_recombinant_minor_native` executable gains cases.

No original executable bytes or proprietary DLLs committed. No generic Source physics, surface/material, model/prop loading or rendering implementation. A passing synthetic DLL load is not proof of retail-engine boot or Android/Quest 3 gameplay.

## Offline original evidence auditor

`python research/tools/audit_sin_original_recombinant_minor_metric_gate.py --self-test` validates the PE32 VA mapper and synthetic corruption rejection without needing original commercial content. With the owner-local original present, use `python research/tools/audit_sin_original_recombinant_minor_metric_gate.py --server /path/to/server.dll --self-test` to authenticate the exact file SHA, vtable slot, complete method hash, original threshold, critical instruction witnesses and targeted corruption rejection. Output is metadata JSON, never raw executable bytes.

The original-byte witness offsets and SHA were independently checked against the locally extracted SHA-matching owner original before this verifier was committed. The newly created auditor itself still requires normal execution validation.

## Authenticated dispatch call sites and inherited behavior

The BaseMutant vtable `0x2249f65c` and Major vtable `0x224a0d8c` both resolve slot `+0x398` to inherited `0x22044730`, an exact 5-byte `xor al,al; ret 4` implementation. The Minor at `0x224a2974` uniquely overrides the slot with `0x2237e8c0`.

Three original direct virtual `+0x398` dispatch sites were identified in `.text`: `0x22042f15`, `0x222a7a17`, and `0x222ac311`, each using the identical x86 `call [eax+0x398]` form. At `0x22042f15`, a shared NPC routine calls the method with the current damage/info pointer only after actor flag `+0xbd & 0x20`; its false result clears actor field `+0x7c` before subsequent handling. At `0x222a7a17`, another shared routine branches on the returned boolean. At `0x222ac311`, the boolean is passed directly to virtual `+0x3fc`. This confirms **real original virtual use** beyond a registered vtable entry; it does not by itself establish the manager's semantic purpose or an official C++ symbol name.

The offline auditor now additionally pins these three call-site instructions and the base/Major inherited false stub. Its corruption test also checks that changing an original call-site witness is rejected.
