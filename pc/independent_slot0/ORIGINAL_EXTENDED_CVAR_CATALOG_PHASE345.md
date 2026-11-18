# Phase 345: extended original server numeric console catalog

The retail Phase 340 owner run proved that 24 selected independently
constructed native variables registered and were recognized by the
actual engine. Its final verification was 24/24 (0x53494E4D).
This does not prove the remaining variables, command callbacks,
or complete global registration order.

Both owner-original PE32 images were SHA-256 verified:
server 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650
client 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
A local scan of original direct calls to the three-argument ConVar
constructor (0x25BD20) identifies 181 distinct numeric declarations
with flags zero and no exact NUL-terminated name in the client.
The canonical ordered metadata rows (name, string default, call
RVA, object RVA) have SHA256
9b47c80ae0393896967f5b1e35fa0779142b13fa58aba97df748fb7c20bbdfbe.
All 24 retail-verified declarations occur in this set. The data
are declaration metadata, not original method implementation bodies.
Ascending constructor-call RVA is a selection order, not a claim
of actual static initialization order. Absence of an exact client
name is not proof of absence of conflicts with the engine/plugins.

server_extended_cvar_catalog.h preserves the exact 181 candidates.
Inspect-OwnerOriginalExtendedCvars.py validates the digest and
matches the catalog to the earlier authenticated complete 625-record
inventory. Its synthetic regression checks the digest, original
subset and numeric/flags/constructor/name filter.

The native registrar now accepts explicitly sized owned catalogs.
The existing 24-object class retains the same entry point and trace.
It rejects duplicate names before engine queries and refuses repeat
attempts. All names and command-line overrides are checked before
external registration, followed by object allocation and per-object
registration/identity checks. The final full recheck now confirms
string storage, names, flags and numeric values as well as parent and
lookup identity. A native fixture exercises all 181 actual metadata
records, last-name/override preflight conflicts, unexpected relinking
at record 100, final first-value numeric corruption and duplicate
names. All fixture interfaces and object bodies are independent.
The extended catalog is not enabled for retail registration in
this phase. No command stubs or incomplete registry are exposed.
The next retail client diagnostic is Phase 343 screen query; Phase
344 Gaussian attach and this catalog require native CI validation.
No original DLL or original implementation bodies are committed.
