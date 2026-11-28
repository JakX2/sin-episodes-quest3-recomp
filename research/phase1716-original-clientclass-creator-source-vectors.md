# Phase 1716: original 191 ClientClass creator address source map

## Original 2006 PE32 evidence

Unmodified original retail client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Original VClient011.slot5 returns a 191-entry native ClientClass
linked list, whose registration order is the reverse of the 1,582
original C runtime initializer callbacks. The original constructor
initializes each 20-byte ClientClass with two potential executable
factory fields: entity creator at offset +0 and temporary-event
creator at offset +4. It has a network name at +8, a RecvTable at
+12, and its linked-list next pointer at +16.

Exactly one creator field is non-null per original node. Examining
all 191 original CRT registration methods and their source-native
node stores gives exactly:

- 149 native entity creator functions.
- 42 native temporary-event creator functions.
- 191 distinct original x86 function virtual addresses inside the
  original unchanged PE32 executable text section.
- The identical 191-class head-to-tail order, names, original class
  nodes and original RecvTable addresses already authenticated by
  Phase 1696.

The independent CRT/register record digest exactly matches the
previous SHA256:
ded698d9f9b6c671e17a03a23a0e909b4acb5d8c6030f905af2899471d616c24.

This is strong original-source metadata evidence; it does not
decompile any of the 191 creator function *bodies*, identify the
entity allocation interface, or implement an executable creator.

## Source-only artifacts

The public original source manifest:
research/manifests/original_client_creator_source_map_phase1716.csv

It contains all 191 original class ordinals, native entity/event
kind, source creator VA, original CRT registration index, native
ClientClass node VA and native RecvTable VA. It contains NO original
executable bytes. Its LF-normalized SHA256 is:
c1083d4cbb610c81e66dbea8d80c96085ac15c942ccdf4e022d35d167bc62f29.

Portable C++17 immutable metadata:
include/sin_compat/original_client_creator_source_map.hpp

The header provides 191 original source creator identity DWORDs,
original entity/event kinds, a source text VA range check and
compile-time uniqueness/cardinality assertions. Original x86
addresses are labels, not portable function pointers or synthetic
arena tokens; no method attempts reinterpret_cast or invocation.

## Original-free tests and private source validation

Original-free audit:
python research/tools/audit_sin_original_client_creator_source_map.py --self-test

Original retail owner-private audit (does not export executable bytes):
python research/tools/audit_sin_original_client_creator_source_map.py --client /private/client.dll

The public auditor authenticates the complete ordered source CSV SHA,
kind/CRT/VA correspondence to the independent Phase1696 class
catalog, exact 149/42 split, unique original function VA inventory,
and entire canonical 191-class creator digest. The optional private
path invokes the exact original registry CRT/source-node verifier
and compares every original creator address with the new manifest.

Native CTest:
tests/test_original_client_creator_source_map.cpp

It loads the real original 191-class catalog, builds the safe
host-owned 32-bit image, and checks every native source creator,
class ordinal/name/kind, CRT index, native node/table reference,
and source pointer uniqueness. Critically, every image ClientClass
still has **both +0 and +4 callback DWORDs equal to zero**.
The metadata source addresses are never copied into executable
host callback positions or exposed to the original Source engine.
Linux C++17 strict, ASan/UBSan, Windows MSVC Win32 and portable
Android CMake validation use the existing testing workflow.

## Actual startup boundary is unchanged

These 191 original source creator VA labels alone are NOT portable
creators. The actual functions vary widely, allocate original
2006 x86 entity object layouts, and may call engine allocation,
constructors and vtables. Before real VClient011.slot5 can return
to the retail engine, creator calling conventions, object ownership,
native RecvTable proxy function pointers, special arrays/overlay,
network table lifecycle, and engine integration remain unfinished.

The native owned-token image intentionally retains NULL +0/+4
creator callback DWORDs and its CanExposeToRetailEngine() guard
remains false. No actual user Windows game DLL has been modified;
no new real original-engine game startup, Android executable or
Quest 3 gameplay is established.
