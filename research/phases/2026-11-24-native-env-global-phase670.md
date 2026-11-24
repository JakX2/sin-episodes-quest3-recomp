# Phase 670: original env_global behavior on native CGlobalState

Original server SHA256: 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Direct disassembly of the owner-provided original verified CEnvGlobal fields
m_globalstate (+0x394, globalstate), m_initialstate (+0x39c, initialstate)
and the Spawn flag at +0xb4 bit 0. The original datamap contains these
input name/function pointer pairs:

| Input | Function VA | Pointer VA | SHA256 (complete function, including all branches) |
|---|---|---|---|
| Spawn | 0x22179470 | vtable 0x2241f7b8 | 04a2e897ed331d8b1d21e28c53a2615faabf1c39f99808a6a3cbabbfd1574592 |
| TurnOn | 0x221794e0 | 0x224fc2e4 | ffe6f0fea454bd62052b99ae3fc291c529087c0743f60de9f4934fb6403b1eb2 |
| TurnOff | 0x22179560 | 0x224fc318 | 5283971b3f6bb9a33be3e4d2e75a34b3e1ca9b052439f7e3051b29a7b68c4624 |
| Remove | 0x221795e0 | 0x224fc34c | 99e8d875fe435466aec99342cfc5679a343c53cb43ef0bf9e0fe9fce667f9513 |
| Toggle | 0x22179660 | 0x224fc380 | b1001430d4df71be63e695cc3fb01fbb5922e7b8babc4a4ee0b39902e89480a6 |

Sizes: 97, 118, 118, 118, 167 bytes respectively.
Spawn with null string_t calls original removal helper 0x22286bf0.
Spawn with flag 1 creates only if missing; duplicate preserved.
Inputs set 1,0,2; missing entries added using current level.
Toggle reads default state 0 for absent index; only states 0/1 flip.
Dead state 2 and unexpected states do not toggle. Remove marks state 2
rather than erasing the registry entry. Existing entry retains its
level. Registry update-disable byte blocks existing state writes,
not creation. A null input name or level resolves to empty string.

The new portable native export SiN_EnvGlobalDispatch operates on the
same DLL-owned CGlobalState as configured campaign start. It accepts
already-resolved strings and flags; reports RemoveSelf for the future
host instead of pretending to delete a Source entity. No original
object layout, entity factory, inputdata_t dispatch or map loader is
implemented by this export. Existing global state API unchanged.

The existing loaded-module test runs these branches on Linux and Win32.
It covers duplicate preservation across levels, remove/dead semantics,
missing entries, update-disable differences, empty strings and case.
No retail engine execution is claimed.
