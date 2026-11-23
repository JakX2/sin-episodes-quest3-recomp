# Phase 659: Original client AppSystem registration, connection and rollback

## Original 2006 retail client.dll provenance

This phase continues from Phase 658's first original client
`VClient011::Init` dependency acquisitions. The original client
startup at `0x240FE39B` calls a separate registry connector at
`0x24164120`, checks its boolean return and branches to startup
failure when it returns false. This is a missing **client startup
interface-lifetime dependency** for genuine non-forwarding game-DLL
replacement, *not* a recreation of existing Half-Life 2 physics,
surface properties, props, model loading or rendering.

Authenticated original PE32 DLLs remain private:

- `client.dll` SHA256
  `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- `server.dll` SHA256
  `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`

These **three complete original native methods** total **141 executable
x86 bytes**, independently checked with full-body hashes:

| Native function | Original VA | Original bytes | Whole-method SHA256 |
|---|---|---:|---|
| Register new app-system list entry | `0x241640C0` | 39 | `07298d888357f960d2015e017d185ec8fcffbac239a2604e8b8eb4d4b8786696` |
| Disconnect/release all registered entries | `0x241640F0` | 48 | `93dbf327403a2963eba2ee6341b0aa5bfc0a7fef581840e57b71b418d7ae78d2` |
| Connect all registered entries | `0x24164120` | 54 | `645a9002d77b476692fe8688d691e87c4d2ebf8a61976be9eda071e41d7d3b7f` |

Exact native method boundaries matter: register ends in `RET 0x0C`
followed by `INT3`; disconnect ends with `RET` immediately
followed by the new valid connect entry at `0x24164120`, **not
`INT3`**; connect ends `RET` followed by `INT3`. The private
audit checks all three method hashes, correct executable `.text`
placement, original linked-list head and three original rel32 call
edges:

- `0x240FE39B → 0x24164120` (real client Init).
- `0x2416414D → 0x241640F0` (failed connect forces full disconnect).
- `0x240FF422 → 0x241640F0` (client shutdown calls disconnect).

## Exact original 16-byte registration-node layout

The shared module global list head is `DWORD[0x24369374]`.

```text
ClientAppSystemReg[+0x00] DWORD pointer to caller-owned interface pointer
ClientAppSystemReg[+0x04] DWORD zero-argument resolve/get callback
ClientAppSystemReg[+0x08] DWORD release/disconnect callback
ClientAppSystemReg[+0x0C] DWORD next node (prior linked-list head)
```

The original registration constructor is
`__thiscall ECX=entry` with three stack DWORDs
`(target_storage*,resolve_callback,release_callback)` and
`RET 0x0C`. It writes the first three fields, copies the
previous list head into entry+0xC, and sets global head to the
new entry. **The list is head-inserted**, so connection and release
traverse the reverse of registration order.

### Original connect `0x24164120`

For each entry, call the original zero-argument resolver callback
through `[entry+4]`; it returns EAX.

- If EAX is nonzero, read `[entry+0]` and write EAX to the
  destination it points to. The native code does not check the
  destination for null on this successful path.
- If EAX is zero and `[entry+0]==0`, the entry is **optional**:
  continue to the next node without a failure.
- If EAX is zero and `[entry+0]!=0`, the entry is **required**:
  invoke original `0x241640F0` to disconnect **the entire registered
  list**, then return AL=0.
- If all entries pass, including optional absent ones, return AL=1.

The original list is not erased or rewired during success or failure.

### Original disconnect `0x241640F0`

Traverse from global head. For every entry with a nonzero
`[entry+0]` target:

1. Read `*target` (the current interface pointer), even if zero.
2. Push that pointer on the stack, then call the original
   callback through `[entry+8]`.
3. Clear `*target=0` **after** the release callback returns.

Entries with null target-storage pointers skip release entirely.
The original visits **every registration on failed connect**, not
only those successfully resolved earlier, and retains the registry
nodes for later reconnect.

## C++17 reference reconstruction and tests

Implemented `AppSystemChain::register_entry`, `connect_all` and
`disconnect_all` through a typed guest host. The host supplies guest
DWORD reads/writes and the actual engine's original resolution and
disconnect callbacks. These callbacks remain **mandatory external
dependencies**; the source model does not invent their native
implementations.

Standalone regression verifies:

- Exact 5-write constructor ordering and head-inserted list.
- Complete successful three-node connection and disconnect with
  original reverse registration order and correct stored pointers.
- A **required provider missing mid-list** invokes whole-list release,
  including a previously connected entry, the failing zero-valued
  entry and a not-yet-connected entry with a preexisting pointer.
- An optional entry with `target_storage=0` and a missing provider
  is skipped, whereas a nonzero provider with a null destination
  is trapped by the protective portable host.
- Failure of a release callback leaves the current destination
  uncleared; later explicit cleanup succeeds.
- Null/invalid registration, callback exception, reentrant invocation,
  and malformed cyclic guest lists are detected as **portable
  safeguards not present in original native code**.

The source has native MSVC x86 and strict C++17 test targets, an
independent Clang ASan+UBSan suite and an original-free invented-byte
audit self-test.

```sh
python research/tools/audit_original_client_appsystem_phase659.py --self-test
```

The owner's read-only retail verification uses:

```sh
python research/tools/audit_original_client_appsystem_phase659.py \
  --client ORIGINAL_2006_CLIENT_DLL --server ORIGINAL_2006_SERVER_DLL
```

The exact run results are added to the PR after CI finishes.

## Next runtime boundary

The native client Init has now revealed and modeled its required
**app-system registration lifecycle** and rollback path, but the
factory callbacks named in the list still require their genuine
engine-facing behavior; this phase does not assert the original
`VClient011::Init` returned success.

Next we should recover the **individual registered callback implementations**
and their registration sites, then attempt a Windows x86 module startup
using the original engine's runtime factories. A factory-resolution
test or compiled synthetic guest-memory test does not establish a
playable game, Android port or Quest 3 VR execution.
