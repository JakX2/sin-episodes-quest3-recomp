# Phase 262: make the Ghidra-to-PE ABI triage executable

Stacked on the Phase 261 original-PE authentication draft. This phase fixes
two integration defects found by reading the actual scripts, not new recovered
retail methods.

- Ghidra 12.2 Java `Address.toString()` exports **bare hexadecimal** entries
  such as `10002000`, while the Phase 252 PE inventory uses `0x10002000`.
  The Ghidra CSV parser now accepts both forms as 32-bit hex without weakening
  the inventory's explicit `0x` format.
- The CLI summary previously iterated over `_authentication` as if it were
  a client/server module. That caused a `KeyError` *after writing* a JSON
  report, making automation appear to fail. Console summaries now include
  only the two actual modules.
- Synthetic regressions verify the real subprocess CLI, both address formats,
  strict inventory parsing, invalid addresses, output JSON, and the fact that
  Ghidra matches never become original-function verification.

To run synthetic validation:

```sh
python3 -m unittest discover -s research/tests -p 'test_triage_original_abi_atlas.py' -v
```

Trusted-owner input workflow (private only):

```sh
python3 research/tools/triage_original_abi_atlas.py \
  --original-dir /private/owner-originals \
  --inventory /private/original-pe-inventory \
  --ghidra /private/ghidra-atlas \
  --out /private/abi-priority.json
```

A full-file SHA256 gate proves the input files' identity, not the correctness
of decompiled C++ or compatibility with the original Source engine. Review
and manually validate interface layouts, vtables and callbacks before using
any candidates for a non-forwarding Windows PE32 game-module replacement.

Evidence unchanged: 403 previously whole-body-SHA-reviewed methods; 64,586
inventory candidates; 24,431 Ghidra-inferred entries in the known successful
run; zero original methods independently load-tested. Retail Windows gameplay
boot remains unproven. No original DLLs, disassembly or private binaries are
committed.
