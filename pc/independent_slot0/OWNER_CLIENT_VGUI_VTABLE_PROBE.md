# Original 2006 Windows VGUI vtable-anchor diagnostic

Previous owner engine verification: factory-only 0x53494E59, with both
exact interfaces acquired from the already-loaded original vgui2.dll,
preserving the original DLLs on restoration. PR #414 describes that run.

Phase 98 original-binary evidence pins the input vtable at 71 methods and
system vtable at 33 methods, with client-used InputInternal slot 56 and
System slot 24. This new, opt-in test checks only the object vptr, table
readability through slot 56 or 24 respectively, and executable module
ownership of slots 0 and 56 / 0 and 24. Checks use VirtualQuery and
GetModuleHandleEx by address. It never dispatches a GUI method and does
not claim complete 71/33 slot verification, original registration helper
equivalence or successful Client Init.

Run with the existing owner bundle's -EnableClientOwnedStartupProbe,
-EnableClientVGuiProbe and additional -EnableClientVGuiLayoutProbe.
Success terminal diagnostic exit 0x53494E5B. Refusal 0x53494E5C.
Both retain the owner tool's original-DLL authentication/restoration.
The synthetic Windows provider test includes correct anchors,
missing high input/system methods and a malformed input object.
Original game and GUI DLL binaries are never committed.
