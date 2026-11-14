# Phase 225: original client shared prediction trace formatters

14 November 2026. **Stacked DRAFT on Phase224 PR #279.** Rechecked both owner-provided original 2006 SiN Episodes: Emergence PE32 game DLLs. Original full SHA256 fingerprints match previous evidence: client.dll 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d (3,592,192 bytes) and server.dll 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 (6,049,792 bytes). No original DLL, complete proprietary method body, disassembly dump or game asset is committed.

## Three complete original native function SHA256 reviews

| Original client VA | Whole x86 bytes | Whole original method SHA256 | Native return | Original INT3 pad |
| --- | ---: | --- | --- | --- |
| 0x24197910 | **166** | d5d121f8e3a25d66fc8f3814fe2a11b6124d751dd640469e844050df608ce7d9 | C3 | 10 |
| 0x241979C0 | **185** | 026244a82ffe4c257ea97eaacbb7e83c812b4fd6ffbcd064a0fad88dccd52e32 | C3 | 7 |
| 0x24197FE0 | **93** | 880a186d02da86b94b953158d2fdaee8a93d59456fdaf4f482031da3bab09cac | C3 | 3 |

**Three new whole-function SHA-verified methods, 444 complete original native x86 bytes.** All three are caller-cleanup C3 varargs wrapper routines, not RET-immediate stack-pop field specialists. Each allocates a native 4096-byte stack buffer through a separate call to original stack-probe 0x242493E0. The original imported functions are verified by PE import name: vstdlib.dll!Q_vsnprintf via IAT 0x2427625C and tier0.dll!Msg via IAT 0x242761EC. These external library implementation bodies remain unreviewed.

The verifier independently checks 21 existing native original caller CALL rel32 sites targeting these three wrappers across multiple comparison and preview specialist families, as well as all three stack-probe instructions.

## Native control flow at 0x24197910: difference message

- Always increments original DWORD(this+0x28), wrapping modulo 32 bits, even if no message is printed.
- When BYTE(this+0x24)==0, no Q_vsnprintf or Msg occurs. The counter increment remains.
- When BYTE(this+0x16)!=0 and DWORD(this+0x30)!=0, exits without Q_vsnprintf or Msg and WITHOUT clearing this+0x24.
- Otherwise selects the descriptor name from DWORD(this+0x18): null descriptor selects "empty" at 0x242A3E38; null descriptor+0x04 name selects "NULL" at 0x242A0174; a nonnull name pointer is forwarded.
- Q_vsnprintf formats caller-supplied raw varargs into a 4096-byte native stack buffer.
- When the counter is exactly one, native Msg prints the string at 0x2429C58C (a newline).
- Native Msg then formats "%03i %s::%s - %s" at 0x242A3E24, passing the counter, group string at this+0x1C, descriptor field name and formatted content.
- Finally clears BYTE(this+0x24); suppressed paths do not clear it.

## Native control flow at 0x241979C0: callback

- Requires BYTE(this+0x25)!=0 and DWORD(this+0x30)!=0. Otherwise returns without formatting or clearing the pending bit.
- Reads original descriptor flags as a signed 16-bit value from descriptor+0x12, extracting independent flags bit 8 and bit 10.
- Q_vsnprintf formats forwarded varargs to a separate 4096-byte stack buffer.
- Reads descriptor DWORD type+0x00, indexing the original 27-pointer array in .data at 0x242FDAE0: 26 named FIELD_* strings followed by an actual null table entry. All original strings are verified in .rdata, including FIELD_VECTOR, FIELD_QUATERNION, FIELD_EHANDLE and FIELD_INTERVALFIELD_MODELINDEX.
- Calls native indirect function pointer DWORD(this+0x30). The eight pushed logical parameters in source order are: group string at this+0x1C; descriptor name; field-type label; descriptor bit8; descriptor bit10; (operation != 1); (operation == 2); formatted text.
- Clears BYTE(this+0x25) after the callback. This wrapper does NOT call Msg itself.
- Null descriptor in the active callback path would be dereferenced in original x86; the host reference explicitly refuses it.

Native operation==2 distinguishes an approximate vector comparison result documented in previous phases.

## Native control flow at 0x24197FE0: unconditional preview Msg

- No gate checks native enable bytes this+0x14, +0x24 or +0x25. It formats the forwarded varargs immediately using the original 4096-byte Q_vsnprintf stack buffer.
- Reads descriptor pointer this+0x18 and its name pointer at descriptor+0x04, plus label pointer this+0x38.
- Reads DWORD global pointer at original .data VA 0x242FA458, then DWORD(global+0x18).
- Calls original tier0 Msg with format "%i %s %s : %s\n" at original rdata 0x242A3F90, passing global number, object label, field name and formatted result.
- The original unconditionally dereferences descriptor/name pointers; host model rejects null rather than inventing original engine objects or fault outcomes.

## Files, limits and reproduction

- research/tools/audit_sin_original_client_shared_trace_formatters.py: whole-original dual-DLL SHA256 gate; three complete native method SHA256s; **53** sparse original instruction witnesses; exact RET/INT3 bounds; 3 stack-probe and **21** nested serializer native CALL sites; five original string literals; 26 field-type strings and 27th null table pointer; original preview global .data location; named Q_vsnprintf and Msg imports.
- research/tests/test_sin_original_client_shared_trace_formatters.py: synthetic-only mutation regression for all opcode/CALL/RET/pad/string/field-table/import and PE base/section evidence. Synthetic fixtures never count as genuine whole-original SHA function reviews.
- include/sin_compat/original_client_shared_trace.hpp: independent portable C++17 reference preserving state gates, modulo-32 difference counters, first-message newline, callback flags, 26 table names, event ordering and preview. Host accepts raw 32-bit i386 vararg words and supplies string formatting and actual Msg/callback sinks.
- tests/test_original_client_shared_trace.cpp: difference suppression, name fallbacks, counter wrapping, 4096-byte host-only output bounding, operation status flags, callback gates, invalid descriptor/type refusal, and preview independence.
- Linux CTest and native Win32 x86 MSVC CTest target registrations test only the host reference, not a retail engine gameplay DLL.

The actual original Q_vsnprintf return/truncation behavior, source string pointer lifetimes, Msg implementation, status callback object and original engine lifecycle remain unresolved. The C++ reference conservatively truncates host formatted output at 4095 characters but does not claim this models every native Q_vsnprintf error case.

Owner-original audit command:

    python3 research/tools/audit_sin_original_client_shared_trace_formatters.py --archive /private/bin.zip

Original-free mutation regression:

    python3 -m unittest discover -s research/tests -p 'test_sin_original_client_shared_trace_formatters.py' -v

## Checkpoint

Reviewed whole-original SHA function inventory **274 → 277** (**server 128, client 149**), adding three formerly opaque TRANSITIVE formatter bodies. The earlier scoped Phase212 36-direct-target router graph remains fully reviewed; this does not imply a complete client.dll or server.dll decompilation, a reconstructed original Source engine ownership graph or a playable Windows replacement gameplay DLL pair.

Priority remains a genuine Windows retail Source engine loading reconstructed gameplay DLLs, followed by Android ARM64 and Quest 3.
