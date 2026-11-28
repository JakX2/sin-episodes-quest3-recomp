# Phase 1718: 142 original entity factory allocation prologues, NOT constructors

## Retail 2006 PE32 evidence

Unchanged user-owned original SiN Episodes client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

From the exact Phase1716 native ClientClass ordered 191-creator catalog:
149 functions are entity creator callbacks, 42 are static
temporary-event getters (Phase1717). The first 11 executable bytes
of exactly **142 of the 149** entity creator functions all have
the original same instruction sequence:

    PUSH ESI
    PUSH original unsigned object allocation SIZE (imm32)
    CALL original client.dll 0x240C4FF0

The 142 original CALL instructions use x86 E8-relative32, and
all resolve to the SAME original allocator helper VA 0x240C4FF0.
The PUSH SIZE operand begins at original creator VA +1 and
the E8 CALL at +6. The 142 size immediates are NOT inferred from
Source SDK types or later engine definitions.

The 142 actual allocation request sizes include **69 distinct
four-byte-aligned byte counts**, ranging from **1,152** to
**10,048** original bytes. These are size requests, not
complete original entity object layouts or safe portable sizes.
The SHA256 of all 142 exact 11-byte original executable prologues
concatenated in authenticated VClient011.slot5 class-list order is:
3562d9fedfa0db8a3e570ef023130e519cfcc3c48c2248c5b0c6ce0aa059c1ba.
These bytes are checked privately, NOT committed to GitHub.

Seven original entity creators do NOT match the above common
allocator prologue and are explicitly excluded. Their original
class-list ordinal and name:

- 48 CInfoTeleporterCountdown
- 69 CRagdollPropAttached
- 75 CWorld
- 120 CBaseEntity
- 143 CTEParticleSystem
- 170 CTEBaseBeam
- 190 CBaseTempEntity

The last three use an alternative source allocation-related
helper at 0x2416FCF0, while the other four have separate
function structure requiring individual analysis. Those are
initial callsite observations, NOT source-backed 2006 object
construction contracts.

## Public source-only metadata and SHA validation

research/manifests/original_client_142_entity_allocator_prologues_phase1718.csv

Records class ordinal/name, original creator VA, original byte
size request, original allocation helper VA, native PUSH-size VA,
native CALL VA, and original CRT initializer index.
Its LF-normalized metadata CSV SHA256 is:
68fe5b23a33cd1b724fe9aa3761c667ef63333efcbe7c4c3afc32eb13dfa5c0e.

include/sin_compat/original_client_entity_allocation_prologues.hpp

This portable C++17 constexpr catalog holds 142 source creator
identities with their original requested sizes. It rejects
duplicate source classes/creators, wrong source PE32 creator
identities, non-entity classes, invalid original size ranges,
and incorrect 142/7/149 size/category counts. Its explicit
CanCreateRetailEntity() always returns **false**.

The original-free audit rechecks SHA256, exact class order,
native VA, CRT index, metadata C++ records, the 69 distinct
sizes and the seven explicit exceptions:

    python research/tools/audit_sin_original_client_entity_allocation_prologues.py --self-test

Optional read-only original-owner source audit:

    python research/tools/audit_sin_original_client_entity_allocation_prologues.py --client /private/client.dll

The private auditor verifies the entire unchanged PE32 DLL
SHA256, all 142 exact original PUSH ESI/PUSH SIZE/CALL opcode
and relative E8 destinations, and concatenated 142x11-byte
original native prologue SHA256. It does NOT claim to authenticate
all instructions within 142 complete creator methods.

Test:
tests/test_original_client_entity_allocation_prologues.cpp

The strict C++17 test loads all 191 original class catalog
records, matches every 142 source CSV record and compile-time
record to the exact class creator and CRT index, validates
69 sizes and seven exceptions, and proves both native32
ClientClass creator callback fields +0/+4 remain NULL for
every source-mapped entity. Linux AddressSanitizer and
UndefinedBehaviorSanitizer plus Windows and Android CMake
builds check the original-free reference code.

## Next actual source / gameplay boundary

Reconstruct the original helper 0x240C4FF0 complete method
body, its allocation ownership, and the entity-specific
constructor chains and vtables. A correct allocation-size
immediate alone does not instantiate a valid 2006 entity.
Before real VClient011.slot5 can be unguarded, the original
creator function pointers and receive-table callback ABI
must be portable executable code with valid memory lifetime
and verified engine effects.

No PE32 executable bytes were shipped, original game files
were not overwritten and no real game startup, map boot,
Android/Quest 3 gameplay, physics/models or renderer is
claimed by this source-only classification phase.
