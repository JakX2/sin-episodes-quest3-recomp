# Phase 263: automate the engine interface queue after Ghidra

The repository already has a working hosted Ghidra headless workflow. A
successful manual original PE32 analysis has been observed, but no GitHub
`schedule` event run was recorded as of 2026-11-16.

This phase adds `research/tools/build_ghidra_interface_queue.py`, a
metadata-only postprocessing step using Ghidra's function/caller CSVs and the
pre-reviewed original interface registration manifest. The outputs prioritize
actual original server/client engine integration anchors:

- server: `ServerGameDLL003`, `ServerGameClients003`, `ServerGameEnts001`
- client: `VClient011`, `VClientPrediction001`, `VClientEntityList003`
- engine API support: movement, game client exports, player info
- material proxies are ranked below the engine game startup path

For each factory it reports exact Ghidra inferred entrypoint overlap, inferred
signature, known direct caller/callee counts, and a bounded callee address list.
Scores are engineering order, **not** verification confidence or reconstruction
percent. A match does NOT prove a vtable or a working game module.

The existing hosted owner-original workflow now calls the generator after
Ghidra analysis and includes the JSON in the metadata-only artifact.
It skips queue generation with an explicit warning if the reviewed manifest
is missing from the current branch, instead of fabricating registry data.
The PR smoke tests only constructed inputs; no game bytes are in GitHub.

Locally/within the owner-runner metadata scratch directory:

```sh
python3 research/tools/build_ghidra_interface_queue.py \
  --ghidra-dir /private/ghidra \
  --manifest research/catalog/original_dll_interface_registrations.json \
  --out /private/ghidra/interface_factory_review_queue.json
python3 -m unittest discover -s research/tests -p test_build_ghidra_interface_queue.py -v
```

No increase in verified original methods (403 previous), no new reconstructed
vtable, no new real original engine boot. This primarily removes repetitive
manual CSV inspection and directs the next clean-room ABI implementation.
Recurring scheduled GitHub Ghidra runs require the workflow's configured
schedule to actually fire on `main`, which must be separately observed.
