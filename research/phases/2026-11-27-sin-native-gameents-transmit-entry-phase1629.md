# Phase 1629: Original ServerGameEnts001 CheckTransmit entry composition

## Exact original evidence

Owner-local server.dll SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` rechecked directly. Original vtable +0x18 method VA `0x221e94d0`, 354 bytes, SHA-256 `e069400e844edfe9c5e756bfd828abfc6764df7b37ce13dfca374f94c25ad176`, already catalogued in Phase 1628; do not count as a newly recovered original method. Direct original disassembly establishes the following entry sequence and distinctions:

- First query: original owner 0x22572198 virtual +0x4c at 0x221e94de, argument zero. Save returned table pointer for the later candidate loop.
- Read recipient edict from info+0 at 0x221e94e9. If null, repeat the same owner call at 0x221e94f8; use this second result only as fallback recipient edict. If the second result is null, return. Do not overwrite the saved first candidate-table pointer.
- Resolve recipient edict+0xc and its virtual +0x14; return if either is null. Snapshot recipient +0x828 (dword area) and +0x92c (byte flag).
- Original signed count check at 0x221e953b exits for count<=0, after the recipient snapshot, before candidate iteration.
- At 0x221e954c reload the saved first table pointer. Candidate index is uint16_t; each table record is 20 bytes, flags at +0, entity pointer token at +0xc. Original direct-always and already-selected paths avoid reading the entity token.
- Context +0x2008/+0x200c supply selected/always bitset pointers. Existing Phase 1627 report documents the remaining flags and callback order, which the existing policy already models.

The updated audit_sin_original_servergameents_methods.py adds ten further precise instruction witnesses (36 total). Locally ran the expanded auditor against the authenticated original DLL with --self-test, including targeted corruption rejection; result ORIGINAL_GAMEENTS_TOUCH_AND_TRANSMIT_FLOW_AUTHENTICATED. Original binaries and disassembly stay private.

## Native implementation

server_gameents_transmit_entry_core.h reads bounded original-format PE32 records and context pointer tokens as little-endian uint32 values; it never casts those values to live host pointers. Owner callbacks acquire table storage, resolve recipient, map writable bitsets and resolve candidate entity tokens. The returned primary table storage must stay valid through a fallback query and the entire call. Recipient values are snapshotted before iteration, and signed nonpositive count returns without requiring candidate/bitset storage. The typed CheckTransmit entry and raw-record entry share exactly one policy loop via CheckTransmitAccess; no duplicated PVS/SetTransmit implementation. Direct bit selection and already-selected entries avoid resolving the entity token.

Explicit independent safety differences: bounded info/table/index extents and stable owner-provided bitsets are preflighted; original reads bitset pointers during the loop. Failed entity-token resolution returns HostFailure and does not roll back earlier bit writes. The owner provides real PVS, entity lifetime, recipient and SetTransmit services; no retail ABI object is advertised. The host may not invalidate the table or bitset storage while executing callbacks.

New Win32 CTest independent_server_gameents_transmit_entry_native verifies primary/fallback table distinction with different table data, actual 20-byte strides and +0/+0xc fields, skip paths, always-bit behavior, recipient snapshot, repeated and propagated indices, signed negative/zero counts, missing recipient, short extents and provider failures. Existing typed policy regression remains intact. A C++17 local-variable default argument issue in the new test lambda was corrected before the PR was opened. Full C++ and Windows CI verification pending.

No original executable bytes committed. No generic physics, surfaces, props/models or rendering reconstructed. This is native entry composition for the existing independently authored entity policy, not successful retail engine startup.
