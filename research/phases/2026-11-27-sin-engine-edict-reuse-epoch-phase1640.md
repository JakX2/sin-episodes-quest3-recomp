# Phase 1640: 2006 engine edict reuse transition and host epoch boundary

**Base:** Phase 1639 final tested commit `d71dc4809f3d21c4af085d4365e72e7b511305a4`. No original DLL bytes are committed.

## Original engine.dll evidence (source-backed)

The owner-private original 2006 `engine.dll` SHA-256 `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81` contains an edict reuse routine at VA **0x20110FB0 through 0x20111008**. The full 89-byte instruction window SHA-256 is `469c928666877faddded91ca47d71fe3439f9270dfb9f2962f621a04833303e1`. Its independent 0x14-byte indexed-edict lookup at 0x20111020 (39 bytes, SHA `aab72caf36338a6845884f0ed865ee4486677bf51bc479248f5d45daf1d1bfdc`) and neighboring reuse-age scan 0x20110EF0 (151 bytes, SHA `567f15c192da14cbbda102dcbd1099f90b947d192fc208ec814fb106cac7c0ee`) corroborate record indexing and timing state. All three SHA-256 windows and **22** short opcode witnesses were verified locally against the original owner file.

The function checks flag bit 0x02, skips if already active, computes index by subtraction from base at `0x2079DB04` and signed division by 20, and skips initial protected indices `1..[0x2079DA64]`. Other unallocated records cause a `ServerGameEnts001` virtual +0x0C FreeContainingEntity callback, then `or dword [edict],2`, signed engine tick input at `0x2076EB6C` converted through x87 to a binary32 temporary, scaled by the float at `0x2079DA6C`, a 32-bit serial increment at edict+0x04, and an x87 float store to edict+0x10. The indexed lookup uses capacity at `0x2079DB00`, distinct from the current allocation count at `0x2079DAFC`. These are **static original-image VAs**, not validated runtime pointers.

The neighboring allocator age scan also reads +0x10 as a float, compares it against its timing thresholds and walks records in 0x14-byte strides. The independent implementation in this phase covers only the per-record reuse transition, **not** the full selection/age-threshold/free-list policy.

## New independent C++17 core and tests

`server_gameents_engine_edict_reuse_core.h` adds `ReuseEngineEdictAt` and typed owner callbacks for whole-array authentication, record authentication, a GameEnts +0x0C release adapter and reservation of a **new globally monotonic uint64 ownership epoch**. The epoch is intentionally **not** the original per-edict uint32 serial: retail edict serials can wrap from 0xFFFFFFFF to zero, or repeat on different records. No entity is automatically enrolled. The callback's host must reserve epochs and later enroll genuine entity identities itself.

Its safety checks exceed the original routine: reject foreign array records and partial extents before forming element addresses; reauthorize physical storage and compare serial after server cleanup and after host epoch reservation, refusing changed/recycled storage without writing the engine epilogue. Callback refusal is reported explicitly and not transactionally undone. A trusted host must independently pin physical array pages across callbacks; the helper's repeated validation by itself cannot prevent adversarial concurrent deallocation.

The original x87 flow rounds the signed integer tick to binary32 before multiplying by binary32 scale. The independent helper uses a double-precision product for two binary32 operands, then rounds the resulting timestamp to binary32, and rejects invalid/nonfinite or overflowing scale computations.

`test_server_gameents_engine_edict_reuse.cpp` uses four synthetic 20-byte records and tests bit-2 active no-op, protected indices, negative/out-of-range index, untrusted array/record, missing authorizer, truncated extent, invalid timing scale, server cleanup callback denial, postcallback revoked ownership, serial ABA, epoch denial/revocation/ABA, original uint32 serial wrap, float pre-rounding and separate monotonic uint64 epochs on distinct records.

`audit_sin_original_engine_gameents_edict_reuse.py` provides SHA-pinned `--engine` read-only evidence and an original-free synthetic self-test that corrupts each of the 22 opcode witnesses, truncates each of the three windows and refuses fabricated bytes under strict SHA checks. This tool is added to the original-free decomp CI workflow. Native PE32 CTest total should increase from **241 to 242**.

## Boundary still missing

Neither the synthetic test's epoch callback nor the independent header is wired to retail engine allocation globals, a live Windows engine process or a physically pinned owner. Original `ServerGameEnts001` CreateInterface registration remains disabled. No retail game boot, Android gameplay, generic HL2-equivalent physics, surfaces, props or rendering is claimed. Next phase should implement and authenticate a host-provided engine array/slot allocator descriptor, then compose it with this transition and the existing owner registry.
