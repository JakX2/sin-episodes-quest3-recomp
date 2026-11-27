# Phase 1650: Attest the loaded original engine's allocator instruction witnesses

**Stack:** Phase 1649 [PR #1658](https://github.com/JakX2/sin-episodes-quest3-recomp/pull/1658), head `ee9ac27b532ac9f5e21f710331aa2dfaf7c1cc2f`, Windows MSVC Win32 native **248/248** CTest and all six applicable original-free/native workflow families green. No live retail process has been observed.

## Gap closed

Prior owner-only read-only edict observers SHA-256-checked the *file on disk*, then used `ReadProcessMemory` for the four original allocator globals and up to four explicitly selected edict records. This was not an attestation of the **loaded instructions**, which could be patched, relocated incorrectly, or unexpectedly different from the original PE32 file.

This phase introduces `research/tools/attest_sin_original_engine_live_allocator_code.py`, an independent original-free helper that checks **ten SHA-backed, narrowly scoped original 2006 PE32 allocator instruction witnesses** in the loaded module. No arbitrary code scans, original executable bytes or disassembly are exported.

## Authenticated in-memory windows

Source evidence is the original engine SHA-256 `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81` and the existing audited short x86 windows from:

- `research/tools/audit_sin_original_engine_gameents_edict_reuse.py`;
- `research/tools/audit_sin_original_engine_gameents_edict_retire.py`.

The instruction VAs and semantics reused:

| Original VA | Instruction role | RVA/operand behavior |
| --- | --- | --- |
| `0x20110ECC` | Select protected-last edict index | absolute DWORD `0x2079DA64` |
| `0x20110ED1` | Load owner edict base pointer | absolute DWORD `0x2079DB04` |
| `0x20110EDE` | Load current edict count | absolute DWORD `0x2079DAFC` |
| `0x20110F36` | Load edict capacity | absolute DWORD `0x2079DB00` |
| `0x20110FBB` | Subtract owner edict base during reuse | absolute DWORD `0x2079DB04` |
| `0x20111024` | Compare selected index to capacity | absolute DWORD `0x2079DB00` |
| `0x20110FE8` | Dispatch GameEnts free callback slot `+0x0C` | fixed short opcode |
| `0x20110FEB` | Set edict active bit `0x02` | fixed short opcode |
| `0x20111000` | Increment original retail serial DWORD | fixed short opcode |
| `0x20111004` | Store reuse timestamp from x87 | fixed short opcode |

For the six instructions with a 32-bit absolute memory operand, expected bytes use `loaded_engine_base + (original_operand_VA - 0x20000000)`. This supports the PE32 loader legitimately rebasing original absolute addresses instead of expecting unrelocated original-file operands. Instruction locations are similarly rebased. The verifier rejects module sizes that cannot hold all referenced global RVAs and all witnessed code extents or that would overflow a 32-bit mapping. Every code window is read **twice** with `ReadProcessMemory`, and mismatches, truncated windows or read failures refuse the observation.

The verifier never executes these instructions or follows the global address operands; it uses only read/query process permissions and returns status labels, never raw code bytes or target addresses.

## Observer integration and owner usage

`Win32Reader`, shared by `observe_sin_original_engine_edict_owner.py` and `observe_sin_original_engine_edict_records.py`, now:

1. Opens only the owner-specified PID with `PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION`.
2. Identifies `SinEpisodes.exe`, loaded `engine.dll` path and module size, and verifies the exact original *on-disk* engine.dll SHA-256.
3. **Before any edict observation**, double-checks all ten original allocator instruction witnesses inside the **running** process, including relocated absolute operands.
4. **After the sample window**, double-checks the same ten witnesses again. Failed post-sampling checks reject the final result.
5. Emits `LOADED_ENGINE_ALLOCATOR_CODE_WITNESSES_BEGIN=10` and `..._END=10` to identify successful bracket checks, without exporting any code bytes.

Both observer commands retain the previous explicit owner opt-in and bounds. Example:

```powershell
$game = Get-Process -Name SinEpisodes | Select-Object -First 1
py -3 research/tools/observe_sin_original_engine_edict_records.py `
  --pid $game.Id --owner-read-only-confirmed --edict-index 2 `
  --samples 300 --interval-ms 100
```

All read operations are passive. Nothing is injected, written to, hotpatched, debugged or loaded into the retail process. A SHA file hash plus in-memory code witnesses provide **stronger negative mismatch rejection**, not cryptographic proof that a process is uncompromised, and cannot prove that the loaded code stays constant between checks.

## Synthetic regression and remaining gaps

Portable original-free `--self-test` constructs ten invented instruction windows, including absolute operands relocated to multiple synthetic PE32 module bases. It verifies both read passes, every individual opcode corruption, every relocated operand corruption, a change between passes, unreadable/short reads, invalid base/size, and strict maximum snippet read size. It uses no proprietary DLL or live game process. Registered in the Linux original-free decomp GitHub Actions workflow and Windows MSVC x86 native CTest (expected total **249**).

Phase 1648's native Windows self-process `ReadProcessMemory` smoke now exercises the new `Win32Reader.read_bytes` path on its own process, **and all ten synthetic in-memory opcode/relocation witnesses** are read through the real Win32 API using independent locally owned buffers. A modified witness must be rejected. This does not open or inspect retail game memory. The bounded reader is pointer-width-aware so 64-bit Python CI on a 64-bit Windows runner can read its own local test values, while the actual original PE32 verifier strictly requires 32-bit module addresses.

This does **not** show a retail edict allocation pin, real native `ServerGameEnts001` registration, safe game-side entity ownership or playable Windows/Android/Quest 3 startup. No retail observation by ChatGPT or public CI is claimed. The user's priority override still excludes generic HL2-equivalent physics, surfaces, props and model loading.
