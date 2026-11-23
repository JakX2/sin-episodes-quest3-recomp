# Phase 657: Exact original SiN Windows game-module CreateInterface ABI evidence

## Why this matters

The Windows original-engine boot milestone remains unproven. Earlier
reconstructions can now register, search, clear and recycle names in a
cross-platform guest-memory reference, but the original 2006 Source engine
will not run a replacement `server.dll` or `client.dll` until their
**actual exported interface versions, factory registrations, singleton
addresses and ABI** match.

This phase researches exactly that boundary using the owner's unchanged
retail Windows PE32 images. It does not revisit Half-Life 2 physics, surface
properties, materials, models, props, or rendering, which the owner has
already demonstrated functioning after porting the maps into HL2.

- Original `server.dll` entire SHA256:
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- Original `client.dll` entire SHA256:
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

## Retail-native code proven exactly

The private read-only PE32 audit authenticates **454 complete native x86
executable bytes**, each by its whole original method SHA256:

| Native role | Original server.dll VA | Original client.dll VA | Authenticated size per DLL |
|---|---|---|---:|
| `InterfaceReg` list-entry constructor | `0x221FB5D0` | `0x24164160` | 32B |
| Exported `CreateInterface` | `0x221FB5F0` | `0x24164180` | 114B |
| Three static `InterfaceReg` initializers | See below | See below | 3 × 21B |
| Three trivial singleton factory methods | See below | See below | 3 × 6B |

Unlike scanning strings alone, the audit follows each registration's
**entire 21-byte original static initializer**, checks the exact first
two `PUSH immediate` arguments, the `ECX` static registration-object
address, its real relative `CALL` into that DLL's `InterfaceReg`
constructor and its `RET`. It then checks each factory's entire
6-byte `MOV EAX, singleton_va; RET` against its whole-method hash,
that original name string in `.rdata`, and a *real initialized DWORD
pointer to the initializer* in `.data`. PE32 exports are parsed from
the original export directory; code must be inside executable `.text`.

### Authenticated versions and factory-returned singletons

| DLL | Interface name | Original initializer | Factory | Factory-returned singleton |
|---|---|---|---|---|
| client | `VClient011` | `0x24268300` | `0x240FD840` | `0x242FA4B8` |
| client | `VClientEntityList003` | `0x24268390` | `0x241056B0` | `0x2434FDDC` |
| client | `VClientPrediction001` | `0x24271450` | `0x2422EA30` | `0x2437FA00` |
| server | `ServerGameDLL003` | `0x223BF140` | `0x221E7630` | `0x22510650` |
| server | `ServerGameEnts001` | `0x223BF210` | `0x221E82C0` | `0x22510654` |
| server | `ServerGameClients003` | `0x223BF230` | `0x221E8320` | `0x22510658` |

These are six **priority original interface names**, not a claim
that the DLLs register no other interfaces.

### Complete `InterfaceReg` layout

Both original constructor bodies establish the identical **12-byte x86
record** and prepend entries to one linked list per DLL:

```text
InterfaceReg[0x00] = function pointer: factory() -> interface singleton
InterfaceReg[0x04] = const char*: exact interface version string
InterfaceReg[0x08] = InterfaceReg*: previous linked-list head
module global head = address of newly initialized InterfaceReg
```

- Server list-head global: `0x22573C5C`.
- Client list-head global: `0x24369378`.

Both original `CreateInterface` bodies are individually authenticated
and parse/list-walk entries, compare requested strings **case-sensitively**,
return 1 through the optional return-code pointer on missing version,
return 0 on hit, and invoke the matching entry's factory pointer.
The first matched factory returns its pinned singleton address; none of
these observations identify the signatures of that singleton's methods.

### Known static vtable boundaries

The original on-disk image already contains a vtable pointer in the
first DWORD of four of these six singleton objects:

| Interface | Static vtable address | Verified prefix |
|---|---|---|
| `VClient011` | `0x24296C6C` | first four pointers target client `.text` |
| `ServerGameDLL003` | `0x22442EFC` | first four pointers target server `.text` |
| `ServerGameEnts001` | `0x22443030` | first four pointers target server `.text` |
| `ServerGameClients003` | `0x22443054` | first four pointers target server `.text` |

The original on-disk `VClientEntityList003` and
`VClientPrediction001` singleton DWORDs are in **unbacked `.data`
virtual space**, so their runtime vtable initialization has **not**
been observed or authenticated here. Do not invent these vtables, and
do not attribute method signatures merely from table positions.

## New tools

### Private original static audit

`research/tools/audit_original_createinterface_phase657.py`

```sh
python research/tools/audit_original_createinterface_phase657.py \
  --server ORIGINAL_2006_SERVER_DLL --client ORIGINAL_2006_CLIENT_DLL
```

The tool checks both exact entire DLL hashes, PE section ownership,
the two original `CreateInterface` exports and their two exact
32-byte registration constructors, all six 21-byte initializer
functions and six 6-byte return factories, initialized pointers to
those thunks, the six exact name strings, and the four file-backed
vtable prefixes. All private game DLL bytes remain local to the owner.

Public CI uses only **invented-byte mutation tests**:

```sh
python research/tools/audit_original_createinterface_phase657.py --self-test
```

### Owner-side Windows x86 runtime factory probe

`research/tools/sin_win32_factory_probe_phase657.cpp`

This is a stricter and more useful continuation of Phase649's basic
`CreateInterface` name probe. It must be run in a **trusted owner-side
32-bit Windows environment**, with the game's required dependencies
installed. It computes the SHA256 of both DLL paths through Windows
CNG/Bcrypt and **refuses to call `LoadLibraryEx` unless their hashes
are the exact pinned 2006 originals**. Only after that check does it:

1. Load the trusted original `client.dll` and `server.dll`.
2. Check exported `CreateInterface` RVA against the retail audit,
   independent of ASLR or rebasing.
3. Call the six proven factory names, checking the success return
   code and exactly pinned **singleton RVA** rather than treating any
   nonnull result as sufficient.
4. Check one deliberately nonexistent version per DLL, requiring
   null result and missing-name return code 1.
5. Emit JSONL results without invoking **any unknown vtable slot,
   `DLLInit`, `Init`, level-init or entity spawn**.

It is built by the existing native MSVC Win32 CMake CI target named
`sin_pc_original_factory_abi_probe_phase657` and linked against
`bcrypt.lib`. This executable is a diagnostic, not a replacement
`client.dll` or `server.dll`. Note that even successful
`LoadLibraryEx` **runs the original DLL's `DllMain`**, so use it only
with the pinned trusted originals.

To build from a Visual Studio 2022 x86-capable command line:

```powershell
cmake -S pc/windows_x86_proxy -B build-pc-win32 -G "Visual Studio 17 2022" -A Win32
cmake --build build-pc-win32 --config Release --target sin_pc_original_factory_abi_probe_phase657
```

Then run the generated executable with the owner-side **actual original**
client/server DLL paths, preferably from an environment with all original
Source runtime dependencies available. A missing dependency yields
a load failure; **it does not prove an interface mismatch**.

## Proof boundary and next milestone

We now have exact **static binary evidence for which two game-module
exports and six major versioned interfaces exist**, along with their
registered factory function addresses, original singleton pointers
and the `InterfaceReg` linked-list ABI. The owner-side dynamic probe
must still be run against retail binaries on actual Windows; GitHub
CI contains neither originals nor any proof of original process startup.

The next step is to authenticate the precise **engine-consumed vtable
method signatures and initialization order** from the retail object
constructors and callsites, then attempt the first non-forwarding
replacement `CreateInterface` game module in a controlled Windows
engine launch. Do not equate source-model tests or a factory hit with
playable PC/Android/Quest 3 integration.
