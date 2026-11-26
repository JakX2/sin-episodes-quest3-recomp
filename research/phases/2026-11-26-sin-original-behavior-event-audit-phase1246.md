# Phase 1246: original Grunt behavior event evidence audit

Run locally with the owner-private archive:
```
python research/tools/audit_sin_events_phase1246.py --bin-zip /private/bin.zip
```

Executed the audit locally against the authenticated 2006 DLL pair. PASS: 21 spans, 21 boundary bytes, 60 vtable entries, 13 dispatch entries, 10 original constants and four reason strings. Eight mutation rejections passed (both full DLL identity mutations, code, boundary, actor virtual, dispatch, constant and reason evidence). Note 22041a60 is exactly 128 bytes and immediately followed by another function (next byte 83), not INT3 padding. No original DLL bytes are committed. This checks source provenance, not C++ equivalence or live in-game behavior. The preceding phase-specific Windows regressions remain separate.