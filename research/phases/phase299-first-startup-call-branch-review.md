# Phase 299: bounded original startup first-call / failure-edge candidate review

**Priority:** Windows startup ABI evidence, not another whole-binary catalog.

The new `ExportFirstStartupCallBranchCandidates.java` Ghidra post-script is
restricted to **two** SHA-reviewed original PE32 vtable entries:

- `client.dll` `VClient011[0]` at `0x240FE0E0` (reviewed `ret 12`).
- `server.dll` `ServerGameDLL003[0]` at `0x221E7640` (reviewed `ret 16`).

It requires `SIN_PRIVATE_DECOMP_ACK=1`, the previously authenticated
`*_vtable_method_seeds.csv` inputs, exact pinned entries, a Ghidra
function starting at the entry, and bounded body size/instruction count.
**The operator must first authenticate both full original DLL SHA256
values** with the existing owner-private recovery procedure; seed filenames
alone are not cryptographic authentication. The script intentionally
does not rerun whole-binary analysis or authenticate the binaries itself.

Example, as an **additional post-script only** in the already authenticated
owner-private Ghidra environment:

```text
-postScript ExportFirstStartupCallBranchCandidates.java /private/reviewed-seeds /private/first-call-review
```

It emits up to eight **computed-call candidates per startup method**, with
offsets relative to the reviewed entry. For each it records the nearest
preceding interface-version literal reference within 32 instructions and
the next conditional branch within 16 instructions, if present. The CSV
is private and metadata-only; no original instruction bytes, whole
disassembly, or proprietary pseudocode are exported.

**Interpretation:** proximity is *not* data-flow or control-dependence.
A literal may belong to another call; a conditional jump may not test the
factory result. These are review locations, **not verified factory calls
or first failure branches**. A missing candidate does not establish
absence of factory calls. The result must not be promoted to the runtime
ABI handoff until owner-private manual review verifies the actual
callback argument origin, requested interface-name pointer, and failure
edge, and both binaries' pinned full-file SHA256 authentication has been
reconfirmed. Source SDK 2004 types remain hypotheses.

This is intentionally separate from the main-only full Ghidra workflow.
Do not upload its owner-private CSV, original DLLs, or proprietary C text
to public Actions artifacts or GitHub. No Windows runtime files changed.
