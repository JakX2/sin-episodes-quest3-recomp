# Phase 1641: Original engine negative-index edict selection

Stacked on the Phase 1640 source-backed reuse transition PR #1649 (`e6da2cf9dcf44cd21dd6776d0f64fec2410dc230`). This phase reconstructs the missing **selection of a reusable edict**, prior to the already implemented per-record restamp.

## Original 2006 engine.dll evidence

The owner-provided original SHA-256 `f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81` executes the negative-index branch from 0x20110ECC to 0x20110FA7 (220-byte SHA-256 `cae5a58071ef8531ac82c3d000dfce4c68a09d70c4d2eb20887f4e703ae3427b`). It reads the **protected last index** at original VA 0x2079DA64, **array base** 0x2079DB04, **current count** 0x2079DAFC and **capacity** 0x2079DB00. The scan starts at protected_last+1 and examines 20-byte records with bit 0x02 set.

The x87 compare at 0x20110EFB reads the 32-bit float **2.0** at 0x202DBF40, whose original file-backed bytes are `00000040`. A record with reuse-time float below 2.0 is selected. Otherwise, 0x20110F08 loads a signed tick at 0x2076EB6C, first rounds it to float, multiplies by scale at 0x2079DA6C, subtracts the record's float at edict+0x10 and compares the *unrounded* intermediate result against original double **1.0** at 0x202DBF60 (bytes `000000000000f03f`). A record aged at least 1.0 is eligible. x87 unordered values are conservatively skipped by the independent core.

If no eligible record is found and current_count is below capacity, the engine selects the new record at current_count, invokes GameEnts +0x0C FreeContainingEntity, clears edict bit 0x02 and post-callback +0x10 and +0x04, then increments count. Existing reused slots follow the same retire path without increasing count. The original source includes an assertion when capacity is exhausted; the independent bounded helper reports `NoEligibleRecord` without attempting writes. This is a safe conservative difference, not a claimed original return value.

## Independent implementation

`server_gameents_engine_edict_select_core.h` adds `SelectEngineEdictForReuse`. It validates whole-array ownership and the caller-provided capacity, count and protected index before inspecting candidate records. Each candidate is separately owner-authenticated. For a selected record, the existing independent `RetireEngineEdict` crosses the injected server-cleanup callback, reauthorizes the record and only then clears reuse-time and serial. The returned status distinguishes `SelectedExisting` and `SelectedNew`, including the proposed next count.

`test_server_gameents_engine_edict_select.cpp` uses five independent 20-byte records to cover an early time <2.0 candidate, elapsed age >=1.0, exact 2.0 threshold exclusion, protected/inactive records, append vs capacity, invalid count/index, foreign/denied owner, refused callback and post-callback ownership revocation. This is native C++17 and retains the original-free factory-off boundary. A new CTest increases the expected Windows test count from 242 to **243**.

The original-free auditor now pins the entire additional 220-byte negative-index function window, asserts **31** opcode witnesses across four original source windows, and verifies both original 2.0f and 1.0d data literals with synthetic corruption tests. The original user-provided engine binary was privately checked with the read-only auditor. Neither DLL bytes nor disassembled original function bodies are committed.

## Remaining integration

This is a deterministic engine-style allocator *transition and selector*, not a live retail allocator, CreateInterface registration, physical memory pin, client/server startup, Android gameplay or real-source-engine binary replacement. A trusted Windows host must still supply an authenticated edict array/epoch journal, own the count/capacity mutation, implement a physical lifetime pin, and connect callbacks to the independent GameEnts vtable. No HL2-equivalent physics, surfaces, props or model rendering are in scope.
