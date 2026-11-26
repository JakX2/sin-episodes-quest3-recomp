# Phase 1368: Native Jetpack unnamed think dispatcher

Fresh original server.dll from Drive SE1/bin.zip SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.

Original ranges (end exclusive):
- 2214d6f0..2214d708 24 bytes SHA256 8a15498e3f1509746c042a5e3d29a9ba14e79e277b71cd22b753fa4598b962a7: null context installs +8.
- 22222c30..22222c9c 108 bytes f245d67593b46c7fcd441e17b389db2522f5dd014786663857ec0d4fdba23ed7: +b8 bit22 gate, default before named.
- 22222b90..22222c26 150 bytes aeda1bdea5a9d6108cc4e62057d15a4cee90554ce0dd77c2d85652d4195318d7: positive due tick <= globals +18, record min(tick*interval,curtime), clear due tick, dispatch, store fresh last tick, return inverse removal bit0.
- 2214d870..2214d953 227 bytes b5e264f11c79c17465d3f56c790ced3094e4585d1ca8d52d878d93bd5eb88b40: negative index writes default tick +74, calls 2214bf30 twice; +c0=22001740 RET on all four.
- 221658e0..221658e8 8 bytes 35f1f9ad76523dd965da7f8072e270f991fcf55a6b318a680b51798d9f88b1cd: +bc dispatch; all four +bc=22002ca0, ten bytes 0c459ceb12ba13c47ce1349aa8fef3445670c88fa3cf44d310ba8c66f26a4920, reads +8 and tail-jumps if nonzero.
- 2221a0d3..2221a0f5 34 bytes ba81d574b7efcf700e25f9741cb2d7bb4fad3c981b0bb9a41ce99efdbb42ecfc: engine enter/leave around callback.
- 2214bfa0..2214c004 100 bytes 7d381720ef45aa75f99ca2273d3524b4832a618060968171cfcdf6274d4b3cb1: default last think +34 via time-to-tick, sentinel -1.
- 221c0d40..221c0d79 57 bytes 07f35ebb311076475ecc55737b7e8a84a20380d13c163dfeaa6672c342f75bc7: positive health correction (+1b4 RET on all four), then 22286bf0 removal.

Add server-only primary think API for four Jetpack tables and the two authenticated death identities; zero identity is no-op. Native default tick, flags, death-think install/reschedule, last-think tick and positive health correction. Host retains global clock, manager record, engine scope, effects and deferred removal. Never call original PE addresses. Named context execution, full generic engine dispatch, profiler and debug logs outside scope. Existing cadence flag helper now re-reads default tick as original 2214bf00. Loaded regression schedules initial identity explicitly, then dispatches through strict due gate, stale handle, unknown token, exact death threshold, native terminal identity swap, and deferred removal. Initial install/schedule from damage still hosted in this phase. CI pending.
