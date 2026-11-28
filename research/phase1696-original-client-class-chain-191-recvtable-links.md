# Phase 1696: Original 2006 CHLClient ClientClass chain and RecvTable inventory

## Actual retail-engine reason for this task

The owner already executed the independently built PE32 x86 replacement client.dll in the **real original SiN Episodes Windows engine**. The resulting session proved all 21 source app-interface queries, native VSoundEmitter001.Connect, cl_predict, prediction, entity list, centerprint and ClientLeafSystem001 factory negotiation. The first true unsupported client virtual method was **VClient011.slot5**, which intentionally exited with diagnostic code 0x53494E45. Client restoration was VERIFIED_COMPLETE; original server.dll unchanged.

Unlike an absent optional factory, slot5 is an engine-invoked *virtual method*. Its actual source behavior cannot be safely approximated with a null or guessed pointer.

The original 48-pointer VClient011 vtable was previously SHA-pinned in Phase533: zero-based slot5 maps to **0x240FD990**. Its whole six-byte original method SHA256 is **de7983e64b69321348f64408db25c7f2a4215cf4201e17d4e10eb2f43a64d41e**; 10 bytes of INT3 padding follow. It returns the dword at source global **0x2433FD90** and does nothing else. The source 46-byte register helper at **0x24053C40**, SHA256 **130516addf1c8a0c3e90dc8950e64168eb2c2872441a86e96b2942d6b360341b**, writes an object's first four pointer members from arguments, stores the old global-head in node **+0x10**, then makes the object the new head. This recovers the *real head-prepend rule*, not a guessed Source SDK interface.

The original DLL is the owner's unchanged 2006 PE32 client.dll, SHA256 **6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d**. All private audits read it from the user's existing game files, never write original binaries to GitHub or public CI.

## Complete original class-registration topology

The original MSVC CRT static initialization pointer array begins at original client VA **0x242F3004** and contains **1,582** callbacks before its zero terminator. The source audit resolves exact callback entry addresses, not inferred file-name order. Of those callbacks, **191** perform complete node-head registration to global 0x2433FD90. Every one independently satisfies these original machine-code features:

- its CRT function begins by loading the current source global head;
- it has a single original per-node predecessor store at **node+16**;
- it publishes the very same node address to the original head and returns;
- class metadata are recovered from original static .data initializers or exact original constructor immediate stores, never read from imaginary .bss file bytes;
- the one non-null creator pointer is in original .text, name in original .rdata, and RecvTable pointer in original .data.

There are **191 unique class names, 191 unique class nodes and 191 distinct RecvTable addresses** among these registrations. Their source creator split is **149 normal entity-creator entries** and **42 temporary-event creator entries**, with *exactly one* creator pointer non-null for each registered class. The CRT callback index order and final linked-list order are opposite because every node is prepended.

The source list head is **CWeaponMelee**, and its final tail is **CBaseTempEntity**. The source audit computes a SHA256 fingerprint over the *full 191-row original source manifest*, including CRT indices and both original creator-function pointers:

`ded698d9f9b6c671e17a03a23a0e909b4acb5d8c6030f905af2899471d616c24`

The separately authored, portable public TSV `research/manifests/original_client_class_catalog_phase1696.tsv` records in exactly that list order: name, actual CRT initializer index, entity/event kind, original source node VA and source RecvTable VA. The public file's own digest is **a5907135131d8dc216ca2b088b587a30314ff0d60f290237c0ba43b787414585**. The private audit does an exact **191/191 row-by-row** comparison against the real PE32, not merely a total count and matching names. The manifest contains **no** whole original function bytes or usable host function pointers.

### Important Source-engine ABI distinction

Original PE32 node members start with **normal factory**, **temporary-event factory**, **network name pointer**, **RecvTable pointer**, and **next pointer at byte +16**. Their original code virtual addresses are **not** valid standalone x64/Android host pointers. The original source additionally has possible class IDs and runtime state outside this five-field prefix. None are assigned from this manifest.

The compiled C++17 `include/sin_compat/original_client_class_catalog.hpp` reads the exact authored manifest into a host-owned, stable, singly linked **metadata-only** catalog with validated order, unique names/node and table addresses, validated 149/42 split, reject-on-invalid rows and no mutation after publication. This does **not** implement the engine's ClientClass or RecvTable ABI: `CanExposeToRetailEngine()` deliberately remains **false**, and the real replacement client still has its existing terminal slot5 diagnostic until the network table/creator body requirements are understood.

## All 191 class receive tables are traceable to real constructor calls

The distinct static client constructor at **0x24126A30** is a whole SHA-authenticated 34-byte original function (SHA256 `6f9e7546053378e2146df443c8244c8250fa214ff339c5404fefd8c291a05555`). It assigns the source RecvProp array pointer at RecvTable +0, count at +4, metadata at +8, and table-name pointer at +12. The earlier original RecvProp audit confirms a source descriptor stride of **0x3C (60 bytes)**.

Source scanning for its actual relative CALL destination, table-object pointer, property array pointer, count and original table-name literals yields **214 source constructor callsites describing 211 distinct RecvTable objects**. Three objects have **repeated, identical** constructor references (including DT_BaseTempEntity, DT_GameRulesProxy and DT_SE1GameRules). The 191 ClientClass records all map to one of these objects, **191/191 matches**.

Across the 191 attached tables the original constructors declare **939 RecvProp slots**: **729** for 149 normal creator classes and **210** for 42 temporary-event classes. **Two** attached class tables have zero declared properties. These are counts of *source-declared slots*, NOT 939 fully reverse-engineered fields or packet layouts. The remaining 20 distinct constructor tables can be nested/auxiliary tables and are **not** counted as ClientClass nodes.

**Source-specific naming exception:** class `CKeypad` registers table `DTKeypad` (without the normal `DT_` underscore), count **3**, original table VA **0x2437BA84**, source property array **0x2437B9CC**. Its constructor call is located at source `0x2421BBD6`, within the guarded property setup at `0x2421BB40`. A DT_-only scan incorrectly claims the table is missing. The new audit accepts this actual naming exception while retaining exact original caller validation.

Full ordered 191-class-to-RecvTable source mapping fingerprint:

`0ba3a72d88cbf5e0881232cf39a3b00ded5bcf59e73f280a4e73502ca7869fbc`


## All 939 original client RecvProp names recovered, not just slot counts

A further read-only SHA-pinned original PE32 scan resolves **939/939 attached class property names** in the exact class head-to-tail order, preserving each table's original declared slot index. The ordered class|table|slot|name fingerprint is:

`ca5f6949f6b477a4257e8357a7009e0908a016dc9838e5ec3746c3cc1a770b31`

**916** named slots originate in the common native `push property-name; push 60-byte RecvProp destination; call constructor` instruction sequence. **23** more are genuine alternate original construction patterns: **12** use an array helper at `0x24126BD0` and **11** use a `rep movsd` / array-copy operation followed by a helper at `0x24126E90`. Requiring the canonical push/push/call sequence for every slot would falsely report these 23 properties as missing. For example, the seven consecutive `CPlayerResource` properties `m_iPing`, `m_iScore`, `m_iDeaths`, `m_bConnected`, `m_iTeam`, `m_bAlive`, `m_iHealth` use the array-copy construction.

Important SiN-specific source fields recovered on `CNPC_GruntBase`: `baseclass` at original x86 object offset 0; `m_receivedDecapitatingBlow` at original x86 offset **3444**; and `m_decapitateVelocity` at original x86 offset **3448**. Those are raw 2006 PE32 offsets, *not* automatically valid ARM64 or Quest offsets. The source `CKeypad` nonstandard `DTKeypad` table has three genuine property names, `baseclass`, `m_bDisabled` and `m_combination`. The native literal for `CTeam`'s fifth property is exactly `"player_array"` including the quotation marks; retain that original literal instead of normalizing it.

The new `research/tools/audit_sin_original_client_class_recvprop_names.py` audits the full owner-original DLL SHA, all 191 original RecvTable links, all 939 explicit property slots and the 916/12/11 split, and checks the whole name-sequence fingerprint. It reports selected source constructor callsites and the original **limited** numeric x86 offsets where uniquely determined. The public `--self-test` refuses fabricated original DLL identity and checks the alternate-name scanner, while **only a private owner-original run** verifies original instruction bodies. No original DLL bytes are copied into public CI.

These are now **names**, not 939 authenticated serialized field data types, native RecvProxy signatures, nested-table semantics, class IDs or runnable original entity factories.

The full 939-slot metadata export is **owner-private and opt-in**, via `--full-json`: the tool only accepts this option together with the SHA-pinned unchanged original client.dll. Output includes all source class names, table names, slot indices, constructor route labels and the original x86 member offset where proven. No executable original function bytes are written. It is not safe to pass these source VA labels to the retail engine or to an Android ARM64 module. Do not return this catalog as a live `ClientClass*` to the retail engine. Slot5 remains intentionally guarded until its ABI can be satisfied.

## Verification

Public, commercial-file-free checks:

```sh
python research/tools/audit_sin_original_client_class_registry.py --self-test --catalog research/manifests/original_client_class_catalog_phase1696.tsv
python research/tools/audit_sin_original_client_class_recvtable_links.py --self-test
python research/tools/audit_sin_original_client_class_recvprop_names.py --self-test
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude tests/test_original_client_class_catalog.cpp -o /tmp/sin-class-catalog
/tmp/sin-class-catalog research/manifests/original_client_class_catalog_phase1696.tsv
```

Private, user-owned original binary read-only checks:

```sh
python research/tools/audit_sin_original_client_class_registry.py --client /private/client.dll --catalog research/manifests/original_client_class_catalog_phase1696.tsv
python research/tools/audit_sin_original_client_class_recvtable_links.py --client /private/client.dll
python research/tools/audit_sin_original_client_class_recvprop_names.py --client /private/client.dll
python research/tools/audit_sin_original_client_class_recvprop_names.py --client /private/client.dll --full-json /private/phase1696-class-property-metadata.json
```

Both new synthetic Python audits and the identical native C++ catalog test are registered in standalone Windows MSVC x86 CTest and focused Linux CI. The original binaries **never** enter GitHub Actions. A green synthetic test is not a real retail-engine class registry test.

## Next real work, and what remains explicitly excluded

To eventually return a valid result from `VClient011.slot5`, reconstruct the live 191 source RecvTable objects including exact field types, flags, nested table semantics and proxy callbacks; recover the 149 entity and 42 temp-event creator interfaces and correct native memory ownership and class IDs. Test compatibility through actual engine requests in a disposable copy only *after* source evidence and independent ABI tests. Never return an invented empty class list to bypass startup.

This phase does **not** implement deferred HL2-equivalent physics, model/prop reading/loading/rendering, surfaces or leaf drawing. No original DLL has been modified, no new retail gameplay run happened, and the original Phase1695 owner diagnostic remains the most recent real engine checkpoint.
