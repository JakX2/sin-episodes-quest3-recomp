# Phase 335: native console object ABI

The private original server.dll SHA-256 was checked again:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Original table spans were read directly at RVAs 0x45377C, 0x4537A4,
and 0x4537F0. They contain 9, 18 and 13 executable slots,
respectively, for the base, ConVar and ConCommand. The owner SDK 2004
public/tier1/convar.h and public/icvar.h corroborate field and method
types. The original public ConVar setter order is int, float, string
(slots 9, 10, 11); internal order is string, float, int (12, 13, 14).
Public setters dispatch through the root parent; virtual flags, name,
help and registration getters use that parent too. Slots 15,16,17 are
clamp, string replacement and nine-argument creation. The command
table's slots 9..12 are completion, completion availability, dispatch,
and the five-argument creation method. A command without completion
uses the original zero-result default callback behavior.

server_console_object_abi.h independently implements these native
virtual interfaces and 24/68/36-byte records with owned strings,
bounded conversion, explicit allocation ownership and root forwarding.
Object initialization and registration use an explicit scoped synthetic
accessor rather than the original DLL's global registry. A failed
accessor return still marks registration, as observed in original
Init. The complete original registration accessor, auto-registration
list, callbacks for the 625 declarations, cross-module allocation and
engine-owned parent lifetime still need integration. The base creation
method only initializes metadata; no implicit original static list is
created. The scalar setters implement numerical and string behavior
without asserting bit-exact legacy x87 equivalence.

The native regression examines the actual record field offsets and
calls the virtual methods by original slot index, tests callback state,
root forwarding, command completion, synthetic registration, x86
callee stack balance and same-module scalar deletion. No retail
interface is passed an object. The SHA-gated inventory inspector now
checks all three original table spans in addition to declaration
metadata. Phase 332 remains the latest retail-confirmed startup.
No original binary or original implementation bodies are committed.
