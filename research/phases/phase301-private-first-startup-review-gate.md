# Phase 301: validate first startup call candidate metadata before handoff

The Phase 299 private Ghidra script records **candidate sites**, not actual
factory callback identities, return-value checks or first failure branches.
The CSV can contain owner-private interface-name literals and relative
instruction offsets, and must stay off public GitHub and Actions artifacts.

This phase adds a strictly bounded **local-only** validation gate:

```sh
python3 research/tools/validate_private_first_startup_review.py \
  --module client.dll --private-csv /private/review/client.dll_first_factory_branch_candidates.csv
python3 research/tools/validate_private_first_startup_review.py \
  --module server.dll --private-csv /private/review/server.dll_first_factory_branch_candidates.csv
```

Only run this after the **separate** owner-private procedure has verified
both original PE32 files against their pinned full-file SHA256 hashes and
produced the Ghidra review files. This validator **does not authenticate**
original binaries, execute Ghidra, prove the function bodies or inspect
runtime engine calls. It checks an exact 10-column schema, two pinned
startup entry identities, maximum eight candidate rows, ordered ranks,
relative offset ranges and consistent literal/conditional proximity fields.
It fails closed without echoing malformed private data.

The JSON output contains counts only: total candidate sites, sites with a
nearby version literal, and sites with a nearby conditional. It always
sets `verified_factory_calls=0`, `verified_failure_edges=0` and
`original_sha256_authenticated_by_this_tool=false`. **Do not treat this
aggregate as verified original ABI evidence** or automatically copy it to
the Windows runtime task. A reviewer must inspect data flow in the
SHA-authenticated original methods to establish the callback's argument
origin, exact version-name pointer, result test, and branch target.

The existing evidence remains: original client first-method `ret 12`,
server first-method `ret 16`; SDK2004 factory argument types are
hypotheses. No original PE32 scan, proprietary C text, or Windows runtime
implementation was changed. Synthetic tests run in research CI.
