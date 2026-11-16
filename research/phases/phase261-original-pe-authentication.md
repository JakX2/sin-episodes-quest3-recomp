# Phase 261: independently authenticate owner PE32 DLLs before bulk ABI triage

This stacked draft builds on Phase 260. The previous ABI triage only checked
SHA256 values **recorded in inventory metadata**. The new gate reads the complete
owner original client.dll and server.dll and computes SHA256 independently,
then checks PE32/i386/DLL structure. Both inputs must pass or no success report
is written. The tool never commits or emits game bytes.

Expected SHA256:
- client.dll: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- server.dll: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

Run on a trusted runner with **owner originals**, outside the repository:

```sh
python3 research/tools/verify_original_batch.py --original-dir /private/originals --out /private/authentication.json
python3 research/tools/triage_original_abi_atlas.py --original-dir /private/originals --inventory /private/inventory --ghidra /private/ghidra --out /private/abi-atlas.json
```

The optional `--original-dir` parameter enforces the independently rehashed
binary gate for triage. Without it, triage still works for synthetic CI and
unverified metadata, but the output explicitly marks original bytes as
**not rehashed**. The original PE batch inventory and Ghidra pipelines are
upstream prerequisites, not substitutes for this gate.

Synthetic CI never has the owner DLLs and therefore cannot establish retail
authenticity. Complete-file SHA256 authenticity is also **not** whole-body
function SHA verification, and an inferred call target or Ghidra function
start remains a candidate. No original code was newly verified or
independently load-tested in this phase.

Cumulative prior baseline: 403 reviewed whole-body SHA methods (256 client,
147 server), 64,586 entrypoint candidates, zero original functions
independently load-tested. Independent retail Windows x86 gameplay boot is
still unproven. Next: authenticate owner originals in hosted Ghidra, export
safe call graph/RTTI/vtable/interface metadata, then implement and load-test
non-forwarding engine-compatible gameplay interfaces.
