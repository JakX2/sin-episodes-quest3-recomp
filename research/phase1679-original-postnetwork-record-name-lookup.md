# Phase 1679: original 121-byte CDECL recursive record-name selection

## Source-authenticated 2006 PE32 original

The previously opaque helper called from the Phase1671 diagnostic core is **not a text formatter**. Direct read-only inspection of the owner-provided unchanged `SE1/bin.zip` original `client.dll` proves that original native `0x24197ED0` is a **121-byte two-argument CDECL record-name lookup**, ending in a plain `RET`. Its complete SHA256 is `ffd2e7d6539a429f896954abf6865665c971f7496d588a0d967209a3f956812b`; the next seven bytes are compiler `INT3` padding.

Its two direct original callers are located at `0x2419915B` and `0x2419917C` in the previously authenticated 139-byte Phase1671 diagnostic core. Their E8 targets both resolve to `0x24197ED0`. The caller cleans eight stack bytes. This lookup's return value is the selected record address, which Phase1671 stores in `this+0x34`; Phase1672 uses `this+0x18` as the current record; Phase1675 then compares the two addresses to decide which diagnostic formatter to invoke.

### Recovered original control flow

- First stack argument: the requested record-name string. Second stack argument: a pointer to a linked-list node.
- The node's `+0x00` DWORD addresses an array of records, its `+0x04` signed DWORD is the record count, and `+0x0C` is the next-node pointer. A count of zero or less skips local record reading and proceeds to the next node.
- Each record has exactly **0x34 = 52 bytes**. Type at `record+0x00` is checked first. Type `0` is skipped without a name comparison.
- Type `10` follows a nested node pointer at `record+0x20` and recursively invokes this *same native CDECL function* before testing the type-10 parent's own name. A found nested record returns immediately.
- For other nonzero types, or for a type-10 whose recursive subtree had no match, the pointer `record+0x04` is passed as the first string argument, while the requested key is passed second, to still-separate original comparator `0x2425F81B` at call site `0x24197F16`. **Comparator EAX=0** means match; the method returns that record's address. The full locale-dependent comparison semantics are not reconstructed by this phase.
- If local records all miss, the method walks the node's `+0x0C` link. No match returns null.
- The exact retail original contains **no HIGHLOW relocations**, unlike the Phase1678 formatter. Its two direct rel32 calls are self-recursion at `0x24197F01` and the comparator at `0x24197F16`. Original code does not protect invalid pointers, list cycles or excessive nesting.

The complete original `client.dll` and `engine.dll` image hashes are required for an owner-private source audit, not stored in GitHub. All machine-code evidence in this repository is limited to complete SHA digests, short sparse opcode witnesses, and call-address/relocation data. No proprietary game executable code or assets are committed.

## Original-free reconstruction

New `pc/independent_slot0/native_original_postnetwork_record_name_lookup.h` supplies:

1. A real compiler-generated **two-argument CDECL `RET0`** `NativeLookup::Find(name,node)` with 0x34-byte records, signed count, type0 skip, type10 child-first recursion, name comparison callback and linked-list next traversal.
2. Explicit grants for synthetic record arrays, nodes, record-name pointers, and the one authorized query key. The retail comparator at `0x2425F81B` is a **separate callback**; the test uses limited ASCII case folding and does not claim locale-faithful original engine behavior.
3. Synthetic-only bounded recursion depth, node-walk budget, total examined-record budget and rejection of unowned pointers. Real retail original would follow/dereference them without validation.
4. `IntegratedFixture::SelectDiagnostic`, an original-free native three-stack-DWORD `__thiscall RET12` same-owner selection gate reproducing the earlier Phase1671 selection contract: reset selected pointer at `this+0x34`, store first diagnostic input at `this+0x38`, gate by nonempty input and expected index, select configured or debug `0x1000` query, invoke the new two-argument cdecl finder, and write its returned record address to `this+0x34`.

New `pc/independent_slot0/test_original_postnetwork_record_name_lookup.cpp` uses real MSVC Win32 x86 generated machine code and invented host-owned records. The regression checks native CDECL stack restoration, nested child-first match, type-10 own-name fallback, type-0 skip, ASCII query-match callback, signed nonpositive record count, linked-node traversal, absent match, and host-only refusal of ungranted pointers/cyclic links/deep type10 recursion. Then, on the **same original-layout synthetic owner**, it connects `SelectDiagnostic RET12 → typed dispatcher RET16 → selected binary/signed-short scalar formatter RET16 → Phase1678 bounded cdecl Msg` for selected type9 binary, type8 integer and recursively selected type7 short records. This tests that a genuine lookup result, not a hardcoded test token, controls preview output.

Source auditor `research/tools/audit_sin_original_postnetwork_record_name_lookup.py` pins the unchanged **whole 121B method SHA256**, both exact SHA-authenticated parent core calls, the two method-local rel32 targets, byte/stack/record layout witnesses, seven INT3 terminators, and zero HIGHLOW relocations. Public `--self-test` constructs only invented sparse non-executable x86 fragments and refuses mutations/truncations/forged original whole-file hashes.

## Test status, scope and non-goals

Verified baseline at Phase1678 exact commit `937f306eedb2efd9dc54530d13ceb4b4d9bc1958`: **302/302** genuine native Win32 MSVC x86 CTests and **all six** corresponding CI workflows green.

Phase1679 adds **two** original-free tests, target **304**, not verified until exact-head CI succeeds. This closes a real missing **name-selection dependency**, not the entire prediction or network reconciliation subsystem. The original source-engine ConVar lookup, 2006 CRT locale comparator, node and string ownership, dispatch type10 specialist recursion, production client/server registration, Windows retail gameplay startup, Android executable and Quest 3 VR startup are still unverified or opaque. No physics, surfaces, props, model loading/rendering, or materials work was added.

**Retail function semantics audited from the original DLL; safe-fixture behavioral assumptions are explicitly separate.**
