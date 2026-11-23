# Phase 654: Original SiN red-black insertion fixup and paired rotations

## Provenance and scope

This phase replaces the last synthetic insertion rebalancing callback in the
Phase 651-653 nested 12-byte tree path with behavior reconstructed directly
from the owner's unmodified 2006 *SiN Episodes: Emergence* `server.dll`.
This is original game entity/symbol registration behavior, **not** a new
implementation of already-working Half-Life 2-equivalent physics, materials,
surface properties, props, model loading or rendering.

Two original PE32 files from the owner Drive `SE1/bin.zip` were verified:

- `server.dll`: SHA256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: SHA256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`

The following **three complete retail native methods** total **1,860 executable bytes**:

| Native VA | Role | Size | Whole-method SHA256 |
| --- | --- | ---: | --- |
| `0x2228E710` | Left rotation around input node | 235B | `5b96eff8753dab4996f3b011a5e4bbd6d8f6ccbfec341d22c9833d94d3759b0a` |
| `0x2228E800` | Right rotation around input node | 238B | `70f1138c52c9aa4ca3a4ce23218620b65935fced6026547bf222ce8a1365e572` |
| `0x2228EDF0` | Insertion recoloring / tree fixup | 1,387B | `69086eef8636a0041aa364549620a2c406af077bb3c451e5f3e0fd97e419ac8c` |

The private audit checks both exact unchanged DLL hashes, executable
`.text` membership, each complete method hash, each terminal
`C2 04 00` (`RET 4`) and following `CC` (`INT3`), and **all 26**
original direct rel32 calls: 22 calls to Phase 653's resolver
`0x2228E100`, and four rotation calls inside the fixup.
Independent GNU binutils disassembly counted 80/80/359 native instructions,
9/9/8 direct calls and 7/7/54 branch instructions respectively. The 54
fixup branches were traced for red-uncle, inner-rotation, outer-rotation and
loop/root exit control flow.

Only derived C++ source, public synthetic tests and hashes are committed.
**The original DLLs and raw machine-code bodies are not in GitHub.**

## Recovered behavior

Nodes are a 12-byte array at `DWORD[map+4]`, with 16-bit indices:

- `WORD[node+0]`: left child
- `WORD[node+2]`: right child
- `WORD[node+4]`: parent
- `WORD[node+6]`: color (red=0, black=1)
- `DWORD[node+8]`: stored symbol/key record

Root is `WORD[map+0x10]`; absent child/parent is `FFFF`.
The lazy shared sentinel is `0x2259012C`, with its bit0 guard at
`0x22590134`. The original helper initializes its three index fields
to `FFFF` and color to black, preserving the other bits in the guard.
Unneeded sentinel initialization is **not** performed on the direct
root-only fast path.

`0x2228E710` moves the right child's left subtree to the old pivot's
right, updates that subtree's parent, rewires the pivot's parent or root,
then makes the old pivot the left child of its former right child.
`0x2228E800` mirrors this for left/right. Neither helper recolors nodes.

`0x2228EDF0` checks the inserted node's parent color. If red, it obtains
the grandparent and uncle, recolors red-uncle cases upward, or handles
left-left, left-right, right-right and right-left configurations with
the original left and right rotation calls. It terminates by setting the
current root's color to black. Its original `__thiscall` input is
`ECX=map` plus one stack DWORD (low WORD node ID); all three methods
return with `RET 4`.

Portable defensive checks on invalid maps, malformed rotations, address
overflow, host callback faults, loop bounds and reentrancy are **not**
represented as original x86 behaviors.

## Regression and integration

`original_nested_rebalance_phase654.hpp/.cpp` provides standalone
`Rebalance::rotate_left`, `rotate_right` and `fixup` using a typed
guest-memory host. The standalone C++17 regression:

- Exercises all **5,040 permutations of seven distinct insertion keys**.
- Checks parent/child linkage, sorted traversal, root blackness, no
  red-parent/red-child pair, and equal black height after every insertion.
- Specifically covers all four three-key zigzag/straight rotations,
  sentinel lazy initialization, pure root initialization, subtree movement
  during a rotation, and reject paths for invalid input/host errors.
- Compiled and PASSED locally with GCC strict warnings and independent
  Clang ASan+UBSan.

A separate compiled Phase 652→653→654 chain test preserves the Phase 653
slot growth/recycle scenario and replaces the synthetic root-black callback
with the **actual Phase 654 compiled fixup**. It also exercises nine
different symbols through original Phase 652 `FindParent`,
`AcquireSlot`, `LinkNode`, Phase 653 `GrowSlots`, and Phase 654
`Rebalance`, including initial capacity 3, and growth to 6 then 12
slots. Tree invariants are checked after every insertion.

Portable and native Windows x86 CMake targets have been added. Android
build/execute, Windows CI, and this new full-chain test remain subject to
exact-commit GitHub Actions verification.

Run the private original audit with:

```sh
python research/tools/audit_original_nested_rebalance_phase654.py \
  --server ORIGINAL_SERVER_DLL --client ORIGINAL_CLIENT_DLL
```

Run the public invented-byte test with:

```sh
python research/tools/audit_original_nested_rebalance_phase654.py --self-test
```

## Proof boundary

The original **three method bodies** have been authenticated, and the
host-independent insertion logic has extensive synthetic tests. This is
**not** native execution of the retail x86 methods, equivalence against
the actual original Windows process, a full replacement `server.dll`,
or proof of Android or Quest 3 gameplay. Game-process ABI loading,
remaining gameplay/AI, and original engine callbacks are outstanding.
