# Phase 340: bounded 24-variable original declaration batch

Actual Phase 337 owner run registered dd_log_events with the real engine,
received the same pointer through lookup, and exited with 0x53494E49.
Original server restoration and retained client verification succeeded.
The actual sequence is now a regression, not hypothetical evidence.

Owner-local SHA verification used both original DLLs. A bounded .text
scan of direct three-argument ConVar constructor calls to 0x25BD20,
with immediate flags zero, numeric defaults and an exact name string
absent from the original client, yielded 181 candidate declarations.
This does not prove absence of engine/plugin name conflicts. Select
24 of these declarations, including original difficulty/resupply,
grenade, grunt, mutant, turret and sprint-related variables. Each
header record contains the exact name/default/constructor-call RVA
and object RVA. The owner-local inspector verifies each against the
original declaration inventory and client SHA pin. The chosen order
is ascending constructor-call RVA, not a claim of reconstructed global
static initialization order. All have no bound or change callback.

The existing single-variable path remains intact. The new batch
mode requires EnableServerCvarBatchProbe in addition to all Phase 337
server gates, and a positive initial gameevents result. It first
checks every name via engine lookup and bounded command-list scan,
checks every override, and allocates all 24 owned native variables.
A conflict refuses the entire batch before registration. Then it
registers each object using the original verified ICvar slot 0,
checks for unexpected relinking/override, verifies actual pointer
identity and unchanged native values, and after the final insertion
rechecks every registered variable. Objects persist through terminal
process exit. If an operation fails after registration, no rollback
or unverified cleanup is attempted; the diagnostic stops and reports
the verified prefix. A refusal after 24 individual successes can
still occur on the final consistency check.

Exit 0x53494E4D means verified 24/24 including final consistency.
0x53494E4E means refusal, with reason and count in the trace. The
classifier requires the ordered individual entries/returns/checks
before accepting a verified count. Tests extend the actual DLL
synthetic child path with full success, last-name and override
preflight conflicts, existing command, cyclic list, partial relink
and lookup failure, and a final consistency failure. They do not
execute a retail game. The complete 625 declarations, command
callbacks, full registration order and subsequent game-system
initialization remain unfinished. No original game code or binary
committed. Only a validated disposable diagnostic should be run.
