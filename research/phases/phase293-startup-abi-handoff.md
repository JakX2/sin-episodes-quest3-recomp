# Phase 293: evidence-gated Windows startup ABI handoff

**Scope:** original startup slot-zero signatures only. No Windows runtime
implementation, no whole-method recovery, no new original-DLL Ghidra scan.

The latest research branch already has Phase 285's reviewed original PE32
return-cleanup evidence, and Phase 286's bounded census validator. This phase
connects their *existing sanitized output* to a fail-closed Windows startup
handoff without promoting SDK hypotheses to original facts.

Run locally against the **sanitized metadata** output from an authenticated
owner-original Phase 286 run:

```sh
python3 research/tools/build_startup_abi_handoff.py \
  --census /path/to/startup_factory_dependency_census.json \
  --out /path/to/startup_abi_runtime_handoff.json
python3 -m unittest discover -s research/tests \
  -p 'test_build_startup_abi_handoff.py' -v
```

The handoff pins the two reviewed original slot-zero entry addresses and
x86 return-immediate cleanup sizes: client `VClient011[0]` has `ret 12`
(three 32-bit stack arguments), and server `ServerGameDLL003[0]` has
`ret 16` (four 32-bit stack arguments). This is evidence for the **number
of stack-passed argument words**, not their types or whether a given word
is a factory callback. It is also not proof of complete method bodies.

Source SDK 2004 *hypotheses*, kept in separate named fields:

| Method | SDK 2004 candidate arguments | SDK first engine request |
| --- | --- | --- |
| `VClient011::Init` | `CreateInterfaceFn appSystemFactory`, `CreateInterfaceFn physicsFactory`, `CGlobalVarsBase* pGlobals` | `VEngineClient012` |
| `ServerGameDLL003::DLLInit` | `CreateInterfaceFn engineFactory`, `CreateInterfaceFn physicsFactory`, `CreateInterfaceFn fileSystemFactory`, `CGlobalVars* pGlobals` | `VEngineServer021` |

These argument *types*, the actual factory invocation order, and the
first failing conditional branch remain **unverified in original SiN**.
A direct Ghidra string reference, even when matching the SDK candidate,
is **not** an observed runtime factory call. The handoff reports this
distinction explicitly.

The converter accepts only the two pinned original method entries and
sanitized bounded version-name candidates, rejects any upstream claim
that original argument types, full bodies, or first failure branches
were verified, and never reads original binaries or private pseudocode.
Synthetic tests verify rejection of malformed, overclaimed, or unsafe
input. Do not publish private original disassembly or owner-only C text.

**Next useful original-private extraction:** from the SHA256-authenticated
original method, verify which register/stack argument supplies the
first indirect factory call, the exact interface-name pointer, and
the immediately dependent failure edge. Review only these two entries
and record explicit evidence tier. No additional full atlas scan needed.
