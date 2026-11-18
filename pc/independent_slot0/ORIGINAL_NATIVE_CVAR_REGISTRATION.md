# Phase 336: native console registration adapter

The SHA-pinned original server registration accessor (RVA 0x1E6490)
was inspected directly again. It calls the object AddFlags slot 3
with 4, IsBitSet slot 2 with 0x2000, and obtains the root default
at parent+0x1C via 0x25BB70 for replicated variables. It detaches
the current object, calls ICvar slot 0, queries slot 1 using the
object's post-registration name, then calls public string setter
slot 11 with an override or the saved replicated default. Original
manager 0x25B350 saves each successor before invoking Init slot 8.
The original subsequent developer lookup uses ICvar const slot 3.
The owner SDK 2004 public/icvar.h corroborates the five-slot table
(including mutable FindVar at slot 2 and GetCommands at slot 4).
The original sv_cheats lookup at 0x25BC00 walks the post-registration
local-head chain, not a direct engine FindVar query.

NativeConsoleRegistration implements these operations on Phase 335
native objects and a typed independently implemented engine fixture.
It preserves the old list successor and default across the register
call, distinguishes engine-owned relinking from its local list,
handles command-line precedence and avoids setting commands. The
test uses a derived variable as the synthetic engine-owned root,
with a distinct virtual setter implementation. Removing final from
Variable prevents forcing its own implementation rather than the
root's actual virtual dispatch. The test confirms both relinked
roots are updated through their own vtable, the original local
strings remain independent, and developer/sv_cheats bindings are
obtained through their correct paths. It also checks cycle and count
preflight, reentry and missing sv_cheats after nonreversible effects.

This adapter is still exercised only against independently owned
objects. No original engine method is called by public CI. A full
retail console registry requires actual declaration objects, safe
ownership across DLLs, and all required command callbacks. Do not
register incomplete command stubs. No original DLL or methods
committed.