# Original 2006 AutoGameSystem registry positional insert and append

## Original source verified from owner's client.dll

The preceding registered-system destructor work (PR #510)
reconstructed first-match removal from the original global list.
This stage reconstructs its insertion partner from the exact same
SHA-256-authenticated SiN Episodes: Emergence 2006 DLL:

6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

| Original routine | VA | Complete length | SHA256 |
| --- | --- | ---: | --- |
| Ordered pointer insertion | 0x24156640 | 100 bytes | 486336177fcab7b1b790c9cffa47767574b21790bb88d23ea570c71883d197e2 |
| Append-at-count wrapper | 0x24156800 | 17 bytes | d8347541722acb5f3079e4a815ecfa670797097a8f64173fd638291a4f60b711 |

Original ordered insertion uses the same global registry structure
as removal: pointer backing array at +0, capacity +4, growth
configuration +8, active count +0x0C, data-pointer mirror +0x10.

The original method:

1. Compare active count+1 with capacity. If capacity would be
   exceeded, call source growth helper 0x241562F0.
2. Increment active count by one.
3. Compute source elements following the caller-supplied insertion
   index. Copy backing array data pointer into mirror field +0x10.
4. If there are trailing elements, memmove them ONE pointer slot
   (four bytes) right using original helper 0x24248B50.
5. Copy the pointer supplied by the caller into data[index]
   and return the insertion index. It does not check duplicate
   pointer identity or enforce uniqueness.

The original 17-byte wrapper at 0x24156800 takes an object pointer,
reads current active count from registry+0x0C, and calls this same
insertion helper with index=count. It is therefore an **append**,
not a sorted insertion or a unique registration.

The complete 100-byte and 17-byte method SHAs, selected exact
x86 instructions and three direct call targets
(EnsureCapacity, memmove, InsertAt) are verified by private mode of
audit_sin_original_autogamesystem_registry_insert.py.
The public source-free Python regression fabricates original-style
opcodes and refuses corrupted witness bytes.

## Independent source policy

The existing owned AutoGameSystem registry from PR #510 is extended
with:

- InsertAt(index, identity): source-order stable 32-bit token
  shift right from the insertion index, then exact token write,
  active count increment and revision increment.
- Append(identity): forwards the current count as index.
- A simulated data-pointer mirror state set only after
  successful insertion, matching the original +0x10 write.
- No duplicate suppression or invented null-pointer restriction.
- Owned-only invalid-index rejection and a fixed capacity of
  1024 entries. These limits are NOT original Source behavior.

The original 0x241562F0 helper uses engine allocation/reallocation,
size growth configuration and a real vector. We deliberately do
NOT claim the fixed-size owned array is a binary-compatible
implementation of that allocator. It never touches actual
engine pointers or the original registry global.

Native Windows x86 fixture
test_client_original_autogamesystem_registry_insert.cpp
exercises initial append, middle insert shifting two elements,
back append with zero shifts, front insert, duplicate values,
original first-match removal preserving the other duplicate,
source mirror-state update and invalid-index/capacity refusal.
No actual retail-game memory or registered engine system is
mutated in public CI.

## What remains for genuine Windows boot

The original game-system registry now has independently owned
models for positional insertion and first-match removal,
plus the base destructor vtable transition. For normal client
startup we still need the exact real game-system registration
call sites, object allocation/lifetime ownership and safe
interface context. A working VClient011::Init cannot be
inferred merely from this list helper.

Last observed owner-retail Windows checkpoint remains 0x53494E74.
Prepared 0x53494E76/77 soundscape catalog and read-only CVar
diagnostics are not yet confirmed in the genuine retail process.
Android/Quest 3 support is still downstream of real Windows boot.
