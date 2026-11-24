# Phase 671: native map-scoped entity behavior registry

Builds on green Phase670 (env_global) and Phase665 (info_challenge).
The actual independent server.dll now exposes a versioned
SiN_GetMapEntityApi table for a host to attach these behavior objects
to engine-created entity identities. No CBaseEntity, datamap, edict,
or original Source class factory is fabricated.

Level start requires an existing fixed campaign. Level epochs are
nonzero, monotonic within a loaded DLL; end_level clears map-local
objects without clearing campaign or CGlobalState. Campaign end
refuses while a level is active. IDs include epoch/index/generation;
a retired slot may be reused only with a greater generation during
the same level. At most 8192 distinct indices per level.

Create accepts exactly env_global and info_challenge. Keyvalue routes
globalstate, initialstate and spawnflags for env_global before spawn.
Unknown/base keys are explicitly unhandled, including Source output
connection strings; the engine host owns those. Integer parsing is
strict port-side decimal, not a claim of original KeyValue conversion.
Spawn routes Phase670 behavior or stores the Phase665 snapshot;
null env_global name reports RemoveSelf, awaiting host removal.
Inputs route exact recovered names. info_challenge delivers the
original output name/field offset and fixed-policy value to a
caller-provided synchronous float callback, with both activator and
caller represented by the same full entity identity. A missing or
failed sink never reports success. A callback cannot reenter mutable
DLL services; host should queue nested I/O and process after return.
No output context or engine pointer is retained.

The actual DLL loader exercises create/keyvalue/spawn/named-input/
remove, duplicate/stale handles, failed output and exceptions,
reentrancy, persistent globals across level transitions, and per-entity
challenge snapshots. Both Win32 and Linux loaded-module paths use the
same implementation. Original engine map parser, factory registration,
inputdata_t and COutputEvent adapters remain future work. No retail
startup or actual map dispatch is claimed.
