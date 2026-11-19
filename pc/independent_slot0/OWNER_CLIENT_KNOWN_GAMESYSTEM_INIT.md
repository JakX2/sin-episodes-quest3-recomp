# Three complete original 2006 client AutoGameSystem Init implementations

Source: original owner `client.dll` (SHA256
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`).
The four static systems and genuine RTTI/vtable ownership are independently
SHA-audited in `research/tools/audit_sin_client_static_gamesystem_rtti.py`.

The three complete **original** x86 method bodies are short:

- `CSoundEmitterSystem::Init` at `0x241BC950`, 11 bytes, loads the
  engine sound-emitter pointer from original global `0x2433F6E4`
  and tail-dispatches its slot 1, returning that method's bool.
- `CViewportClientSystem::Init` at `0x241ED310`, 14 bytes, gets
  original global `0x2437F8A0` and calls its slot 7. It returns true
  immediately afterward, ignoring that callback's return.
- `CClientThinkList::Init` at `0x24103DF0`, 11 bytes, zeroes the
  32-bit word at this+0x48 and the byte at this+0x4C, returns true.

`client_known_gamesystem_init.h` reproduces these behaviors with
already-supplied **independent** provider object pointers, verifies
virtual method memory belongs to the provider's module, and refuses
unsafe pointer layouts without dispatch. ThinkList writes are
restricted to writable, committed independent memory.

The native x86 test covers both results of sound emitter slot 1,
viewport slot 7, a missing provider/vtable, exact ThinkList offsets,
and integration with the independent AutoGameSystem registry's
constructor LIFO and first-failure gate.

The original `CPerfVisualBenchmark::Init` at `0x24182800` is a
longer subsystem initializer. Its entry has a verified 32-byte
prefix, but its full implementation and resource dependencies remain
unrecovered. Other original BSS-backed object constructors also
remain required before a true `VClient011::Init` return. **Do not use
fake empty game systems to pass that real startup gate.**

No retail engine process test is claimed for these objects. Previous
owner-original Windows milestone remains `0x53494E61`.
