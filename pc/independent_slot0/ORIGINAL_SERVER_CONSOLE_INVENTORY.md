# Phase 334: original console declaration inventory and root values

## Retail evidence

The Phase 332 owner diagnostic successfully executed the original engine
event loader after sound Connect. Its signed return was 47. Exit code
0x53494E48 and restoration VERIFIED_COMPLETE confirm the intended
terminal diagnostic. This is not complete DLLInit. The actual trace
is replayed in the offline classifier regression.

## Declaration inventory

Direct owner-local SHA-256 verification of the original server.dll:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.

Scan the original executable section for relative calls to the verified
console constructor wrappers, validate their immediate receiver and
complete argument sequence, check mapped storage and name/default strings.
The following are the observed direct call counts, not an assertion
that no additional dynamically constructed console objects exist:

| Constructor RVA | Argument count | Declaration count |
| --- | ---: | ---: |
| 0x25BD20 | 3 | 335 |
| 0x25BDA0 | 4 | 85 |
| 0x25BDE0 | 8 | 5 |
| 0x25BE30 | 5 | 4 |
| 0x25BE80 | 9 | 0 |
| 0x25BCC0 | 5 (command) | 196 |

All 625 sites yielded exactly one valid immediate argument pattern in
the local scan. There are 429 variable and 196 command declarations,
73 declarations with flag 0x2000 and 9 with flag 1. Names and object
storage addresses are unique. The developer declaration has default 0
and flags 0. sv_cheats does not appear as a constructor declaration in
this inventory: the original searches its linked command list after
registration, so it must not be manufactured as a game-owned variable.

Inspect-OwnerOriginalConsole.py reproduces the inventory on an owner
original and authenticates a canonical digest. The output includes
names, default values, flags, object/call/constructor RVAs and raw
constructor argument scalars; callbacks remain unresolved addresses.
No original binary or callback implementation is committed.
The synthetic test exercises sign extension, duplicate names/receivers,
out-of-bounds receivers and the SHA gate.

## Root-value semantics

Original ConVar record is 0x44 bytes on x86, with parent +0x18,
default +0x1C, mutable string +0x20, length +0x24, float +0x28,
integer +0x2C, bound flags/values +0x30/+0x34/+0x38/+0x3C,
and change callback +0x40. Original ConVar vtable RVA 0x4537A4
contains 18 executable entries. Original public setter slots 9,10,11
delegate to root internal int, float and string methods. Internal
setter RVAs are 0x25B930, 0x25B8B0 and 0x25B720. Clamp is 0x25B860;
string replacement and old-value callback is 0x25B7B0.
SDK 2004 public/tier1/convar.h and tier1/convar.cpp corroborate.

server_console_value_core.h independently models a root and one-hop
aliases, preserved default, numeric and string setters, min/max,
old-string callback, and never-as-string behavior. It preserves the
original distinction between numeric setters' unchanged-value early
return and the string setter's callback. It rejects nonfinite inputs,
invalid bounds and unsafe alias chains as independent safeguards.
No bit-exact legacy floating-point or locale claim is made. Object
owners must keep roots alive while aliases exist.

The regression checks alias updates, initial default not clamped,
clamped string formatting, callback old/new state, unchanged numeric
setter suppression, string self-assignment, revert, never-as-string,
and bounded conversion. This is a standalone owned model, not the
original 68-byte ABI object or a registration-ready engine object.
Actual console callback reconstruction, typed ABI and engine-owned
registration remain unfinished. No new retail test is requested.
