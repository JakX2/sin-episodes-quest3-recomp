# Phase 280: authenticated original SiN interface vtable method seeds

## Real original DLLs inspected (17 November 2026)

This phase directly examined the user's original SiN Episodes 2006
client.dll (3,592,192 bytes) and server.dll (6,049,792 bytes), authenticated
against the pinned whole-file SHA-256 for each. All observations were made
from the file-backed PE32 images, not fabricated from a generic Source SDK.

The stdlib-only parser validates i386 PE32 headers, image bases and sections;
previously pinned interface singletons in .data; MSVC32 RTTI
CompleteObjectLocators at vtable[-1], TypeDescriptors in .data and class
hierarchies in .rdata; plus each vtable pointer to an executable section.

| Module and interface | Verified pointer entries | Existing exact Ghidra function starts | Absent as exact starts |
| --- | ---: | ---: | ---: |
| server.dll: ServerGameDLL003 | 31 | 4 | 27 |
| server.dll: ServerGameEnts001 | 7 | 1 | 6 |
| server.dll: ServerGameClients003 | 15 | 2 | 13 |
| client.dll: GameClientExports001 | 4 | 0 | 4 |
| client.dll: VClient011 | 48 | 2 | 46 |
| **Total** | **105** | **9** | **96** |

The comparison uses the owner-original Ghidra 12.2 atlas produced on
November 17, 2026. The 96 entries not currently recognized at exactly
their target address might be valid starts Ghidra failed to infer, or
addresses subsumed inside other inferred functions. The 105 observed
pointers are distinct within the five reviewed prefixes.

**Evidence tier:** authenticated pointer values at previously established
PE-initialized class vtables. They do not establish complete C++ class
layouts, virtual-method signatures, inferred body extents, newly SHA-verified
original method implementations, or actual Source engine startup.

## Repeatable owner-private analysis

Run research/tools/recover_original_vtable_prefixes.py with the
--original-dir pointing at exact original client.dll and server.dll, optional
--ghidra-dir pointing at a previously extracted Ghidra atlas, and --out-dir
for metadata-only output. The tool authenticates original file size and full
SHA256 before interpreting pointers.

Outputs include original_vtable_prefix_review.json and the two
module-specific _vtable_method_seeds.csv files. These include only addresses,
slots, interface labels, RTTI locator VAs and evidence statuses. They contain
no game DLL bytes, original instructions, decompiled C, strings or assets.

If no prior Ghidra atlas is provided, exact-function-start matches are
reported as unknown/null, never as a misleading zero.

## Hosted Ghidra integration

After the original ZIPs are authenticated, the owner-only Actions job
revalidates all five pinned vtable prefixes and prepares 105 reviewed
method-target seeds. The Java script SeedAuthenticatedVtablePrefixMethods.java
then attempts Ghidra function creation at those addresses only.

The seeder never clears instructions or deletes functions. If the target
lies inside a different inferred function, it records the overlap for
manual review; if an exact start already exists, it preserves it. The
output CSV distinguishes already_inferred, seeded_candidate, overlapping
function, non-executable and explicit error statuses.

An independent synthetic ELF smoke removes two harmless function starts
and checks that both the existing factory seeder and new vtable seeder can
rebuild their respective candidates. Twelve synthetic PE32 Python tests
cover SHA256 gating, memory-section ownership, RTTI, corrupted pointers,
method-target validation and metadata-only output.

New interface-vtable seeding runs against originals on GitHub Actions
**only after the draft research branch is integrated into main**. The
real owner-original pointer audit described above was performed privately
in this session, but the new Java seeding step has not yet run on originals.

## Next ABI milestone

Use new seeded boundaries to decompile and inspect actual first-party
virtual method bodies for VClient011, ServerGameDLL003 and related interfaces,
then verify x86 calling conventions, initialization order and dependent
engine callbacks. Independently loadable Windows replacement gameplay
DLLs, real 2006 game boot, Android and Quest 3 remain unproven.
