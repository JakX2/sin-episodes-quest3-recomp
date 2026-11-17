# Phase 278: independent Win32 x86 lifecycle virtual ABI smoke

Stacked on Phase 277 PR #334. A separate synthetic-only DLL now provides a
four-slot MSVC x86 virtual interface with `initialize`, `initialized`,
`generation`, and `shutdown`. It deliberately rejects retail interface
names and never links, forwards to, or loads original SiN DLLs.

The native x86 executable checks PE32 module ownership of the factory,
vtable and four method pointers. It exercises invalid callback descriptors
with no state mutation, first initialization, double-initialize rejection,
callback calling convention, reentrant virtual state queries from callbacks,
partial DLL unload, idempotent shutdown, second initialization generation,
and final teardown. Sixteen independent LoadLibrary/FreeLibrary cycles
contain 32 complete init/shutdown pairs and 64 host callbacks.

The callback structure is an explicitly synthetic three-DWORD contract;
this does **not** claim that retail `VClient011` or `ServerGameDLL003`
uses these slots or arguments. No original executable or DLL is used in CI.
Original PE32 provenance SHA256 gates remain unchanged.

Next milestone: after separately reviewed owner-original interface
method signatures become available, implement an engine-specific adapter
and validate it against a real retail executable in a disposable game copy.
No original-engine interoperability or gameplay boot is demonstrated here.
